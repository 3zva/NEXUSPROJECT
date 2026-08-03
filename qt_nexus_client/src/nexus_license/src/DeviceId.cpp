#include "DeviceId.h"
#include "LicenseTypes.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")

namespace nexus::license {
namespace {

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int required = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
        nullptr, 0, nullptr, nullptr);
    if (required <= 0) throw LicenseException("Unable to encode the Windows device identifier.");

    std::string output(static_cast<std::size_t>(required), '\0');
    if (WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
            output.data(), required, nullptr, nullptr) <= 0) {
        throw LicenseException("Unable to encode the Windows device identifier.");
    }
    return output;
}

std::wstring ReadMachineGuid() {
    constexpr wchar_t kPath[] = L"SOFTWARE\\Microsoft\\Cryptography";
    constexpr wchar_t kValue[] = L"MachineGuid";

    DWORD type = 0;
    DWORD bytes = 0;
    LSTATUS status = RegGetValueW(
        HKEY_LOCAL_MACHINE, kPath, kValue,
        RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY,
        &type, nullptr, &bytes);

    if (status != ERROR_SUCCESS) {
        status = RegGetValueW(
            HKEY_LOCAL_MACHINE, kPath, kValue,
            RRF_RT_REG_SZ, &type, nullptr, &bytes);
    }
    if (status != ERROR_SUCCESS || bytes < sizeof(wchar_t)) {
        throw LicenseException("Unable to read the Windows device identifier.");
    }

    std::vector<wchar_t> buffer(bytes / sizeof(wchar_t));
    status = RegGetValueW(
        HKEY_LOCAL_MACHINE, kPath, kValue,
        RRF_RT_REG_SZ, &type, buffer.data(), &bytes);
    if (status != ERROR_SUCCESS) {
        throw LicenseException("Unable to read the Windows device identifier.");
    }

    std::wstring guid(buffer.data());
    if (guid.empty()) throw LicenseException("The Windows device identifier is empty.");
    return guid;
}

std::string Sha256Hex(const std::vector<unsigned char>& input) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectLength = 0;
    DWORD hashLength = 0;
    DWORD copied = 0;

    auto cleanup = [&]() {
        if (hash) BCryptDestroyHash(hash);
        if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    };

    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
        BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &copied, 0) < 0 ||
        BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength), &copied, 0) < 0) {
        cleanup();
        throw LicenseException("Unable to initialize device hashing.");
    }

    std::vector<unsigned char> object(objectLength);
    std::vector<unsigned char> digest(hashLength);

    if (BCryptCreateHash(algorithm, &hash, object.data(), objectLength, nullptr, 0, 0) < 0 ||
        BCryptHashData(hash, const_cast<PUCHAR>(input.data()), static_cast<ULONG>(input.size()), 0) < 0 ||
        BCryptFinishHash(hash, digest.data(), hashLength, 0) < 0) {
        cleanup();
        throw LicenseException("Unable to hash the Windows device identifier.");
    }
    cleanup();

    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const auto byte : digest) output << std::setw(2) << static_cast<int>(byte);
    return output.str();
}

} // namespace

std::string GetHashedDeviceId() {
    const std::string guid = WideToUtf8(ReadMachineGuid());
    static constexpr char kDomain[] = "NEXUS-LICENSE-V1";

    std::vector<unsigned char> material;
    material.insert(material.end(), std::begin(kDomain), std::end(kDomain) - 1);
    material.push_back(0);
    material.insert(material.end(), guid.begin(), guid.end());
    return Sha256Hex(material);
}

} // namespace nexus::license

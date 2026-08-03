#include "SecureLicenseStore.h"
#include "LicenseTypes.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dpapi.h>
#include <shlobj.h>

#include <fstream>
#include <vector>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shell32.lib")

namespace nexus::license {
namespace {

std::filesystem::path DefaultPath() {
    PWSTR rawPath = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &rawPath);
    if (FAILED(result) || rawPath == nullptr) {
        if (rawPath) CoTaskMemFree(rawPath);
        throw LicenseException("Unable to locate the local application-data folder.");
    }

    std::filesystem::path path(rawPath);
    CoTaskMemFree(rawPath);
    return path / L"HighCloudNEXUS" / L"NEXUS" / L"license.dat";
}

DATA_BLOB EntropyBlob() {
    static char entropy[] = "NEXUS-LICENSE-DPAPI-V1";
    return DATA_BLOB{
        static_cast<DWORD>(sizeof(entropy) - 1),
        reinterpret_cast<BYTE*>(entropy)};
}

} // namespace

SecureLicenseStore::SecureLicenseStore(std::filesystem::path storagePath)
    : storagePath_(storagePath.empty() ? DefaultPath() : std::move(storagePath)) {}

void SecureLicenseStore::Save(const std::string& licenseKey) const {
    if (licenseKey.empty()) throw LicenseException("Cannot securely save an empty license key.");

    DATA_BLOB input{
        static_cast<DWORD>(licenseKey.size()),
        reinterpret_cast<BYTE*>(const_cast<char*>(licenseKey.data()))};
    DATA_BLOB output{};
    DATA_BLOB entropy = EntropyBlob();

    if (!CryptProtectData(
            &input, L"NEXUS License", &entropy, nullptr, nullptr,
            CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        throw LicenseException("Windows could not protect the license key.");
    }

    try {
        std::filesystem::create_directories(storagePath_.parent_path());
        std::filesystem::path tempPath = storagePath_;
        tempPath += L".tmp";
        {
            std::ofstream stream(tempPath, std::ios::binary | std::ios::trunc);
            if (!stream) throw LicenseException("Unable to open the secure license file for writing.");
            stream.write(reinterpret_cast<const char*>(output.pbData), output.cbData);
            if (!stream) throw LicenseException("Unable to write the secure license file.");
        }
        std::error_code ec;
        std::filesystem::remove(storagePath_, ec);
        std::filesystem::rename(tempPath, storagePath_);
    } catch (...) {
        LocalFree(output.pbData);
        throw;
    }

    LocalFree(output.pbData);
}

std::optional<std::string> SecureLicenseStore::Load() const {
    if (!std::filesystem::exists(storagePath_)) return std::nullopt;

    std::ifstream stream(storagePath_, std::ios::binary | std::ios::ate);
    if (!stream) throw LicenseException("Unable to open the secure license file.");
    const auto size = stream.tellg();
    if (size <= 0 || size > 64 * 1024) throw LicenseException("The secure license file is invalid.");
    stream.seekg(0);

    std::vector<BYTE> encrypted(static_cast<std::size_t>(size));
    stream.read(reinterpret_cast<char*>(encrypted.data()), static_cast<std::streamsize>(size));
    if (!stream) throw LicenseException("Unable to read the secure license file.");

    DATA_BLOB input{static_cast<DWORD>(encrypted.size()), encrypted.data()};
    DATA_BLOB output{};
    DATA_BLOB entropy = EntropyBlob();
    if (!CryptUnprotectData(
            &input, nullptr, &entropy, nullptr, nullptr,
            CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        throw LicenseException("The saved license cannot be decrypted for this Windows user.");
    }

    std::string key(reinterpret_cast<const char*>(output.pbData), output.cbData);
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);
    if (key.empty()) throw LicenseException("The saved license is empty.");
    return key;
}

void SecureLicenseStore::Remove() const {
    std::error_code ec;
    std::filesystem::remove(storagePath_, ec);
}

const std::filesystem::path& SecureLicenseStore::Path() const noexcept {
    return storagePath_;
}

} // namespace nexus::license

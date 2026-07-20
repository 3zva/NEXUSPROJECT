#include "LicenseClient.h"
#include "DeviceId.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <winhttp.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cctype>
#include <climits>
#include <iomanip>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace nexus::license {
namespace {

struct InternetHandleCloser {
    void operator()(HINTERNET value) const noexcept {
        if (value) WinHttpCloseHandle(value);
    }
};
using InternetHandle = std::unique_ptr<std::remove_pointer_t<HINTERNET>, InternetHandleCloser>;

std::string WideToUtf8(std::wstring_view value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
        nullptr, 0, nullptr, nullptr);
    if (count <= 0) throw LicenseException("Unable to encode the license-server response.");
    std::string output(static_cast<std::size_t>(count), '\0');
    if (WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
            output.data(), count, nullptr, nullptr) <= 0) {
        throw LicenseException("Unable to encode the license-server response.");
    }
    return output;
}

std::wstring NormalizeBaseUrl(std::wstring value) {
    while (!value.empty() && value.back() == L'/') value.pop_back();
    if (value.rfind(L"https://", 0) != 0 && value.rfind(L"http://localhost", 0) != 0 &&
        value.rfind(L"http://127.0.0.1", 0) != 0) {
        throw LicenseException("The license API must use HTTPS outside local development.");
    }
    return value;
}

std::string JsonEscape(std::string_view value) {
    std::ostringstream out;
    for (const unsigned char ch : value) {
        switch (ch) {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
            } else {
                out << static_cast<char>(ch);
            }
        }
    }
    return out.str();
}

std::string RandomNonce() {
    std::array<unsigned char, 16> bytes{};
    if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
        throw LicenseException("Unable to create a secure license request.");
    }
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto byte : bytes) out << std::setw(2) << static_cast<int>(byte);
    return out.str();
}

std::string JsonStringValue(const std::string& body, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const std::size_t keyPos = body.find(needle);
    if (keyPos == std::string::npos) return {};
    std::size_t colon = body.find(':', keyPos + needle.size());
    if (colon == std::string::npos) return {};
    std::size_t quote = body.find('"', colon + 1);
    if (quote == std::string::npos) return {};
    std::string out;
    bool escaped = false;
    for (std::size_t i = quote + 1; i < body.size(); ++i) {
        const char ch = body[i];
        if (escaped) {
            switch (ch) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            default: out.push_back(ch); break;
            }
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '"') {
            return out;
        } else {
            out.push_back(ch);
        }
    }
    return {};
}

bool JsonBoolValue(const std::string& body, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const std::size_t keyPos = body.find(needle);
    if (keyPos == std::string::npos) return false;
    std::size_t colon = body.find(':', keyPos + needle.size());
    if (colon == std::string::npos) return false;
    ++colon;
    while (colon < body.size() && std::isspace(static_cast<unsigned char>(body[colon]))) ++colon;
    return body.compare(colon, 4, "true") == 0;
}

LicenseResult ParseResult(const std::string& body) {
    LicenseResult result;
    result.valid = JsonBoolValue(body, "valid");
    result.licenseType = JsonStringValue(body, "license_type");
    result.status = JsonStringValue(body, "status");
    result.activatedAt = JsonStringValue(body, "activated_at");
    result.expiresAt = JsonStringValue(body, "expires_at");
    result.serverTime = JsonStringValue(body, "server_time");
    result.validationToken = JsonStringValue(body, "validation_token");
    result.tokenExpiresAt = JsonStringValue(body, "token_expires_at");
    result.message = JsonStringValue(body, "message");
    result.code = JsonStringValue(body, "code");
    return result;
}

} // namespace

LicenseClient::LicenseClient(std::wstring apiBaseUrl, std::chrono::milliseconds timeout)
    : apiBaseUrl_(NormalizeBaseUrl(std::move(apiBaseUrl))), timeout_(timeout) {
    if (timeout_.count() <= 0) throw LicenseException("The HTTP timeout must be positive.");
}

LicenseResult LicenseClient::Activate(std::string_view licenseKey) const {
    return Request(L"/v1/license/activate", licenseKey);
}

LicenseResult LicenseClient::Validate(std::string_view licenseKey) const {
    return Request(L"/v1/license/validate", licenseKey);
}

LicenseResult LicenseClient::Request(std::wstring_view endpoint, std::string_view licenseKey) const {
    if (licenseKey.empty() || licenseKey.size() > 128) {
        throw LicenseException("Enter a valid NEXUS license key.");
    }

    const std::wstring fullUrl = apiBaseUrl_ + std::wstring(endpoint);
    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(fullUrl.c_str(), 0, 0, &components)) {
        throw LicenseException("The configured license API URL is invalid.");
    }

    const std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
    if (components.dwExtraInfoLength > 0) {
        path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }

    InternetHandle session(WinHttpOpen(
        L"NEXUS-Desktop/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));
    if (!session) throw LicenseException("Unable to initialize the Windows HTTP client.");

    const int timeoutMs = static_cast<int>(std::min<std::int64_t>(timeout_.count(), INT_MAX));
    if (!WinHttpSetTimeouts(session.get(), timeoutMs, timeoutMs, timeoutMs, timeoutMs)) {
        throw LicenseException("Unable to configure the license-server timeout.");
    }

    InternetHandle connection(WinHttpConnect(session.get(), host.c_str(), components.nPort, 0));
    if (!connection) throw LicenseException("Unable to connect to the NEXUS license server.");

    const DWORD flags = components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    InternetHandle request(WinHttpOpenRequest(
        connection.get(), L"POST", path.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!request) throw LicenseException("Unable to create the license request.");

    const std::string nonce = RandomNonce();
    const std::string body =
        "{\"license_key\":\"" + JsonEscape(licenseKey) +
        "\",\"device_id\":\"" + JsonEscape(GetHashedDeviceId()) +
        "\",\"client_version\":\"1.3.0" +
        "\",\"nonce\":\"" + nonce + "\"}";
    constexpr wchar_t headers[] = L"Content-Type: application/json\r\nAccept: application/json\r\n";

    if (!WinHttpSendRequest(
            request.get(), headers, static_cast<DWORD>(-1),
            const_cast<char*>(body.data()), static_cast<DWORD>(body.size()),
            static_cast<DWORD>(body.size()), 0) ||
        !WinHttpReceiveResponse(request.get(), nullptr)) {
        throw LicenseException("Unable to reach the NEXUS license server.");
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(
            request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX)) {
        throw LicenseException("Unable to read the license-server status.");
    }

    std::string responseBody;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &available)) {
            throw LicenseException("Unable to read the license-server response.");
        }
        if (available == 0) break;
        if (responseBody.size() + available > 64 * 1024) {
            throw LicenseException("The license-server response was unexpectedly large.");
        }
        const auto oldSize = responseBody.size();
        responseBody.resize(oldSize + available);
        DWORD read = 0;
        if (!WinHttpReadData(request.get(), responseBody.data() + oldSize, available, &read)) {
            throw LicenseException("Unable to read the license-server response.");
        }
        responseBody.resize(oldSize + read);
    }

    LicenseResult result = ParseResult(responseBody);
    if (status >= 500 && result.message.empty()) {
        throw LicenseException("The NEXUS license server is temporarily unavailable.");
    }
    return result;
}

} // namespace nexus::license

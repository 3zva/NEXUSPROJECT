#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
constexpr char kFooterMagic[] = "NEXUSPKGEND01";
constexpr char kArchiveMagic[] = "NEXUSPKG01";
constexpr std::size_t kFooterMagicSize = sizeof(kFooterMagic) - 1;
constexpr std::size_t kArchiveMagicSize = sizeof(kArchiveMagic) - 1;

std::wstring utf8ToWide(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) throw std::runtime_error("Invalid package path.");
    std::wstring output(static_cast<std::size_t>(count), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), output.data(), count) <= 0) {
        throw std::runtime_error("Invalid package path.");
    }
    return output;
}

std::string readBytes(std::ifstream& input, std::size_t count) {
    std::string value(count, '\0');
    input.read(value.data(), static_cast<std::streamsize>(count));
    if (!input) throw std::runtime_error("The NEXUS package is incomplete.");
    return value;
}

uint16_t readU16(std::ifstream& input) {
    unsigned char bytes[2]{};
    input.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
    if (!input) throw std::runtime_error("The NEXUS package is incomplete.");
    return static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
}

uint32_t readU32(std::ifstream& input) {
    unsigned char bytes[4]{};
    input.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
    if (!input) throw std::runtime_error("The NEXUS package is incomplete.");
    return static_cast<uint32_t>(bytes[0]) |
        (static_cast<uint32_t>(bytes[1]) << 8) |
        (static_cast<uint32_t>(bytes[2]) << 16) |
        (static_cast<uint32_t>(bytes[3]) << 24);
}

uint64_t readU64(std::ifstream& input) {
    unsigned char bytes[8]{};
    input.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
    if (!input) throw std::runtime_error("The NEXUS package is incomplete.");
    uint64_t value = 0;
    for (int i = 7; i >= 0; --i) {
        value = (value << 8) | bytes[i];
    }
    return value;
}

fs::path executablePath() {
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0) throw std::runtime_error("Unable to locate the launcher executable.");
        if (size < buffer.size() - 1) return fs::path(std::wstring(buffer.data(), size));
        buffer.resize(buffer.size() * 2);
    }
}

fs::path localAppDataRoot() {
    PWSTR raw = nullptr;
    const HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &raw);
    if (FAILED(hr) || raw == nullptr) {
        if (raw) CoTaskMemFree(raw);
        throw std::runtime_error("Unable to locate LocalAppData.");
    }
    fs::path path(raw);
    CoTaskMemFree(raw);
    return path / L"HighCloudNEXUS" / L"NEXUS" / L"app";
}

bool safeRelativePath(const std::string& path) {
    if (path.empty() || path.size() > 260) return false;
    if (path[0] == '/' || path[0] == '\\') return false;
    if (path.find(':') != std::string::npos) return false;
    fs::path rel = fs::path(utf8ToWide(path));
    for (const auto& part : rel) {
        if (part == L"..") return false;
    }
    return true;
}

void extractBundle(const fs::path& self, const fs::path& installRoot) {
    std::ifstream input(self, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Unable to open the NEXUS package.");
    const auto totalSize = static_cast<uint64_t>(input.tellg());
    const uint64_t footerSize = static_cast<uint64_t>(sizeof(uint64_t) + kFooterMagicSize);
    if (totalSize <= footerSize) throw std::runtime_error("This executable does not contain a NEXUS package.");

    input.seekg(static_cast<std::streamoff>(totalSize - footerSize), std::ios::beg);
    const uint64_t archiveSize = readU64(input);
    const std::string footerMagic = readBytes(input, kFooterMagicSize);
    if (footerMagic != kFooterMagic || archiveSize == 0 || archiveSize > totalSize - footerSize) {
        throw std::runtime_error("This executable does not contain a valid NEXUS package.");
    }

    const uint64_t archiveStart = totalSize - footerSize - archiveSize;
    input.seekg(static_cast<std::streamoff>(archiveStart), std::ios::beg);
    if (readBytes(input, kArchiveMagicSize) != kArchiveMagic) {
        throw std::runtime_error("The NEXUS package header is invalid.");
    }
    const uint32_t count = readU32(input);
    if (count == 0 || count > 4096) throw std::runtime_error("The NEXUS package file count is invalid.");

    fs::create_directories(installRoot);
    for (uint32_t i = 0; i < count; ++i) {
        const uint16_t pathLength = readU16(input);
        const uint64_t fileSize = readU64(input);
        const std::string relativeName = readBytes(input, pathLength);
        if (!safeRelativePath(relativeName)) throw std::runtime_error("The NEXUS package contains an unsafe path.");
        if (fileSize > 512ull * 1024ull * 1024ull) throw std::runtime_error("A packaged NEXUS file is too large.");

        const fs::path outputPath = installRoot / fs::path(utf8ToWide(relativeName));
        fs::create_directories(outputPath.parent_path());
        std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Unable to write a NEXUS application file.");

        std::vector<char> buffer(1024 * 1024);
        uint64_t remaining = fileSize;
        while (remaining > 0) {
            const std::size_t chunk = static_cast<std::size_t>(std::min<uint64_t>(remaining, buffer.size()));
            input.read(buffer.data(), static_cast<std::streamsize>(chunk));
            if (!input) throw std::runtime_error("The NEXUS package ended unexpectedly.");
            output.write(buffer.data(), static_cast<std::streamsize>(chunk));
            if (!output) throw std::runtime_error("Unable to write a NEXUS application file.");
            remaining -= chunk;
        }
    }
}

void launchNexus(const fs::path& installRoot) {
    const fs::path app = installRoot / L"Nexus Loader.exe";
    if (!fs::exists(app)) throw std::runtime_error("Nexus Loader.exe was not installed.");

    std::wstring command = L"\"" + app.wstring() + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(app.c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, installRoot.c_str(), &startup, &process)) {
        throw std::runtime_error("Unable to launch NEXUS.");
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
}

void showError(const std::string& message) {
    MessageBoxA(nullptr, message.c_str(), "NEXUS", MB_OK | MB_ICONERROR);
}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int) {
    try {
        const fs::path self = executablePath();
        const fs::path installRoot = localAppDataRoot();
        extractBundle(self, installRoot);
        if (commandLine != nullptr && std::wstring(commandLine).find(L"--extract-only") != std::wstring::npos) {
            return 0;
        }
        launchNexus(installRoot);
        return 0;
    } catch (const std::exception& error) {
        showError(error.what());
        return 1;
    }
}

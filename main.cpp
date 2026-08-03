#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <winreg.h>
#include <winhttp.h>
#include <wrl.h>
#include <tlhelp32.h>
#include <initguid.h>
#include <sapi.h>
#include "WebView2.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <iomanip>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "ole32.lib")

namespace fs = std::filesystem;

constexpr wchar_t APP_TITLE[] = L"NEXUS";
constexpr char CONFIG_PREFIX[] = "NEXUS_CONFIG_V1";
constexpr char LOCAL_CONFIG_FILENAME[] = "nexus_runtime_config.txt";
constexpr char LOCAL_HTTP_HOST[] = "127.0.0.1";
constexpr int LOCAL_HTTP_PORT = 20112;
constexpr int LOADER_HTTP_PORT = 20110;
constexpr int WS_PORTS[] = {20111, 8765, 6741};
constexpr wchar_t RUNTIME_HELPER_WORD_LIST_FILENAME[] = L"nexus_runtime_helper_words.txt";
constexpr wchar_t RUNTIME_HELPER_EXE_FILENAME[] = L"NEXUS Runtime Helper.exe";
constexpr wchar_t RUNTIME_HELPER_CONFIG_FILENAME[] = L"nexus_runtime_helper_config.txt";
constexpr wchar_t RUNTIME_SUPPORT_DIR[] = L"runtime";
constexpr wchar_t QT_CLIENT_UI_DIR[] = L"nexus-client";
constexpr wchar_t QT_CLIENT_UI_EXE[] = L"Nexus Client.exe";

HWND g_hwnd = nullptr;
HWND g_status = nullptr;
HWND g_profile = nullptr;
HWND g_pause = nullptr;
HWND g_weapon = nullptr;
HFONT g_font = nullptr;
HFONT g_titleFont = nullptr;
HFONT g_smallBoldFont = nullptr;

std::mutex g_stateMutex;
std::mutex g_configMutex;
std::mutex g_clientsMutex;
std::vector<SOCKET> g_wsClients;

double g_speed1 = 0.01;
double g_x1 = 0.0;
double g_y1 = 1.0;
double g_rampX1 = 0.0;
double g_rampY1 = 0.0;
double g_rampStart1 = 0.75;
double g_speed2 = 0.01;
double g_x2 = 0.0;
double g_y2 = 1.0;
double g_rampX2 = 0.0;
double g_rampY2 = 0.0;
double g_rampStart2 = 0.75;
int g_rapidFireEnabled = 0;
int g_weaponIndex = 1;
bool g_paused = false;
bool g_profileActive = false;
bool g_leftHeld = false;
bool g_rightHeld = false;
bool g_holding = false;
std::chrono::steady_clock::time_point g_rapidActionUntil;
std::chrono::steady_clock::time_point g_syntheticClicksUntil;
std::chrono::steady_clock::time_point g_startupTime;

char g_primaryHotkey = '1';
char g_secondaryHotkey = '2';
char g_pauseKey = 'p';
bool g_lastPrimary = false;
bool g_lastSecondary = false;
bool g_lastPause = false;

std::atomic<bool> g_running{true};
std::atomic<bool> g_runtimeStarted{false};
HHOOK g_mouseHook = nullptr;

enum class Screen {
    Login,
    Register,
    Verify,
    AccountCreated,
    Install,
    Runtime
};

Screen g_screen = Screen::Login;
std::wstring g_email;
std::wstring g_password;
std::wstring g_fullName;
std::wstring g_username;
std::wstring g_confirmPassword;
std::wstring g_installDir;
std::wstring g_authenticatedUser;
std::wstring g_pendingEmail;
std::wstring g_pendingUsername;
std::string g_firebaseApiKey;
std::string g_firebaseIdToken;
std::string g_firebaseRefreshToken;
std::string g_pendingIdToken;
std::string g_pendingRefreshToken;
bool g_remember = false;
bool g_passwordVisible = false;
bool g_registerPasswordVisible = false;
bool g_registerConfirmVisible = false;
bool g_termsAccepted = false;
bool g_clientOnlyMode = false;
bool g_backendOnlyMode = false;
std::wstring g_displayName = L"";

constexpr COLORREF COLOR_BG = RGB(7, 10, 18);
constexpr COLORREF COLOR_PANEL = RGB(14, 19, 32);
constexpr COLORREF COLOR_INPUT = RGB(17, 23, 37);
constexpr COLORREF COLOR_BORDER = RGB(43, 52, 72);
constexpr COLORREF COLOR_TEXT = RGB(247, 249, 255);
constexpr COLORREF COLOR_MUTED = RGB(152, 162, 183);
constexpr COLORREF COLOR_ACCENT = RGB(118, 91, 255);
constexpr COLORREF COLOR_DANGER = RGB(255, 107, 130);
constexpr COLORREF COLOR_SUCCESS = RGB(78, 212, 154);
HBRUSH g_bgBrush = nullptr;
HBRUSH g_panelBrush = nullptr;
HBRUSH g_inputBrush = nullptr;
ComPtr<ICoreWebView2Environment> g_webViewEnvironment;
ComPtr<ICoreWebView2Controller> g_webViewController;
ComPtr<ICoreWebView2> g_webView;

const wchar_t* MAIN_LIST_A[] = {
    L"App", L"Setup", L"Client", L"Package", L"Module", L"Bundle", L"Program", L"Software",
    L"Application", L"Component", L"Utility", L"Toolkit", L"Suite", L"Platform", L"Service",
    L"System", L"Product", L"Build", L"Release", L"Launcher", L"Framework", L"Library",
    L"Extension", L"Plugin", L"Addon", L"Feature", L"Resource", L"Archive", L"Distribution",
    L"Runtime", L"Environment", L"Workspace", L"Project", L"Solution", L"Interface", L"Console",
    L"Engine", L"Core", L"Repository", L"Deployment"
};

const wchar_t* MAIN_LIST_B[] = {
    L"Installer", L"Wizard", L"Manager", L"Assistant", L"Helper", L"Updater", L"Launcher", L"Loader",
    L"Handler", L"Controller", L"Processor", L"Coordinator", L"Agent", L"Service", L"Utility",
    L"Tool", L"Setup", L"Bootstrapper", L"Deployer", L"Configurator", L"Administrator", L"Operator",
    L"Supervisor", L"Organizer", L"Integrator", L"Migrator", L"Initializer", L"Activator", L"Extractor",
    L"Packager", L"Builder", L"Compiler", L"Generator", L"Provisioner", L"Dispatcher", L"Executor",
    L"Monitor", L"Maintainer", L"Resolver", L"Synchronizer"
};

fs::path ExeDir();
fs::path RuntimeDir();
fs::path RuntimeDirFor(const fs::path& root);
fs::path RuntimeFile(const wchar_t* filename);
fs::path RuntimeFileFor(const fs::path& root, const wchar_t* filename);
fs::path ResourceRoot();
fs::path ConfigPath();
std::string ReadFileUtf8(const fs::path& path);
std::string Trim(const std::string& value);
std::string ToLowerAscii(std::string value);
std::wstring QuoteArg(const std::wstring& value);
std::wstring PowerShellSingleQuoted(const std::wstring& value);
void LaunchHiddenUtility(const std::wstring& commandLine);
bool LaunchRainbowSixSiege(std::wstring* errorMessage = nullptr);
bool IsRainbowSixSiegeRunning();
void KillRainbowSixSiegeProcesses();
void StartRuntimeThreads();
void ShowLogin();
void ShowRegister();
void ShowVerify();
void ShowAccountCreated();
void ShowInstall();
void ShowRuntime();
void SetStatus(const std::wstring& text);
void BroadcastWebSocketText(const std::string& message);
void OpenHtmlUi();
void LoaderHttpServerThread();
void HandleLogout();
void NavigateEmbedded(const std::wstring& url);
std::wstring ClientCoreHtmlUrl();

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) return L"";
    int needed = MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), nullptr, 0);
    std::wstring out(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), (int)value.size(), out.data(), needed);
    return out;
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return "";
    int needed = WideCharToMultiByte(CP_UTF8, 0, value.data(), (int)value.size(), nullptr, 0, nullptr, nullptr);
    std::string out(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), (int)value.size(), out.data(), needed, nullptr, nullptr);
    return out;
}

namespace AudioFeedback {
class TextToSpeech {
private:
    ISpVoice* pVoice = nullptr;
    bool comInitialized = false;
    std::thread worker;
    std::mutex mutex;
    std::condition_variable changed;
    std::wstring pendingPhrase;
    std::atomic_bool speechEnabled{true};
    std::atomic_long speechVolume{80};
    bool hasPendingPhrase = false;
    bool purgeRequested = false;
    bool stopping = false;

    std::wstring ConvertToWide(const std::string& str) {
        if (str.empty()) return L"";
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
        if (sizeNeeded <= 0) return L"";
        std::wstring result(sizeNeeded, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), sizeNeeded);
        return result;
    }

    void InitializeVoice() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        comInitialized = SUCCEEDED(hr);
        if (!comInitialized) {
            return;
        }

        hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, reinterpret_cast<void**>(&pVoice));
        if (FAILED(hr)) {
            pVoice = nullptr;
        }
    }

    void ShutdownVoice() {
        if (pVoice != nullptr) {
            pVoice->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
            pVoice->Release();
            pVoice = nullptr;
        }
        if (comInitialized) {
            CoUninitialize();
            comInitialized = false;
        }
    }

    void Run() {
        InitializeVoice();
        while (true) {
            std::wstring phrase;
            bool shouldPurge = false;
            {
                std::unique_lock<std::mutex> lock(mutex);
                changed.wait(lock, [this] { return stopping || hasPendingPhrase || purgeRequested; });
                if (stopping) {
                    break;
                }
                shouldPurge = purgeRequested;
                purgeRequested = false;
                phrase = std::move(pendingPhrase);
                pendingPhrase.clear();
                hasPendingPhrase = false;
            }

            if (pVoice == nullptr) {
                continue;
            }

            pVoice->SetVolume(static_cast<USHORT>(std::clamp<long>(speechVolume.load(), 0, 100)));
            if (shouldPurge) {
                pVoice->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
            }
            if (speechEnabled.load() && !phrase.empty()) {
                pVoice->Speak(phrase.c_str(), SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
            }
        }
        ShutdownVoice();
    }

public:
    TextToSpeech()
        : worker(&TextToSpeech::Run, this) {}

    ~TextToSpeech() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping = true;
        }
        changed.notify_one();
        if (worker.joinable()) {
            worker.join();
        }
    }

    TextToSpeech(const TextToSpeech&) = delete;
    TextToSpeech& operator=(const TextToSpeech&) = delete;

    void SetEnabled(bool enabled) {
        speechEnabled.store(enabled);
        if (!enabled) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                pendingPhrase.clear();
                hasPendingPhrase = false;
                purgeRequested = true;
            }
            changed.notify_one();
        }
    }

    void SetVolume(int volume) {
        speechVolume.store(std::clamp(volume, 0, 100));
    }

    void TriggerLoadoutSpeech(const std::string& op, const std::string& primary, const std::string& secondary) {
        if (!speechEnabled.load()) {
            return;
        }

        std::wstring phrase = L"Operator " + ConvertToWide(op) + L" loaded.";
        if (!primary.empty()) {
            phrase += L" " + ConvertToWide(primary) + L".";
        }
        if (!secondary.empty()) {
            phrase += L" " + ConvertToWide(secondary) + L".";
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            pendingPhrase = std::move(phrase);
            hasPendingPhrase = true;
        }
        changed.notify_one();
    }
};

TextToSpeech& LoadoutSpeechClient() {
    static TextToSpeech client;
    return client;
}
}

std::wstring GetWindowTextString(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    std::wstring value(len, L'\0');
    if (len) GetWindowTextW(hwnd, value.data(), len + 1);
    return value;
}

void SetControlFont(HWND hwnd, HFONT font = nullptr) {
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)(font ? font : g_font), TRUE);
}

fs::path LocalAppDataPath() {
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)) && path) {
        fs::path out(path);
        CoTaskMemFree(path);
        return out;
    }
    return ExeDir();
}

fs::path CurrentExePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return fs::path(path);
}

std::wstring CurrentExeDisplayName() {
    std::wstring stem = CurrentExePath().stem().wstring();
    return stem.empty() ? L"NEXUS" : stem;
}

fs::path NexusDataDir() {
    return LocalAppDataPath() / L"NexusLoader";
}

fs::path SessionPath() {
    return NexusDataDir() / L"session.json";
}

fs::path SettingsPath() {
    return NexusDataDir() / L"settings.json";
}

std::wstring ToLowerWide(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) { return (wchar_t)towlower(ch); });
    return value;
}

std::string JsonEscape(const std::string& value) {
    std::string out;
    for (char ch : value) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(ch); break;
        }
    }
    return out;
}

std::string JsonStringValue(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos);
    if (pos == std::string::npos) return "";
    std::string out;
    bool escape = false;
    for (++pos; pos < json.size(); ++pos) {
        char ch = json[pos];
        if (escape) {
            if (ch == 'n') out.push_back('\n');
            else if (ch == 'r') out.push_back('\r');
            else if (ch == 't') out.push_back('\t');
            else out.push_back(ch);
            escape = false;
        } else if (ch == '\\') {
            escape = true;
        } else if (ch == '"') {
            break;
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

bool JsonBoolValue(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return false;
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) ++pos;
    return json.rfind("true", pos) == pos;
}

std::wstring DefaultInstallDir() {
    return (ExeDir() / L"installed").wstring();
}

std::wstring ChooseVisibleExeName() {
    static std::mt19937 rng((unsigned)GetTickCount() ^ (unsigned)std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<size_t> left(0, (sizeof(MAIN_LIST_A) / sizeof(MAIN_LIST_A[0])) - 1);
    std::uniform_int_distribution<size_t> right(0, (sizeof(MAIN_LIST_B) / sizeof(MAIN_LIST_B[0])) - 1);
    return std::wstring(MAIN_LIST_A[left(rng)]) + MAIN_LIST_B[right(rng)] + L".exe";
}

std::vector<std::wstring> ParseRuntimeHelperWords(const std::string& text, const std::string& beginMarker, const std::string& endMarker) {
    std::vector<std::wstring> words;
    size_t begin = text.find(beginMarker);
    if (begin == std::string::npos) return words;
    begin = text.find('.', begin + beginMarker.size());
    begin = begin == std::string::npos ? 0 : begin + 1;
    size_t end = endMarker.empty() ? text.size() : text.find(endMarker, begin);
    if (end == std::string::npos) end = text.size();

    std::string current;
    for (size_t i = begin; i < end; ++i) {
        unsigned char ch = static_cast<unsigned char>(text[i]);
        if (std::isalnum(ch)) {
            current.push_back(static_cast<char>(ch));
        } else if (!current.empty()) {
            words.push_back(Utf8ToWide(current));
            current.clear();
        }
    }
    if (!current.empty()) words.push_back(Utf8ToWide(current));
    return words;
}

std::wstring ChooseRuntimeHelperExeName() {
    const fs::path wordList = RuntimeFile(RUNTIME_HELPER_WORD_LIST_FILENAME);
    std::vector<std::wstring> listA = ParseRuntimeHelperWords(ReadFileUtf8(wordList), "List A", "List B");
    std::vector<std::wstring> listB = ParseRuntimeHelperWords(ReadFileUtf8(wordList), "List B", "");
    if (listA.empty() || listB.empty()) {
        throw std::runtime_error("NEXUS runtime helper word list is missing or invalid.");
    }

    static std::mt19937 rng((unsigned)GetTickCount() ^ (unsigned)std::chrono::steady_clock::now().time_since_epoch().count() ^ 0x5A17C0DEu);
    std::uniform_int_distribution<size_t> left(0, listA.size() - 1);
    std::uniform_int_distribution<size_t> right(0, listB.size() - 1);
    return listA[left(rng)] + listB[right(rng)] + L".exe";
}

std::wstring QuoteArg(const std::wstring& value) {
    std::wstring out = L"\"";
    for (wchar_t ch : value) {
        if (ch == L'"') out += L"\\\"";
        else out.push_back(ch);
    }
    out += L"\"";
    return out;
}

std::string ConfigValueLine(const std::string& text, const std::string& key) {
    std::istringstream lines(text);
    std::string line;
    std::string prefix = ToLowerAscii(key) + "=";
    while (std::getline(lines, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;
        std::string lower = ToLowerAscii(line);
        if (lower.rfind(prefix, 0) == 0) return Trim(line.substr(prefix.size()));
    }
    return "";
}

bool IsInsidePath(const fs::path& path, const fs::path& parent) {
    std::error_code ec;
    fs::path child = fs::weakly_canonical(path, ec);
    fs::path root = fs::weakly_canonical(parent, ec);
    std::wstring childText = child.wstring();
    std::wstring rootText = root.wstring();
    if (!rootText.empty() && rootText.back() != L'\\') rootText.push_back(L'\\');
    return childText.rfind(rootText, 0) == 0;
}

std::vector<std::wstring> GeneratedVisibleExeNames() {
    std::vector<std::wstring> names;
    names.reserve((sizeof(MAIN_LIST_A) / sizeof(MAIN_LIST_A[0])) * (sizeof(MAIN_LIST_B) / sizeof(MAIN_LIST_B[0])));
    for (const wchar_t* left : MAIN_LIST_A) {
        for (const wchar_t* right : MAIN_LIST_B) {
            names.push_back(std::wstring(left) + right + L".exe");
        }
    }
    return names;
}

void StopProcessByExecutablePath(const fs::path& executablePath) {
    if (executablePath.empty()) {
        return;
    }
    LaunchHiddenUtility(
        L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "
        L"\"$target=" + PowerShellSingleQuoted(executablePath.wstring()) + L"; "
        L"Get-CimInstance Win32_Process | "
        L"Where-Object { $_.ExecutablePath -eq $target } | "
        L"ForEach-Object { Invoke-CimMethod -InputObject $_ -MethodName Terminate | Out-Null }\""
    );
}

void RemoveCopiedExecutable(const fs::path& executablePath, const fs::path& installDir) {
    std::error_code ec;
    if (executablePath.empty() || !IsInsidePath(executablePath, installDir) || !fs::exists(executablePath, ec)) {
        return;
    }

    StopProcessByExecutablePath(executablePath);
    for (int attempt = 0; attempt < 8 && fs::exists(executablePath, ec); ++attempt) {
        fs::remove(executablePath, ec);
        if (!fs::exists(executablePath, ec)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(125));
    }
}

void CleanupPreviousInstall(const fs::path& installDir) {
    std::error_code ec;
    fs::path marker = installDir / L"current_install.txt";
    std::wstring previous = Utf8ToWide(Trim(ReadFileUtf8(marker)));
    if (!previous.empty()) {
        fs::path previousPath(previous);
        RemoveCopiedExecutable(previousPath, installDir);
        const fs::path previousBackend = installDir / (previousPath.stem().wstring() + L" Runtime.exe");
        RemoveCopiedExecutable(previousBackend, installDir);
    }

    const fs::path nexusUiDir = installDir / QT_CLIENT_UI_DIR;
    for (const std::wstring& visibleName : GeneratedVisibleExeNames()) {
        const fs::path visibleCopy = nexusUiDir / visibleName;
        const fs::path backendCopy = installDir / (fs::path(visibleName).stem().wstring() + L" Runtime.exe");
        RemoveCopiedExecutable(visibleCopy, installDir);
        RemoveCopiedExecutable(backendCopy, installDir);
    }

    fs::remove(marker, ec);
}

void LaunchHiddenUtility(const std::wstring& commandLine) {
    std::wstring mutableCommand = commandLine;
    STARTUPINFOW si{sizeof(si)};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    if (CreateProcessW(
            nullptr,
            mutableCommand.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi)) {
        WaitForSingleObject(pi.hProcess, 3000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

std::wstring PowerShellSingleQuoted(const std::wstring& value) {
    std::wstring escaped = L"'";
    for (wchar_t ch : value) {
        if (ch == L'\'') escaped += L"''";
        else escaped.push_back(ch);
    }
    escaped += L"'";
    return escaped;
}

void StopNexusRuntimeHelperProcesses(const fs::path& installDir = fs::path()) {
    std::wstring executablePathClause;
    if (!installDir.empty()) {
        fs::path runtimeDir = RuntimeDirFor(installDir);
        for (const fs::path& pidMarker : {runtimeDir / L"current_runtime_helper_pid.txt", installDir / L"current_runtime_helper_pid.txt"}) {
            std::string pidText = Trim(ReadFileUtf8(pidMarker));
            if (!pidText.empty()) {
                LaunchHiddenUtility(L"taskkill.exe /PID " + Utf8ToWide(pidText) + L" /F /T");
            }
        }

        for (const fs::path& marker : {runtimeDir / L"current_runtime_helper.txt", installDir / L"current_runtime_helper.txt"}) {
            std::wstring previous = Utf8ToWide(Trim(ReadFileUtf8(marker)));
            if (!previous.empty()) {
                fs::path previousPath(previous);
                std::wstring imageName = previousPath.filename().wstring();
                if (!imageName.empty()) {
                    LaunchHiddenUtility(L"taskkill.exe /IM " + QuoteArg(imageName) + L" /F");
                }
                executablePathClause += L" -or ($_.ExecutablePath -eq " + PowerShellSingleQuoted(previousPath.wstring()) + L")";
            }
        }
    }

    std::wstring command =
        L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "
        L"\"$self=$PID; Get-CimInstance Win32_Process | Where-Object { "
        L"($_.ProcessId -ne $self) -and ("
        L"($_.ExecutablePath -like '*NEXUS Runtime Helper.exe') -or "
        L"($_.CommandLine -like '*nexus_runtime_helper_config.txt*')";
    command += executablePathClause;
    command += L") } | ForEach-Object { Invoke-CimMethod -InputObject $_ -MethodName Terminate | Out-Null }\"";

    LaunchHiddenUtility(command);
}

void CleanupPreviousRuntimeHelper(const fs::path& installDir) {
    StopNexusRuntimeHelperProcesses(installDir);
    fs::path runtimeDir = RuntimeDirFor(installDir);
    fs::path marker = runtimeDir / L"current_runtime_helper.txt";
    std::wstring previous = Utf8ToWide(Trim(ReadFileUtf8(marker)));
    std::error_code ec;
    if (!previous.empty()) {
        fs::path previousPath(previous);
        if (fs::exists(previousPath, ec) && IsInsidePath(previousPath, runtimeDir)) {
            fs::remove(previousPath, ec);
        }
    }
    fs::remove(marker, ec);
    fs::remove(runtimeDir / L"current_runtime_helper_pid.txt", ec);
    fs::remove(installDir / L"current_runtime_helper.txt", ec);
    fs::remove(installDir / L"current_runtime_helper_pid.txt", ec);
}

void CopyRuntimeResources(const fs::path& installDir) {
    std::error_code ec;
    fs::path runtimeDir = RuntimeDirFor(installDir);
    fs::create_directories(runtimeDir, ec);
    fs::create_directories(installDir / L"nexus-runtime-core", ec);
    fs::create_directories(installDir / L"configs", ec);
    fs::create_directories(installDir / L"nexus-ui", ec);
    fs::create_directories(installDir / QT_CLIENT_UI_DIR, ec);

    if (fs::exists(RuntimeFile(L"WebView2Loader.dll"))) {
        fs::copy_file(RuntimeFile(L"WebView2Loader.dll"), runtimeDir / L"WebView2Loader.dll", fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(RuntimeFile(L"firebase_config.json"))) {
        fs::copy_file(RuntimeFile(L"firebase_config.json"), runtimeDir / L"firebase_config.json", fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(RuntimeFile(L"nexus.ico"))) {
        fs::copy_file(RuntimeFile(L"nexus.ico"), runtimeDir / L"nexus.ico", fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(ExeDir() / L"nexus-ui")) {
        fs::copy(ExeDir() / L"nexus-ui", installDir / L"nexus-ui", fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(ExeDir() / QT_CLIENT_UI_DIR)) {
        fs::copy(ExeDir() / QT_CLIENT_UI_DIR, installDir / QT_CLIENT_UI_DIR, fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(ResourceRoot() / L"NexusRuntimeCore.html")) {
        fs::copy_file(ResourceRoot() / L"NexusRuntimeCore.html", installDir / L"nexus-runtime-core" / L"NexusRuntimeCore.html", fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(ResourceRoot() / L"associated_icon.png")) {
        fs::copy_file(ResourceRoot() / L"associated_icon.png", installDir / L"nexus-runtime-core" / L"associated_icon.png", fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(ResourceRoot() / L"assets")) {
        fs::copy(ResourceRoot() / L"assets", installDir / L"nexus-runtime-core" / L"assets", fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(ConfigPath())) {
        fs::copy_file(ConfigPath(), installDir / L"configs" / LOCAL_CONFIG_FILENAME, fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(RuntimeFile(RUNTIME_HELPER_EXE_FILENAME))) {
        fs::copy_file(RuntimeFile(RUNTIME_HELPER_EXE_FILENAME), runtimeDir / RUNTIME_HELPER_EXE_FILENAME, fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(RuntimeFile(RUNTIME_HELPER_WORD_LIST_FILENAME))) {
        fs::copy_file(RuntimeFile(RUNTIME_HELPER_WORD_LIST_FILENAME), runtimeDir / RUNTIME_HELPER_WORD_LIST_FILENAME, fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(RuntimeFile(RUNTIME_HELPER_CONFIG_FILENAME)) && !fs::exists(runtimeDir / RUNTIME_HELPER_CONFIG_FILENAME, ec)) {
        fs::copy_file(RuntimeFile(RUNTIME_HELPER_CONFIG_FILENAME), runtimeDir / RUNTIME_HELPER_CONFIG_FILENAME, fs::copy_options::overwrite_existing, ec);
    }
}

std::vector<wchar_t> EnvironmentWithPrependedPath(const std::wstring& pathPrefix) {
    std::vector<std::wstring> entries;
    bool replacedPath = false;
    LPWCH block = GetEnvironmentStringsW();
    if (block) {
        for (LPWCH cursor = block; *cursor; cursor += wcslen(cursor) + 1) {
            std::wstring entry(cursor);
            if (_wcsnicmp(entry.c_str(), L"PATH=", 5) == 0) {
                entries.push_back(L"PATH=" + pathPrefix + L";" + entry.substr(5));
                replacedPath = true;
            } else {
                entries.push_back(entry);
            }
        }
        FreeEnvironmentStringsW(block);
    }
    if (!replacedPath) {
        entries.push_back(L"PATH=" + pathPrefix);
    }

    std::vector<wchar_t> out;
    for (const auto& entry : entries) {
        out.insert(out.end(), entry.begin(), entry.end());
        out.push_back(L'\0');
    }
    out.push_back(L'\0');
    return out;
}

DWORD LaunchProcess(const fs::path& exePath, const std::wstring& arguments, const fs::path& workingDir, const std::wstring& pathPrefix = L"", DWORD extraCreationFlags = 0) {
    std::wstring command = L"\"" + exePath.wstring() + L"\"" + (arguments.empty() ? L"" : L" " + arguments);
    std::wstring mutableCommand = command;
    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> environment;
    LPVOID environmentBlock = nullptr;
    if (!pathPrefix.empty()) {
        environment = EnvironmentWithPrependedPath(pathPrefix);
        environmentBlock = environment.data();
    }

    DWORD creationFlags = extraCreationFlags | (pathPrefix.empty() ? 0 : CREATE_UNICODE_ENVIRONMENT);
    BOOL ok = CreateProcessW(
        nullptr,
        mutableCommand.data(),
        nullptr,
        nullptr,
        FALSE,
        creationFlags,
        environmentBlock,
        workingDir.c_str(),
        &si,
        &pi);
    if (!ok) throw std::runtime_error("Unable to launch " + exePath.filename().string() + ".");
    DWORD processId = pi.dwProcessId;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return processId;
}

std::wstring RuntimeHelperLaunchArguments(const fs::path& installDir) {
    return L"--config " + QuoteArg((RuntimeDirFor(installDir) / RUNTIME_HELPER_CONFIG_FILENAME).wstring());
}

fs::path InstallAndLaunchRuntimeHelperCopy(const fs::path& installDir) {
    fs::path runtimeDir = RuntimeDirFor(installDir);
    fs::path sourceRuntimeHelper = RuntimeFileFor(installDir, RUNTIME_HELPER_EXE_FILENAME);
    std::error_code ec;
    if (!fs::exists(sourceRuntimeHelper, ec)) {
        throw std::runtime_error("NEXUS runtime helper is missing from the build output.");
    }

    CleanupPreviousRuntimeHelper(installDir);
    fs::create_directories(runtimeDir, ec);
    fs::path runtimeHelperExe = runtimeDir / ChooseRuntimeHelperExeName();
    fs::copy_file(sourceRuntimeHelper, runtimeHelperExe, fs::copy_options::overwrite_existing, ec);
    if (ec) throw std::runtime_error("Unable to create the runtime helper executable copy.");

    std::ofstream(runtimeDir / L"current_runtime_helper.txt", std::ios::trunc) << runtimeHelperExe.string();
    std::ofstream(installDir / L"install.log", std::ios::app) << "Installed and launched runtime helper " << runtimeHelperExe.string() << "\n";
    DWORD runtimeHelperPid = LaunchProcess(runtimeHelperExe, RuntimeHelperLaunchArguments(installDir), runtimeDir, L"C:\\msys64\\mingw64\\bin");
    std::ofstream(runtimeDir / L"current_runtime_helper_pid.txt", std::ios::trunc) << runtimeHelperPid;
    return runtimeHelperExe;
}

fs::path InstallAndLaunchClientCopy(const std::wstring& selectedDir) {
    if (selectedDir.empty()) throw std::runtime_error("Select an installation folder first.");
    fs::path installDir = fs::path(selectedDir);
    std::error_code ec;
    fs::create_directories(installDir, ec);
    if (ec) throw std::runtime_error("Unable to create the selected installation folder.");
    CleanupPreviousInstall(installDir);
    CleanupPreviousRuntimeHelper(installDir);
    CopyRuntimeResources(installDir);

    const std::wstring visibleExeName = ChooseVisibleExeName();
    fs::path sourceExe = CurrentExePath();
    fs::path backendExe = installDir / (fs::path(visibleExeName).stem().wstring() + L" Runtime.exe");
    fs::copy_file(sourceExe, backendExe, fs::copy_options::overwrite_existing, ec);
    if (ec) throw std::runtime_error("Unable to create the runtime executable copy.");

    fs::path sourceQtExe = installDir / QT_CLIENT_UI_DIR / QT_CLIENT_UI_EXE;
    if (!fs::exists(sourceQtExe, ec)) {
        throw std::runtime_error("The native NEXUS client is missing from the build output.");
    }
    fs::path installedExe = installDir / QT_CLIENT_UI_DIR / visibleExeName;
    fs::copy_file(sourceQtExe, installedExe, fs::copy_options::overwrite_existing, ec);
    if (ec) throw std::runtime_error("Unable to create the visible NEXUS client executable copy.");

    std::ofstream(installDir / L"install.log", std::ios::app)
        << "Installed backend " << backendExe.string() << "\n"
        << "Installed and launched GUI " << installedExe.string() << "\n";

    LaunchProcess(backendExe, L"--backend-only", installDir);
    LaunchProcess(
        installedExe,
        L"--trusted-loader-handoff --install-path " + QuoteArg(installDir.wstring())
            + L" --authenticated-email " + QuoteArg(g_authenticatedUser)
            + L" --window-title " + QuoteArg(fs::path(visibleExeName).stem().wstring()),
        installedExe.parent_path(),
        installedExe.parent_path().wstring() + L";C:\\msys64\\mingw64\\bin"
    );
    fs::remove(installDir / L"current_install.txt", ec);
    return installedExe;
}

std::wstring EnvironmentValue(const wchar_t* name) {
    DWORD needed = GetEnvironmentVariableW(name, nullptr, 0);
    if (needed == 0) {
        return L"";
    }
    std::wstring value(needed, L'\0');
    DWORD written = GetEnvironmentVariableW(name, value.data(), needed);
    if (written == 0) {
        return L"";
    }
    value.resize(written);
    return value;
}

void AddUniquePath(std::vector<fs::path>& paths, const fs::path& path) {
    if (path.empty()) {
        return;
    }
    const std::wstring incoming = ToLowerWide(path.lexically_normal().wstring());
    for (const fs::path& existing : paths) {
        if (ToLowerWide(existing.lexically_normal().wstring()) == incoming) {
            return;
        }
    }
    paths.push_back(path);
}

std::vector<fs::path> RegistryStringValues(HKEY hive, const wchar_t* subkey, const wchar_t* valueName) {
    std::vector<fs::path> values;
    for (REGSAM view : {KEY_WOW64_64KEY, KEY_WOW64_32KEY}) {
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, subkey, 0, KEY_READ | view, &key) != ERROR_SUCCESS) {
            continue;
        }

        DWORD type = 0;
        DWORD bytes = 0;
        if (RegQueryValueExW(key, valueName, nullptr, &type, nullptr, &bytes) == ERROR_SUCCESS
            && (type == REG_SZ || type == REG_EXPAND_SZ)
            && bytes > sizeof(wchar_t)) {
            std::wstring value(bytes / sizeof(wchar_t), L'\0');
            if (RegQueryValueExW(key, valueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(value.data()), &bytes) == ERROR_SUCCESS) {
                value.resize(wcsnlen(value.c_str(), value.size()));
                if (type == REG_EXPAND_SZ) {
                    wchar_t expanded[MAX_PATH * 4] = L"";
                    if (ExpandEnvironmentStringsW(value.c_str(), expanded, (DWORD)std::size(expanded)) > 0) {
                        value = expanded;
                    }
                }
                AddUniquePath(values, value);
            }
        }
        RegCloseKey(key);
    }
    return values;
}

std::vector<fs::path> DriveRoots() {
    std::vector<fs::path> roots;
    DWORD length = GetLogicalDriveStringsW(0, nullptr);
    if (length == 0) {
        return roots;
    }

    std::wstring buffer(length + 1, L'\0');
    if (GetLogicalDriveStringsW(length, buffer.data()) == 0) {
        return roots;
    }

    const wchar_t* cursor = buffer.c_str();
    while (*cursor != L'\0') {
        fs::path root(cursor);
        const UINT type = GetDriveTypeW(cursor);
        if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
            AddUniquePath(roots, root);
        }
        cursor += wcslen(cursor) + 1;
    }
    return roots;
}

std::wstring SteamVdfUnescape(std::wstring value) {
    std::wstring out;
    out.reserve(value.size());
    bool escaped = false;
    for (wchar_t ch : value) {
        if (escaped) {
            out.push_back(ch);
            escaped = false;
        } else if (ch == L'\\') {
            escaped = true;
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

std::vector<fs::path> SteamLibrariesFromVdf(const fs::path& steamRoot) {
    std::vector<fs::path> libraries;
    AddUniquePath(libraries, steamRoot);

    const fs::path vdfPath = steamRoot / L"steamapps" / L"libraryfolders.vdf";
    const std::wstring text = Utf8ToWide(ReadFileUtf8(vdfPath));
    size_t pos = 0;
    while ((pos = text.find(L"\"path\"", pos)) != std::wstring::npos) {
        pos = text.find(L'"', pos + 6);
        if (pos == std::wstring::npos) {
            break;
        }
        const size_t begin = pos + 1;
        const size_t end = text.find(L'"', begin);
        if (end == std::wstring::npos) {
            break;
        }
        AddUniquePath(libraries, SteamVdfUnescape(text.substr(begin, end - begin)));
        pos = end + 1;
    }
    return libraries;
}

std::vector<fs::path> FindRainbowSixSiegeExecutables() {
    constexpr wchar_t gameDir[] = L"Tom Clancy's Rainbow Six Siege";
    std::vector<fs::path> candidates;
    auto addExe = [&candidates](const fs::path& path) {
        std::error_code ec;
        if (fs::exists(path, ec) && fs::is_regular_file(path, ec)) {
            AddUniquePath(candidates, path);
        }
    };

    std::vector<fs::path> steamRoots;
    for (const auto& path : RegistryStringValues(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath")) {
        AddUniquePath(steamRoots, path);
    }
    for (const auto& path : RegistryStringValues(HKEY_LOCAL_MACHINE, L"Software\\Valve\\Steam", L"InstallPath")) {
        AddUniquePath(steamRoots, path);
    }
    const std::wstring programFilesX86 = EnvironmentValue(L"ProgramFiles(x86)");
    const std::wstring programFiles = EnvironmentValue(L"ProgramFiles");
    if (!programFilesX86.empty()) {
        AddUniquePath(steamRoots, fs::path(programFilesX86) / L"Steam");
    }
    if (!programFiles.empty()) {
        AddUniquePath(steamRoots, fs::path(programFiles) / L"Steam");
    }
    for (const fs::path& drive : DriveRoots()) {
        AddUniquePath(steamRoots, drive / L"Steam");
        AddUniquePath(steamRoots, drive / L"SteamLibrary");
        AddUniquePath(steamRoots, drive / L"Games" / L"Steam");
        AddUniquePath(steamRoots, drive / L"Games" / L"SteamLibrary");
        AddUniquePath(steamRoots, drive / L"Program Files" / L"Steam");
        AddUniquePath(steamRoots, drive / L"Program Files (x86)" / L"Steam");
    }

    std::vector<fs::path> steamLibraries;
    for (const fs::path& root : steamRoots) {
        for (const fs::path& library : SteamLibrariesFromVdf(root)) {
            AddUniquePath(steamLibraries, library);
        }
    }
    for (const fs::path& library : steamLibraries) {
        addExe(library / L"steamapps" / L"common" / gameDir / L"RainbowSix.exe");
        addExe(library / L"steamapps" / L"common" / gameDir / L"RainbowSix_BE.exe");
    }

    std::vector<fs::path> ubisoftRoots;
    for (const auto& path : RegistryStringValues(HKEY_LOCAL_MACHINE, L"Software\\Ubisoft\\Launcher", L"InstallDir")) {
        AddUniquePath(ubisoftRoots, path);
    }
    for (const auto& path : RegistryStringValues(HKEY_LOCAL_MACHINE, L"Software\\WOW6432Node\\Ubisoft\\Launcher", L"InstallDir")) {
        AddUniquePath(ubisoftRoots, path);
    }
    if (!programFilesX86.empty()) {
        AddUniquePath(ubisoftRoots, fs::path(programFilesX86) / L"Ubisoft" / L"Ubisoft Game Launcher");
    }
    if (!programFiles.empty()) {
        AddUniquePath(ubisoftRoots, fs::path(programFiles) / L"Ubisoft" / L"Ubisoft Game Launcher");
    }
    for (const fs::path& drive : DriveRoots()) {
        AddUniquePath(ubisoftRoots, drive / L"Ubisoft" / L"Ubisoft Game Launcher");
        AddUniquePath(ubisoftRoots, drive / L"Ubisoft Game Launcher");
        AddUniquePath(ubisoftRoots, drive / L"Games" / L"Ubisoft" / L"Ubisoft Game Launcher");
        AddUniquePath(ubisoftRoots, drive / L"Program Files" / L"Ubisoft" / L"Ubisoft Game Launcher");
        AddUniquePath(ubisoftRoots, drive / L"Program Files (x86)" / L"Ubisoft" / L"Ubisoft Game Launcher");
    }
    for (const fs::path& root : ubisoftRoots) {
        addExe(root / L"games" / gameDir / L"RainbowSix.exe");
        addExe(root / L"games" / gameDir / L"RainbowSix_BE.exe");
    }

    return candidates;
}

bool LaunchRainbowSixSiege(std::wstring* errorMessage) {
    const std::vector<fs::path> executables = FindRainbowSixSiegeExecutables();
    if (executables.empty()) {
        if (errorMessage != nullptr) {
            *errorMessage = L"Rainbow Six Siege was not found in installed Steam or Ubisoft libraries on this computer.";
        }
        return false;
    }

    for (const fs::path& executable : executables) {
        HINSTANCE result = ShellExecuteW(
            nullptr,
            L"open",
            executable.c_str(),
            nullptr,
            executable.parent_path().c_str(),
            SW_SHOWNORMAL
        );
        if (reinterpret_cast<intptr_t>(result) > 32) {
            return true;
        }
    }

    if (errorMessage != nullptr) {
        *errorMessage = L"Rainbow Six Siege was found, but Windows could not launch it.";
    }
    return false;
}

std::vector<std::wstring> RainbowSixProcessNames() {
    return {
        L"RainbowSix.exe",
        L"RainbowSix_BE.exe",
        L"RainbowSix_Vulkan.exe",
        L"RainbowSix_DX11.exe",
        L"RainbowSixHelper.exe",
    };
}

bool IsRainbowSixSiegeRunning() {
    const std::vector<std::wstring> names = RainbowSixProcessNames();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            const std::wstring processName = ToLowerWide(entry.szExeFile);
            for (const std::wstring& name : names) {
                if (processName == ToLowerWide(name)) {
                    found = true;
                    break;
                }
            }
        } while (!found && Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

void KillRainbowSixSiegeProcesses() {
    for (const std::wstring& name : RainbowSixProcessNames()) {
        LaunchHiddenUtility(L"taskkill.exe /IM " + QuoteArg(name) + L" /F /T");
    }
}

void SaveLastInstallPath(const std::wstring& path) {
    if (path.empty()) return;
    fs::create_directories(NexusDataDir());
    std::ofstream out(SettingsPath(), std::ios::binary | std::ios::trunc);
    out << "{\n  \"lastInstallPath\": \"" << JsonEscape(WideToUtf8(path)) << "\"\n}\n";
}

std::wstring LoadLastInstallPath() {
    return Utf8ToWide(JsonStringValue(ReadFileUtf8(SettingsPath()), "lastInstallPath"));
}

void SaveSession(const std::string& refreshToken, const std::wstring& email, bool remember) {
    fs::create_directories(NexusDataDir());
    std::ofstream out(SessionPath(), std::ios::binary | std::ios::trunc);
    out << "{\n  \"remember\": " << (remember ? "true" : "false") << ",\n"
        << "  \"email\": \"" << JsonEscape(WideToUtf8(email)) << "\",\n"
        << "  \"refreshToken\": \"" << JsonEscape(refreshToken) << "\"\n}\n";
}

void ClearSavedSession() {
    std::error_code ec;
    fs::remove(SessionPath(), ec);
}

std::string WinHttpPost(const std::wstring& host, const std::wstring& path, const std::string& body, const std::wstring& contentType, DWORD* statusOut = nullptr) {
    std::string result;
    HINTERNET session = WinHttpOpen(L"NEXUS/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return "";
    HINTERNET connect = WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connect) {
        WinHttpCloseHandle(session);
        return "";
    }
    HINTERNET request = WinHttpOpenRequest(connect, L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return "";
    }
    std::wstring headers = L"Content-Type: " + contentType + L"\r\n";
    BOOL ok = WinHttpSendRequest(request, headers.c_str(), (DWORD)-1L, (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0)
        && WinHttpReceiveResponse(request, nullptr);
    if (ok) {
        DWORD status = 0, statusSize = sizeof(status);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &statusSize, nullptr);
        if (statusOut) *statusOut = status;
        DWORD available = 0;
        while (WinHttpQueryDataAvailable(request, &available) && available) {
            std::string chunk(available, '\0');
            DWORD read = 0;
            if (!WinHttpReadData(request, chunk.data(), available, &read)) break;
            chunk.resize(read);
            result += chunk;
        }
    }
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return result;
}

void LoadFirebaseConfig() {
    fs::path configPath = RuntimeFile(L"firebase_config.json");
    if (!fs::exists(configPath)) configPath = ExeDir() / L"loader-assets" / L"firebase_config.json";
    g_firebaseApiKey = JsonStringValue(ReadFileUtf8(configPath), "apiKey");
}

std::string FirebaseErrorMessage(const std::string& body) {
    std::string code = JsonStringValue(body, "message");
    if (code == "EMAIL_EXISTS") return "That email address is already registered.";
    if (code == "INVALID_EMAIL") return "Enter a valid email address.";
    if (code == "INVALID_LOGIN_CREDENTIALS" || code == "EMAIL_NOT_FOUND" || code == "INVALID_PASSWORD") return "Incorrect email or password.";
    if (code == "USER_DISABLED") return "This account has been disabled.";
    if (code.find("WEAK_PASSWORD") != std::string::npos) return "Use a stronger password.";
    if (code == "TOO_MANY_ATTEMPTS_TRY_LATER") return "Too many attempts. Try again later.";
    if (code == "INVALID_ID_TOKEN") return "Your verification session expired. Sign in or create the account again.";
    if (code.empty()) return "Firebase authentication failed.";
    std::replace(code.begin(), code.end(), '_', ' ');
    return code;
}

std::string FirebaseRequest(const std::string& endpoint, const std::string& payload, bool* ok = nullptr) {
    if (g_firebaseApiKey.empty()) {
        if (ok) *ok = false;
        return "{\"error\":{\"message\":\"Add your Firebase Web API key to firebase_config.json.\"}}";
    }
    std::wstring path = Utf8ToWide("/v1/" + endpoint + "?key=" + g_firebaseApiKey);
    DWORD status = 0;
    std::string body = WinHttpPost(L"identitytoolkit.googleapis.com", path, payload, L"application/json", &status);
    bool success = status >= 200 && status < 300 && body.find("\"error\"") == std::string::npos;
    if (ok) *ok = success;
    return body;
}

std::string FirebaseRefresh(const std::string& refreshToken, bool* ok = nullptr) {
    std::string data = "grant_type=refresh_token&refresh_token=" + refreshToken;
    std::wstring path = Utf8ToWide("/v1/token?key=" + g_firebaseApiKey);
    DWORD status = 0;
    std::string body = WinHttpPost(L"securetoken.googleapis.com", path, data, L"application/x-www-form-urlencoded", &status);
    bool success = status >= 200 && status < 300 && body.find("\"error\"") == std::string::npos;
    if (ok) *ok = success;
    return body;
}

bool FirebaseEmailVerified(const std::string& idToken) {
    bool ok = false;
    std::string body = FirebaseRequest("accounts:lookup", "{\"idToken\":\"" + JsonEscape(idToken) + "\"}", &ok);
    return ok && body.find("\"emailVerified\": true") != std::string::npos;
}

bool RestoreSavedSession() {
    std::string json = ReadFileUtf8(SessionPath());
    g_remember = JsonBoolValue(json, "remember");
    std::string email = JsonStringValue(json, "email");
    std::string refresh = JsonStringValue(json, "refreshToken");
    if (email.size()) g_email = Utf8ToWide(email);
    if (!g_remember || refresh.empty()) return false;
    bool ok = false;
    std::string refreshed = FirebaseRefresh(refresh, &ok);
    std::string idToken = JsonStringValue(refreshed, "id_token");
    std::string newRefresh = JsonStringValue(refreshed, "refresh_token");
    if (!ok || idToken.empty() || !FirebaseEmailVerified(idToken)) {
        ClearSavedSession();
        return false;
    }
    g_firebaseIdToken = idToken;
    g_firebaseRefreshToken = newRefresh.empty() ? refresh : newRefresh;
    g_authenticatedUser = g_email.empty() ? L"Verified user" : g_email;
    SaveSession(g_firebaseRefreshToken, g_authenticatedUser, true);
    return true;
}

std::string Trim(const std::string& value) {
    size_t begin = 0;
    while (begin < value.size() && (value[begin] == '\xef' || value[begin] == '\xbb' || value[begin] == '\xbf' ||
           value[begin] == '\r' || value[begin] == '\n' || value[begin] == '\t' || value[begin] == ' ')) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && (value[end - 1] == '\r' || value[end - 1] == '\n' || value[end - 1] == '\t' || value[end - 1] == ' ')) {
        --end;
    }
    return value.substr(begin, end - begin);
}

fs::path ExeDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return fs::path(path).parent_path();
}

fs::path RuntimeDirFor(const fs::path& root) {
    return root / RUNTIME_SUPPORT_DIR;
}

fs::path RuntimeDir() {
    return RuntimeDirFor(ExeDir());
}

fs::path RuntimeFileFor(const fs::path& root, const wchar_t* filename) {
    fs::path preferred = RuntimeDirFor(root) / filename;
    if (fs::exists(preferred)) return preferred;
    return root / filename;
}

fs::path RuntimeFile(const wchar_t* filename) {
    return RuntimeFileFor(ExeDir(), filename);
}

fs::path ResourceRoot() {
    fs::path dir = ExeDir();
    if (fs::exists(dir / L"NexusRuntimeCore.html")) return dir;
    if (fs::exists(dir / L"nexus-runtime-core" / L"NexusRuntimeCore.html")) return dir / L"nexus-runtime-core";
    return dir;
}

fs::path ConfigDir() {
    return ExeDir() / L"configs";
}

fs::path BackupDir() {
    return ConfigDir() / L"backups";
}

fs::path ConfigPath() {
    return ConfigDir() / LOCAL_CONFIG_FILENAME;
}

std::string ReadFileUtf8(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::string Timestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << st.wYear << std::setw(2) << st.wMonth
        << std::setw(2) << st.wDay << "-" << std::setw(2) << st.wHour
        << std::setw(2) << st.wMinute << std::setw(2) << st.wSecond
        << "-" << std::setw(3) << st.wMilliseconds;
    return out.str();
}

std::string SanitizeReason(const std::string& reason) {
    std::string out;
    for (char ch : reason) {
        if (std::isalnum((unsigned char)ch) || ch == '-' || ch == '_') out.push_back(ch);
        else if (!out.empty() && out.back() != '_') out.push_back('_');
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out.empty() ? "backup" : out;
}

void EnsureConfigDirs() {
    fs::create_directories(ConfigDir());
    fs::create_directories(BackupDir());
}

void BackupFileIfPresent(const fs::path& path, const std::string& reason) {
    std::error_code ec;
    if (!fs::is_regular_file(path, ec) || fs::file_size(path, ec) == 0) return;
    EnsureConfigDirs();
    fs::path target = BackupDir() / (path.filename().string() + "." + SanitizeReason(reason) + "." + Timestamp() + ".bak");
    int counter = 1;
    while (fs::exists(target)) {
        target = BackupDir() / (path.filename().string() + "." + SanitizeReason(reason) + "." + Timestamp() + "." + std::to_string(counter++) + ".bak");
    }
    fs::copy_file(path, target, fs::copy_options::overwrite_existing, ec);
}

bool LooksLikeFullConfig(const std::string& data, std::string* error = nullptr) {
    std::string blob = Trim(data);
    if (blob.rfind(CONFIG_PREFIX, 0) != 0) {
        if (error) *error = "config must be the full NEXUS operator config";
        return false;
    }
    if (blob.find("\"ALL_OPERATOR_DATA\"") == std::string::npos ||
        blob.find("\"SETTINGS\"") == std::string::npos ||
        blob.find("\"ALL_RAPIDFIRE_SETTINGS\"") == std::string::npos) {
        if (error) *error = "config is missing required sections";
        return false;
    }
    if (std::count(blob.begin(), blob.end(), '[') < 70) {
        if (error) *error = "config does not appear to contain the full operator list";
        return false;
    }
    return true;
}

std::vector<fs::path> LegacyConfigPaths() {
    std::vector<fs::path> paths = {
        ExeDir() / LOCAL_CONFIG_FILENAME,
        ExeDir() / L"dist" / LOCAL_CONFIG_FILENAME,
        ExeDir() / L"dist" / L"configs" / LOCAL_CONFIG_FILENAME,
        ResourceRoot() / L"dist" / L"configs" / LOCAL_CONFIG_FILENAME,
    };
    PWSTR docs = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs) {
        paths.emplace_back(fs::path(docs) / L"Config.txt");
        CoTaskMemFree(docs);
    }
    return paths;
}

std::string ReadLocalConfig() {
    std::lock_guard<std::mutex> lock(g_configMutex);
    EnsureConfigDirs();
    fs::path primary = ConfigPath();
    std::string existing = ReadFileUtf8(primary);
    if (!existing.empty() && LooksLikeFullConfig(existing)) return Trim(existing);
    if (!existing.empty()) BackupFileIfPresent(primary, "invalid-primary");

    for (const fs::path& candidate : LegacyConfigPaths()) {
        std::error_code ec;
        if (fs::equivalent(candidate, primary, ec) || !fs::is_regular_file(candidate, ec)) continue;
        std::string data = ReadFileUtf8(candidate);
        if (!LooksLikeFullConfig(data)) continue;
        BackupFileIfPresent(primary, "import-" + candidate.filename().string());
        std::ofstream out(primary, std::ios::binary | std::ios::trunc);
        out << Trim(data);
        return Trim(data);
    }
    return "";
}

bool WriteLocalConfig(const std::string& data, const std::string& reason, std::string* error = nullptr, const fs::path& target = ConfigPath()) {
    std::string blob = Trim(data);
    if (!LooksLikeFullConfig(blob, error)) return false;
    std::lock_guard<std::mutex> lock(g_configMutex);
    EnsureConfigDirs();
    fs::create_directories(target.parent_path());
    BackupFileIfPresent(target, reason);
    fs::path temp = target;
    temp += ".tmp-" + Timestamp();
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) {
            if (error) *error = "unable to write config";
            return false;
        }
        out << blob;
    }
    std::error_code ec;
    fs::rename(temp, target, ec);
    if (ec) {
        fs::copy_file(temp, target, fs::copy_options::overwrite_existing, ec);
        fs::remove(temp, ec);
    }
    return true;
}

void SetStatus(const std::wstring& text) {
    if (g_status) SetWindowTextW(g_status, text.c_str());
}

void RefreshStateLabels() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    std::wstringstream profile;
    profile << L"Profile: " << (g_profileActive ? L"active" : L"inactive")
            << L"   Speed/X/Y: " << g_speed1 << L", " << g_x1 << L", " << g_y1
            << L"   Secondary: " << g_speed2 << L", " << g_x2 << L", " << g_y2
            << L"   Rapid: " << g_rapidFireEnabled;
    if (g_profile) SetWindowTextW(g_profile, profile.str().c_str());
    std::wstring weapon = L"Weapon: ";
    weapon += g_weaponIndex == 1 ? L"primary" : L"secondary";
    weapon += g_paused ? L"   Paused" : L"   Running";
    if (g_weapon) SetWindowTextW(g_weapon, weapon.c_str());
    if (g_pause) SendMessageW(g_pause, BM_SETCHECK, g_paused ? BST_CHECKED : BST_UNCHECKED, 0);
}

void ClearWindow() {
    std::vector<HWND> children;
    EnumChildWindows(g_hwnd, [](HWND child, LPARAM param) -> BOOL {
        reinterpret_cast<std::vector<HWND>*>(param)->push_back(child);
        return TRUE;
    }, (LPARAM)&children);
    for (HWND child : children) DestroyWindow(child);
    g_status = g_profile = g_pause = g_weapon = nullptr;
}

HWND Label(const std::wstring& text, int x, int y, int w, int h, HFONT font = nullptr) {
    HWND hwnd = CreateWindowW(L"STATIC", text.c_str(), WS_CHILD | WS_VISIBLE, x, y, w, h, g_hwnd, nullptr, nullptr, nullptr);
    SetControlFont(hwnd, font);
    return hwnd;
}

HWND Edit(int id, const std::wstring& text, int x, int y, int w, int h, bool password = false, bool readonly = false) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    if (password) style |= ES_PASSWORD;
    if (readonly) style |= ES_READONLY;
    HWND hwnd = CreateWindowW(L"EDIT", text.c_str(), style, x, y, w, h, g_hwnd, (HMENU)(intptr_t)id, nullptr, nullptr);
    SetControlFont(hwnd);
    return hwnd;
}

HWND Button(int id, const std::wstring& text, int x, int y, int w, int h) {
    HWND hwnd = CreateWindowW(L"BUTTON", text.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, y, w, h, g_hwnd, (HMENU)(intptr_t)id, nullptr, nullptr);
    SetControlFont(hwnd);
    return hwnd;
}

HWND Check(int id, const std::wstring& text, bool checked, int x, int y, int w, int h) {
    HWND hwnd = CreateWindowW(L"BUTTON", text.c_str(), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, x, y, w, h, g_hwnd, (HMENU)(intptr_t)id, nullptr, nullptr);
    SetControlFont(hwnd);
    SendMessageW(hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    return hwnd;
}

void DrawBrandText(const std::wstring& eyebrow, const std::wstring& title1, const std::wstring& title2, const std::wstring& body, const std::wstring& footer) {
    Label(L"NEXUS Loader", 56, 38, 280, 28, g_smallBoldFont);
    Label(eyebrow, 56, 156, 360, 24, g_smallBoldFont);
    Label(title1, 56, 204, 430, 46, g_titleFont);
    Label(title2, 56, 254, 430, 46, g_titleFont);
    Label(body, 56, 330, 440, 58);
    Label(footer, 56, 600, 360, 24);
}

void SetStatusText(const std::wstring& text) {
    if (g_status) SetWindowTextW(g_status, text.c_str());
}

bool BasicEmailValid(const std::wstring& email) {
    size_t at = email.find(L'@');
    size_t dot = email.rfind(L'.');
    return at != std::wstring::npos && dot != std::wstring::npos && at > 0 && dot > at + 1 && dot + 1 < email.size();
}

bool PasswordMeetsRules(const std::wstring& password) {
    bool upper = false, lower = false, number = false;
    for (wchar_t ch : password) {
        upper = upper || iswupper(ch);
        lower = lower || iswlower(ch);
        number = number || iswdigit(ch);
    }
    return password.size() >= 8 && upper && lower && number;
}

bool UsernameValid(const std::wstring& username) {
    if (username.size() < 3 || username.size() > 24) return false;
    for (wchar_t ch : username) {
        if (!(iswalnum(ch) || ch == L'_' || ch == L'-')) return false;
    }
    return true;
}

void ShowLogin() {
    g_screen = Screen::Login;
    ClearWindow();
    SetWindowTextW(g_hwnd, L"Nexus Loader");
    DrawBrandText(L"PRIVATE ACCESS", L"Everything you need.", L"One secure login.", L"Sign in to continue to your workspace and manage your account.", L"Protected connection");
    Label(L"WELCOME BACK", 610, 96, 220, 24, g_smallBoldFont);
    Label(L"Sign in to your account", 610, 132, 420, 42, g_titleFont);
    Label(L"Enter your credentials to continue.", 610, 184, 420, 24);
    Label(L"Email address", 610, 236, 180, 22, g_smallBoldFont);
    Edit(2001, g_email, 610, 262, 380, 28);
    Label(L"Password", 610, 306, 180, 22, g_smallBoldFont);
    Edit(2002, L"", 610, 332, 280, 28, !g_passwordVisible);
    Button(2003, g_passwordVisible ? L"Hide" : L"Show", 900, 331, 90, 30);
    Check(2004, L"Keep me signed in", g_remember, 610, 374, 220, 24);
    g_status = Label(L"", 610, 412, 430, 42);
    Button(2005, L"Sign in", 610, 468, 380, 42);
    Label(L"New here?", 690, 526, 90, 24);
    Button(2006, L"Create an account", 780, 522, 170, 30);
}

void ShowRegister() {
    g_screen = Screen::Register;
    ClearWindow();
    DrawBrandText(L"CREATE YOUR ACCESS", L"One account.", L"Your entire setup.", L"Create a secure account, verify your email, and continue directly into installation.", L"01  Account details");
    Button(2101, L"<  Back to sign in", 610, 44, 170, 30);
    Label(L"NEW ACCOUNT", 610, 92, 220, 24, g_smallBoldFont);
    Label(L"Create your account", 610, 124, 420, 42, g_titleFont);
    Label(L"Use accurate information so account recovery works later.", 610, 172, 440, 24);
    Label(L"Full name", 610, 214, 160, 22, g_smallBoldFont);
    Edit(2102, g_fullName, 610, 240, 180, 28);
    Label(L"Username", 810, 214, 160, 22, g_smallBoldFont);
    Edit(2103, g_username, 810, 240, 180, 28);
    Label(L"Email address", 610, 284, 180, 22, g_smallBoldFont);
    Edit(2104, g_email, 610, 310, 380, 28);
    Label(L"Password", 610, 354, 160, 22, g_smallBoldFont);
    Edit(2105, L"", 610, 380, 180, 28, !g_registerPasswordVisible);
    Button(2106, g_registerPasswordVisible ? L"Hide" : L"Show", 800, 379, 70, 30);
    Label(L"Confirm password", 610, 424, 180, 22, g_smallBoldFont);
    Edit(2107, L"", 610, 450, 180, 28, !g_registerConfirmVisible);
    Button(2108, g_registerConfirmVisible ? L"Hide" : L"Show", 800, 449, 70, 30);
    Label(L"Password rules: 8+ characters, uppercase letter, lowercase letter, number", 610, 492, 470, 24);
    Check(2109, L"I agree to the Terms and Privacy Policy.", g_termsAccepted, 610, 526, 360, 24);
    Button(2110, L"Terms", 610, 560, 80, 28);
    Button(2111, L"Privacy Policy", 700, 560, 130, 28);
    g_status = Label(L"", 610, 598, 430, 42);
    Button(2112, L"Continue", 610, 652, 380, 42);
}

void ShowVerify() {
    g_screen = Screen::Verify;
    ClearWindow();
    DrawBrandText(L"SECURE VERIFICATION", L"Confirm it's", L"really you.", L"Use the Firebase verification email to finish account creation.", L"02  Email verification");
    Button(2201, L"<  Change account details", 610, 78, 220, 30);
    Label(L"CHECK YOUR EMAIL", 610, 166, 240, 24, g_smallBoldFont);
    Label(L"Verify your email", 610, 204, 420, 42, g_titleFont);
    Label(L"We sent a verification link. Click it, then return here and continue.", 610, 260, 440, 48);
    Label(L"Email:", 610, 326, 70, 22, g_smallBoldFont);
    Label(g_pendingEmail, 684, 326, 360, 22);
    g_status = Label(L"", 610, 386, 430, 56);
    Button(2202, L"I verified my email", 610, 458, 380, 42);
    Button(2203, L"Resend verification email", 680, 516, 230, 32);
}

void ShowAccountCreated() {
    g_screen = Screen::AccountCreated;
    ClearWindow();
    DrawBrandText(L"ACCOUNT READY", L"Welcome to", L"Nexus Loader.", L"Your account is verified and signed in. Continue directly into installation.", L"03  Creation complete");
    Label(L"YOU'RE ALL SET", 610, 176, 240, 24, g_smallBoldFont);
    Label(L"Account created", 610, 214, 420, 42, g_titleFont);
    Label(L"Your email has been verified and your account is ready.", 610, 270, 440, 28);
    Label(L"Account", 610, 332, 100, 24, g_smallBoldFont);
    Label(g_pendingUsername, 720, 332, 320, 24);
    Label(L"Email", 610, 366, 100, 24, g_smallBoldFont);
    Label(g_pendingEmail, 720, 366, 320, 24);
    Button(2301, L"Continue to setup", 610, 444, 380, 42);
}

void ShowInstall() {
    g_screen = Screen::Install;
    ClearWindow();
    if (g_installDir.empty()) g_installDir = LoadLastInstallPath();
    if (g_installDir.empty()) g_installDir = DefaultInstallDir();
    DrawBrandText(L"INSTALLATION SETUP", L"Choose where", L"Nexus Loader lives.", L"Select a destination folder, then load the application files using the existing installation logic.", L"02  Final setup step");
    Label(L"SIGNED IN", 56, 648, 120, 22, g_smallBoldFont);
    Label(g_authenticatedUser.empty() ? L"Authenticated user" : g_authenticatedUser, 150, 648, 260, 22);
    Button(2401, L"Log out", 420, 642, 100, 32);
    Label(L"READY TO INSTALL", 610, 124, 240, 24, g_smallBoldFont);
    Label(L"Select a path", 610, 162, 420, 42, g_titleFont);
    Label(L"Choose the folder where the application should be installed.", 610, 214, 440, 28);
    Label(L"Installation path", 610, 268, 180, 22, g_smallBoldFont);
    Edit(2402, g_installDir, 610, 294, 300, 28, false, true);
    Button(2403, L"Browse", 920, 293, 90, 30);
    Label(L"Installation destination", 610, 352, 220, 22, g_smallBoldFont);
    Label(g_installDir, 610, 382, 460, 44);
    g_status = Label(L"", 610, 448, 430, 42);
    Button(2404, L"LOAD", 610, 512, 380, 42);
}

void ShowRuntime() {
    g_screen = Screen::Runtime;
    ClearWindow();
    if (g_displayName.empty()) g_displayName = CurrentExeDisplayName();
    SetWindowTextW(g_hwnd, g_displayName.c_str());
    Label(g_displayName + L" is running as a native C++ app.", 24, 22, 620, 22, g_smallBoldFont);
    g_profile = Label(L"", 24, 58, 700, 22);
    g_weapon = Label(L"", 24, 86, 360, 22);
    g_status = Label(L"Local services: HTTP 20112, WebSocket 20111/8765/6741.", 24, 276, 680, 22);
    Button(1001, L"Open NEXUS UI", 24, 128, 210, 34);
    Button(1002, L"Import Config", 252, 128, 150, 34);
    Button(1003, L"Save Config As", 420, 128, 150, 34);
    Button(1004, L"Primary", 24, 182, 110, 30);
    Button(1005, L"Secondary", 146, 182, 110, 30);
    g_pause = Check(1006, L"Paused", g_paused, 278, 187, 120, 22);
    Label(L"Use right mouse + left mouse after selecting a profile in the UI. Hotkeys default to 1, 2, and P.", 24, 232, 680, 22);
    if (!g_runtimeStarted.exchange(true)) StartRuntimeThreads();
    RefreshStateLabels();
    OpenHtmlUi();
}

std::vector<double> ParseCsvDoubles(const std::string& text) {
    std::vector<double> values;
    std::stringstream ss(text);
    std::string part;
    while (std::getline(ss, part, ',')) {
        values.push_back(std::stod(Trim(part)));
    }
    return values;
}

bool IsStartupPlaceholder(const std::string& text) {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - g_startupTime).count() > 6) return false;
    try {
        std::vector<double> values = ParseCsvDoubles(text);
        double placeholder[] = {0.03, 0.0, 1.0, 0.04, 0.0, 1.0, 0.0};
        if (values.size() != 7) return false;
        for (size_t i = 0; i < 7; ++i) {
            if (std::fabs(values[i] - placeholder[i]) > 0.000001) return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

double NormalizeDelaySeconds(double value);

bool ApplySettings(const std::string& text) {
    std::vector<double> values;
    try {
        values = ParseCsvDoubles(text);
    } catch (...) {
        return false;
    }
    if (values.size() != 3 && values.size() != 7 && values.size() != 11 && values.size() != 13) return false;

    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_speed1 = NormalizeDelaySeconds(values[0]);
    g_x1 = values[1];
    g_y1 = values[2];
    g_rampX1 = 0.0;
    g_rampY1 = 0.0;
    g_rampX2 = 0.0;
    g_rampY2 = 0.0;
    g_rampStart1 = 0.75;
    g_rampStart2 = 0.75;
    if (values.size() == 7) {
        g_speed2 = NormalizeDelaySeconds(values[3]);
        g_x2 = values[4];
        g_y2 = values[5];
        g_rapidFireEnabled = (int)values[6];
    } else if (values.size() == 11 || values.size() == 13) {
        g_speed2 = NormalizeDelaySeconds(values[3]);
        g_x2 = values[4];
        g_y2 = values[5];
        g_rapidFireEnabled = (int)values[6];
        g_rampX1 = values[7];
        g_rampY1 = values[8];
        g_rampX2 = values[9];
        g_rampY2 = values[10];
        if (values.size() == 13) {
            g_rampStart1 = std::clamp(values[11], 0.0, 10.0);
            g_rampStart2 = std::clamp(values[12], 0.0, 10.0);
        }
    } else {
        g_rapidFireEnabled = 0;
    }
    g_paused = false;
    g_profileActive = true;
    PostMessageW(g_hwnd, WM_APP + 1, 0, 0);
    return true;
}

std::chrono::steady_clock::duration SecondsDuration(double seconds) {
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(std::max(0.0, seconds))
    );
}

double NormalizeDelaySeconds(double value) {
    return std::max(0.001, value > 1.0 ? value / 1000.0 : value);
}

struct RuntimeTimingConfig {
    double Target_Base_Delay_MS = 8.0;
    double Max_Variance_MS = 1.5;
    double Spin_Lock_Window_MS = 0.35;
};

struct RuntimeMotionInput {
    double configDx = 0.0;
    double configDy = 0.0;
    double actualElapsedMs = 1.0;
    double targetBaseDelayMs = 1.0;
};

struct RuntimeMotionOutput {
    int moveX = 0;
    int moveY = 0;
};

class IRuntimeMotionCalculator {
public:
    virtual ~IRuntimeMotionCalculator() = default;
    virtual void Reset() = 0;
    virtual RuntimeMotionOutput Calculate(const RuntimeMotionInput& input) = 0;
};

template <typename T, int MinimumTargetDelayMicroseconds>
class RuntimeMotionCalculator final : public IRuntimeMotionCalculator {
public:
    static_assert(std::is_floating_point<T>::value, "T must preserve floating-point timing precision.");
    static_assert(MinimumTargetDelayMicroseconds > 0, "Minimum target delay must be positive.");

    static constexpr T MinimumTargetDelayMs =
        static_cast<T>(MinimumTargetDelayMicroseconds) / static_cast<T>(1000.0);

    RuntimeMotionCalculator()
        : impl_(std::make_unique<Impl>()) {}

    void Reset() override {
        impl_->remainderX = static_cast<T>(0);
        impl_->remainderY = static_cast<T>(0);
    }

    RuntimeMotionOutput Calculate(const RuntimeMotionInput& input) override {
        const T safeTarget = std::max(
            MinimumTargetDelayMs,
            static_cast<T>(input.targetBaseDelayMs)
        );
        const T requestedElapsed = static_cast<T>(input.actualElapsedMs);
        const T safeElapsed = input.actualElapsedMs > 0.0 && std::isfinite(input.actualElapsedMs)
            ? requestedElapsed
            : safeTarget;
        const T dynamicRatio = safeElapsed / safeTarget;
        const T scaledDx = static_cast<T>(input.configDx) * dynamicRatio;
        const T scaledDy = static_cast<T>(input.configDy) * dynamicRatio;

        const T totalX = impl_->remainderX + scaledDx;
        const T totalY = impl_->remainderY + scaledDy;
        const int moveX = static_cast<int>(std::trunc(totalX));
        const int moveY = static_cast<int>(std::trunc(totalY));

        impl_->remainderX = totalX - static_cast<T>(moveX);
        impl_->remainderY = totalY - static_cast<T>(moveY);

        return {moveX, moveY};
    }

private:
    struct Impl {
        T remainderX = static_cast<T>(0);
        T remainderY = static_cast<T>(0);
    };

    std::unique_ptr<Impl> impl_;
};

IRuntimeMotionCalculator& MotionCalculator() {
    static RuntimeMotionCalculator<double, 1> calculator;
    return calculator;
}

double QpcNowMilliseconds() {
    static LARGE_INTEGER frequency = [] {
        LARGE_INTEGER value{};
        QueryPerformanceFrequency(&value);
        return value;
    }();
    LARGE_INTEGER counter{};
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
}

using NtDelayExecutionProc = LONG (NTAPI*)(BOOLEAN, PLARGE_INTEGER);

NtDelayExecutionProc NtDelayExecutionFunction() {
    static NtDelayExecutionProc function = [] {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        return ntdll
            ? reinterpret_cast<NtDelayExecutionProc>(GetProcAddress(ntdll, "NtDelayExecution"))
            : nullptr;
    }();
    return function;
}

void HighPrecisionDelayUntilMilliseconds(double targetTimeMs, double spinLockWindowMs) {
    NtDelayExecutionProc ntDelay = NtDelayExecutionFunction();
    const double spinWindow = std::max(0.05, spinLockWindowMs);
    while (g_running) {
        const double remainingMs = targetTimeMs - QpcNowMilliseconds();
        if (remainingMs <= 0.0) break;

        if (remainingMs > spinWindow && ntDelay) {
            const double delayMs = std::max(0.1, remainingMs - spinWindow);
            LARGE_INTEGER interval{};
            interval.QuadPart = -(LONGLONG)(delayMs * 10000.0);
            ntDelay(FALSE, &interval);
        } else {
            YieldProcessor();
        }
    }
}

double NextBoundedRuntimeDelayMs(
    double targetBaseDelayMs,
    double maxVarianceMs,
    std::mt19937& rng
) {
    const double base = std::max(1.0, targetBaseDelayMs);
    const double variance = std::max(0.0, std::min(maxVarianceMs, base * 0.25));
    if (variance <= 0.0) return base;
    std::uniform_real_distribution<double> jitter(-variance, variance);
    return std::max(1.0, base + jitter(rng));
}

void DisableProfile() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    g_profileActive = false;
    g_rapidFireEnabled = 0;
    g_holding = false;
    g_leftHeld = false;
    g_rightHeld = false;
    g_rapidActionUntil = std::chrono::steady_clock::time_point{};
    g_syntheticClicksUntil = std::chrono::steady_clock::time_point{};
    PostMessageW(g_hwnd, WM_APP + 1, 0, 0);
}

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
    return value;
}

std::string ParseJsonQuotedAt(const std::string& text, size_t quotePos) {
    if (quotePos == std::string::npos || quotePos >= text.size() || text[quotePos] != '"') return "";
    std::string out;
    bool escape = false;
    for (size_t i = quotePos + 1; i < text.size(); ++i) {
        char ch = text[i];
        if (escape) {
            switch (ch) {
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case '\\': out.push_back('\\'); break;
            case '"': out.push_back('"'); break;
            default: out.push_back(ch); break;
            }
            escape = false;
        } else if (ch == '\\') {
            escape = true;
        } else if (ch == '"') {
            break;
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

std::string ConfigSectionValue(const std::string& config, const std::string& sectionName) {
    std::string needle = "\"" + sectionName + "\"";
    size_t section = config.find(needle);
    if (section == std::string::npos) return "";
    size_t comma = config.find(',', section + needle.size());
    if (comma == std::string::npos) return "";
    size_t quote = config.find('"', comma + 1);
    return ParseJsonQuotedAt(config, quote);
}

std::string JsonObjectStringValue(const std::string& objectJson, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t keyPos = objectJson.find(needle);
    if (keyPos == std::string::npos) return "";
    size_t colon = objectJson.find(':', keyPos + needle.size());
    if (colon == std::string::npos) return "";
    size_t quote = objectJson.find('"', colon + 1);
    return ParseJsonQuotedAt(objectJson, quote);
}

std::vector<double> ParseOperatorRowNumbers(const std::string& row) {
    std::vector<double> values;
    std::string token;
    bool inString = false;
    for (char ch : row) {
        if (ch == '"') {
            inString = !inString;
            continue;
        }
        if (inString) continue;
        if (std::isdigit((unsigned char)ch) || ch == '-' || ch == '+' || ch == '.') {
            token.push_back(ch);
        } else if (!token.empty()) {
            values.push_back(std::stod(token));
            token.clear();
        }
    }
    if (!token.empty()) values.push_back(std::stod(token));
    return values;
}

bool ApplyOperatorByName(const std::string& rawName, std::string* error = nullptr) {
    std::string name = ToLowerAscii(Trim(rawName));
    if (name.empty()) {
        if (error) *error = "operator name is empty";
        return false;
    }

    std::string config = ReadLocalConfig();
    std::string operatorMap = ConfigSectionValue(config, "OPERATOR_SETTINGS_BY_NAME");
    std::string rapidMap = ConfigSectionValue(config, "RAPID_FIRE_BY_NAME");
    std::string row = JsonObjectStringValue(operatorMap, name);
    std::string rapid = JsonObjectStringValue(rapidMap, name);
    if (row.empty()) {
        if (error) *error = "operator not found: " + name;
        return false;
    }
    if (rapid.empty()) rapid = "0";

    std::vector<double> values;
    try {
        values = ParseOperatorRowNumbers(row);
    } catch (...) {
        if (error) *error = "operator settings are invalid for: " + name;
        return false;
    }
    if (values.size() < 6) {
        if (error) *error = "operator settings are incomplete for: " + name;
        return false;
    }

    int rapidValue = 0;
    try {
        rapidValue = std::stoi(rapid);
    } catch (...) {
        rapidValue = 0;
    }

    std::ostringstream settings;
    settings << NormalizeDelaySeconds(values[2]) << "," << values[0] << "," << -values[1] << ","
             << NormalizeDelaySeconds(values[5]) << "," << values[3] << "," << -values[4] << ","
             << rapidValue;
    if (!ApplySettings(settings.str())) {
        if (error) *error = "unable to activate operator: " + name;
        return false;
    }

    BroadcastWebSocketText("OPERATOR_SELECTED:" + name);
    SetStatus(Utf8ToWide("Activated operator: " + name));
    return true;
}

std::mutex g_ttsLoadoutMutex;
std::string g_lastSpokenOperator;
std::string g_lastSpokenPrimary;
std::string g_lastSpokenSecondary;

bool TriggerLoadoutSpeechIfChanged(
    const std::string& rawOperator,
    const std::string& rawPrimary,
    const std::string& rawSecondary
) {
    const std::string operatorName = ToLowerAscii(Trim(rawOperator));
    const std::string primary = Trim(rawPrimary);
    const std::string secondary = Trim(rawSecondary);
    if (operatorName.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(g_ttsLoadoutMutex);
        if (operatorName == g_lastSpokenOperator
            && primary == g_lastSpokenPrimary
            && secondary == g_lastSpokenSecondary) {
            return false;
        }
        g_lastSpokenOperator = operatorName;
        g_lastSpokenPrimary = primary;
        g_lastSpokenSecondary = secondary;
    }

    AudioFeedback::LoadoutSpeechClient().TriggerLoadoutSpeech(operatorName, primary, secondary);
    return true;
}

void ApplyHotkeys(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (message.size() > 4) g_pauseKey = message[4];
    if (message.size() > 5) g_primaryHotkey = message[5];
    if (message.size() > 6) g_secondaryHotkey = message[6];
}

char FirstBindableChar(const std::string& value, char fallback) {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) return fallback;
    return (char)std::tolower((unsigned char)trimmed.front());
}

std::pair<std::string, std::string> ParseKeyValueBody(const std::string& body) {
    const size_t separator = body.find('=');
    if (separator == std::string::npos) return {Trim(body), ""};
    return {Trim(body.substr(0, separator)), Trim(body.substr(separator + 1))};
}

bool ApplyRuntimeKeybind(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    const std::string normalized = ToLowerAscii(Trim(key));
    if (normalized == "pause_input") {
        g_pauseKey = FirstBindableChar(value, g_pauseKey);
        return true;
    }
    if (normalized == "primary_weapon") {
        g_primaryHotkey = FirstBindableChar(value, g_primaryHotkey);
        return true;
    }
    if (normalized == "secondary_weapon") {
        g_secondaryHotkey = FirstBindableChar(value, g_secondaryHotkey);
        return true;
    }
    return false;
}

void SetAccentColor(const std::string& color) {
    if (color.empty() || color[0] != '#') return;
    std::string hex = color.substr(1);
    if (hex.size() != 6 && hex.size() != 8) return;
    int r = std::stoi(hex.substr(0, 2), nullptr, 16);
    int g = std::stoi(hex.substr(2, 2), nullptr, 16);
    int b = std::stoi(hex.substr(4, 2), nullptr, 16);
    DWORD value = ((DWORD)b << 16) | ((DWORD)g << 8) | (DWORD)r;
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegSetValueExW(key, L"AccentColor", 0, REG_DWORD, (BYTE*)&value, sizeof(value));
        RegCloseKey(key);
    }
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegSetValueExW(key, L"AccentColorMenu", 0, REG_DWORD, (BYTE*)&value, sizeof(value));
        RegCloseKey(key);
    }
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ImmersiveColorSet", SMTO_ABORTIFHUNG, 500, nullptr);
}

bool ApplyRuntimeAppSetting(const std::string& key, const std::string& value) {
    const std::string normalized = ToLowerAscii(Trim(key));
    const std::string trimmedValue = Trim(value);
    if (normalized == "accent") {
        std::string color = trimmedValue;
        const std::string lowered = ToLowerAscii(trimmedValue);
        if (lowered == "purple") color = "#765BFF";
        else if (lowered == "violet") color = "#8B5CF6";
        else if (lowered == "lavender") color = "#A898FF";
        if (!color.empty() && color.front() == '#') {
            try {
                SetAccentColor(color);
            } catch (...) {
                return false;
            }
        }
        return true;
    }
    if (normalized == "tts_enabled") {
        const std::string lowered = ToLowerAscii(trimmedValue);
        AudioFeedback::LoadoutSpeechClient().SetEnabled(
            lowered == "true" || lowered == "1" || lowered == "yes" || lowered == "on"
        );
        return true;
    }
    if (normalized == "tts_volume") {
        try {
            AudioFeedback::LoadoutSpeechClient().SetVolume(std::stoi(trimmedValue));
        } catch (...) {
            return false;
        }
        return true;
    }
    if (normalized == "theme"
        || normalized == "ui_scale"
        || normalized == "language"
        || normalized == "auto_updates"
        || normalized == "anonymous_data"
        || normalized == "mute_sounds"
        || normalized == "show_fps"
        || normalized == "performance_mode"
        || normalized == "outline_crosshairs"
        || normalized == "minimize_to_tray"
        || normalized == "startup"
        || normalized == "refresh_rate") {
        return true;
    }
    return false;
}

void MoveMouseScaled(double configDx, double configDy, double actualElapsedMs, double targetBaseDelayMs);

void MoveMouse(double dx, double dy) {
    MoveMouseScaled(dx, dy, 1.0, 1.0);
}

void MoveMouseScaled(
    double configDx,
    double configDy,
    double actualElapsedMs,
    double targetBaseDelayMs
) {
    RuntimeMotionOutput move;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        move = MotionCalculator().Calculate({
            configDx,
            configDy,
            actualElapsedMs,
            targetBaseDelayMs
        });
    }
    if (move.moveX == 0 && move.moveY == 0) return;
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = move.moveX;
    input.mi.dy = move.moveY;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(input));
}

void LeftClick(double holdSeconds) {
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_syntheticClicksUntil = std::chrono::steady_clock::now() + SecondsDuration(holdSeconds + 0.01);
    }
    INPUT inputs[2]{};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &inputs[0], sizeof(INPUT));
    std::this_thread::sleep_for(std::chrono::duration<double>(std::max(0.0, holdSeconds)));
    SendInput(1, &inputs[1], sizeof(INPUT));
}

bool RapidFireArmedLocked() {
    return g_profileActive
        && (g_weaponIndex == 1 || g_weaponIndex == 2)
        && (g_rapidFireEnabled == g_weaponIndex || g_rapidFireEnabled == 3)
        && !g_paused;
}

LRESULT CALLBACK MouseHookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0 && (
            wParam == WM_LBUTTONDOWN
            || wParam == WM_LBUTTONUP
            || wParam == WM_RBUTTONDOWN
            || wParam == WM_RBUTTONUP)) {
        auto* event = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        if (!(event->flags & LLMHF_INJECTED)) {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            if (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP) {
                g_rightHeld = (wParam == WM_RBUTTONDOWN);
                if (!g_rightHeld) {
                    g_holding = false;
                    g_rapidActionUntil = std::chrono::steady_clock::time_point{};
                }
            }
            if (wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP) {
                g_leftHeld = (wParam == WM_LBUTTONDOWN);
            }
            if (!g_leftHeld || !g_rightHeld) {
                g_holding = false;
                g_rapidActionUntil = std::chrono::steady_clock::time_point{};
            }
            if ((wParam == WM_LBUTTONDOWN || wParam == WM_LBUTTONUP) && RapidFireArmedLocked() && g_rightHeld) return 1;
        }
    }
    return CallNextHookEx(g_mouseHook, code, wParam, lParam);
}

void InputControlThread() {
    RuntimeTimingConfig timingConfig;
    double nextMoveAtMs = QpcNowMilliseconds();
    double lastMoveAtMs = nextMoveAtMs;
    bool hasLastMoveTimestamp = false;
    std::mt19937 rng((unsigned)GetTickCount());
    int lastActiveWeaponIndex = 0;
    double lastX = 0.0;
    double lastY = 0.0;
    double lastRampX = 0.0;
    double lastRampY = 0.0;
    double lastRampStart = 0.75;
    double activeStartedAtMs = nextMoveAtMs;
    bool hasActiveStart = false;
    bool lastUseSwitchedPattern = false;
    while (g_running) {
        auto nowSteady = std::chrono::steady_clock::now();
        double nowMs = QpcNowMilliseconds();
        SHORT leftState = GetAsyncKeyState(VK_LBUTTON);
        SHORT rightState = GetAsyncKeyState(VK_RBUTTON);
        bool currentLeft = (leftState & 0x8000) != 0;
        bool leftClickedSinceLastRead = (leftState & 0x0001) != 0;
        bool currentRight = (rightState & 0x8000) != 0;

        bool active = false;
        double speed = 0.01;
        double x = 0.0;
        double y = 1.0;
        double rampX = 0.0;
        double rampY = 0.0;
        double rampStart = 0.75;
        int weaponIndex = 1;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_rightHeld = currentRight;
            bool rapidArmed = RapidFireArmedLocked();
            if (rapidArmed) {
                currentLeft = g_leftHeld;
                leftClickedSinceLastRead = false;
            } else if (nowSteady <= g_syntheticClicksUntil) {
                currentLeft = currentLeft || g_leftHeld;
                g_leftHeld = currentLeft;
            } else {
                g_leftHeld = currentLeft;
            }
            bool rapidActionActive = rapidArmed && currentRight && nowSteady <= g_rapidActionUntil;
            g_holding = rapidActionActive || ((!rapidArmed) && currentRight && (currentLeft || leftClickedSinceLastRead));
            active = g_profileActive && g_holding && !g_paused;
            speed = g_weaponIndex == 1 ? g_speed1 : g_speed2;
            x = g_weaponIndex == 1 ? g_x1 : g_x2;
            y = g_weaponIndex == 1 ? g_y1 : g_y2;
            rampX = g_weaponIndex == 1 ? g_rampX1 : g_rampX2;
            rampY = g_weaponIndex == 1 ? g_rampY1 : g_rampY2;
            rampStart = g_weaponIndex == 1 ? g_rampStart1 : g_rampStart2;
            weaponIndex = g_weaponIndex;
        }

        if (!active) {
            nextMoveAtMs = nowMs;
            lastMoveAtMs = nowMs;
            hasLastMoveTimestamp = false;
            lastActiveWeaponIndex = 0;
            hasActiveStart = false;
            lastUseSwitchedPattern = false;
            MotionCalculator().Reset();
            HighPrecisionDelayUntilMilliseconds(nowMs + 1.0, timingConfig.Spin_Lock_Window_MS);
            continue;
        }
        if (!hasActiveStart) {
            activeStartedAtMs = nowMs;
            hasActiveStart = true;
        }
        if (weaponIndex != lastActiveWeaponIndex
            || std::fabs(x - lastX) > 0.000001
            || std::fabs(y - lastY) > 0.000001
            || std::fabs(rampX - lastRampX) > 0.000001
            || std::fabs(rampY - lastRampY) > 0.000001
            || std::fabs(rampStart - lastRampStart) > 0.000001) {
            activeStartedAtMs = nowMs;
            hasActiveStart = true;
            lastUseSwitchedPattern = false;
            MotionCalculator().Reset();
            lastActiveWeaponIndex = weaponIndex;
            lastX = x;
            lastY = y;
            lastRampX = rampX;
            lastRampY = rampY;
            lastRampStart = rampStart;
        }
        const double targetBaseDelayMs = std::max(1.0, speed * 1000.0);
        timingConfig.Target_Base_Delay_MS = targetBaseDelayMs;
        if (nowMs >= nextMoveAtMs) {
            const double activeElapsedSeconds = std::max(0.0, (nowMs - activeStartedAtMs) / 1000.0);
            const bool patternSwitchActive = std::fabs(rampX) > 0.000001 || std::fabs(rampY) > 0.000001;
            const bool useSwitchedPattern = patternSwitchActive && activeElapsedSeconds >= rampStart;
            if (useSwitchedPattern != lastUseSwitchedPattern) {
                MotionCalculator().Reset();
                hasLastMoveTimestamp = false;
                lastUseSwitchedPattern = useSwitchedPattern;
            }
            const double switchedX = std::fabs(rampX) > 0.000001 ? rampX : x;
            const double switchedY = std::fabs(rampY) > 0.000001 ? rampY : y;
            const double activeX = useSwitchedPattern ? switchedX : x;
            const double activeY = useSwitchedPattern ? switchedY : y;
            const double actualElapsedMs = hasLastMoveTimestamp
                ? std::max(0.001, nowMs - lastMoveAtMs)
                : timingConfig.Target_Base_Delay_MS;
            MoveMouseScaled(
                activeX,
                activeY,
                actualElapsedMs,
                timingConfig.Target_Base_Delay_MS
            );
            lastMoveAtMs = nowMs;
            hasLastMoveTimestamp = true;
            nextMoveAtMs = nowMs + NextBoundedRuntimeDelayMs(
                timingConfig.Target_Base_Delay_MS,
                timingConfig.Max_Variance_MS,
                rng
            );
        }
        HighPrecisionDelayUntilMilliseconds(
            std::min(nextMoveAtMs, QpcNowMilliseconds() + 1.0),
            timingConfig.Spin_Lock_Window_MS
        );
    }
}

int VkForChar(char ch) {
    SHORT vk = VkKeyScanA(ch);
    return vk == -1 ? 0 : (vk & 0xFF);
}

void BroadcastWebSocketText(const std::string& message);

void HotkeyThread() {
    while (g_running) {
        int primaryVk, secondaryVk, pauseVk;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            primaryVk = VkForChar(g_primaryHotkey);
            secondaryVk = VkForChar(g_secondaryHotkey);
            pauseVk = VkForChar(g_pauseKey);
        }
        bool primary = primaryVk && (GetAsyncKeyState(primaryVk) & 0x8000);
        bool secondary = secondaryVk && (GetAsyncKeyState(secondaryVk) & 0x8000);
        bool pause = pauseVk && (GetAsyncKeyState(pauseVk) & 0x8000);
        bool sendPause = false;
        bool pausedNow = false;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            if (primary && !g_lastPrimary) g_weaponIndex = 1;
            if (secondary && !g_lastSecondary) g_weaponIndex = 2;
            if (pause && !g_lastPause) {
                g_paused = !g_paused;
                sendPause = true;
                pausedNow = g_paused;
            }
            g_lastPrimary = primary;
            g_lastSecondary = secondary;
            g_lastPause = pause;
        }
        if (primary || secondary || sendPause) PostMessageW(g_hwnd, WM_APP + 1, 0, 0);
        if (sendPause) BroadcastWebSocketText(pausedNow ? "Paused" : "Resumed");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

bool RapidFireActive() {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    return RapidFireArmedLocked() && g_leftHeld && g_rightHeld;
}

bool WaitWhileRapidActive(double seconds) {
    auto deadline = std::chrono::steady_clock::now() + SecondsDuration(seconds);
    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) break;
        if (!RapidFireActive()) return false;
        auto remaining = deadline - now;
        auto tick = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::milliseconds(1));
        std::this_thread::sleep_for(remaining < tick ? remaining : tick);
    }
    return true;
}

void RapidFireThread() {
    std::mt19937 rng((unsigned)GetTickCount() + 17);
    std::normal_distribution<double> executeMs(40.0, 2.0);
    std::normal_distribution<double> intervalMs(60.0, 3.5);
    std::uniform_real_distribution<double> rare(0.0, 1.0);
    while (g_running) {
        if (!RapidFireActive()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        double exec = std::max(1.0, executeMs(rng));
        double interval = std::max(1.0, intervalMs(rng));
        if (rare(rng) < 0.01) interval += 30.0;
        double hold = std::min(0.015, exec / 1000.0);
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            g_rapidActionUntil = std::max(g_rapidActionUntil, std::chrono::steady_clock::now() + SecondsDuration(exec / 1000.0));
        }
        LeftClick(hold);
        if (!WaitWhileRapidActive((exec / 1000.0) - hold)) continue;
        WaitWhileRapidActive(interval / 1000.0);
    }
}

uint32_t LeftRotate(uint32_t value, int bits) {
    return (value << bits) | (value >> (32 - bits));
}

std::array<uint8_t, 20> Sha1(const std::string& input) {
    std::vector<uint8_t> msg(input.begin(), input.end());
    uint64_t bitLen = (uint64_t)msg.size() * 8;
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) msg.push_back(0);
    for (int i = 7; i >= 0; --i) msg.push_back((uint8_t)(bitLen >> (i * 8)));

    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[80]{};
        for (int i = 0; i < 16; ++i) {
            size_t j = chunk + i * 4;
            w[i] = ((uint32_t)msg[j] << 24) | ((uint32_t)msg[j + 1] << 16) | ((uint32_t)msg[j + 2] << 8) | msg[j + 3];
        }
        for (int i = 16; i < 80; ++i) w[i] = LeftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }
            uint32_t temp = LeftRotate(a, 5) + f + e + k + w[i];
            e = d; d = c; c = LeftRotate(b, 30); b = a; a = temp;
        }
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }
    std::array<uint8_t, 20> out{};
    uint32_t h[5] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; ++i) {
        out[i * 4] = (uint8_t)(h[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(h[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(h[i] >> 8);
        out[i * 4 + 3] = (uint8_t)h[i];
    }
    return out;
}

std::string Base64(const uint8_t* data, size_t len) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t value = (uint32_t)data[i] << 16;
        if (i + 1 < len) value |= (uint32_t)data[i + 1] << 8;
        if (i + 2 < len) value |= data[i + 2];
        out.push_back(table[(value >> 18) & 63]);
        out.push_back(table[(value >> 12) & 63]);
        out.push_back(i + 1 < len ? table[(value >> 6) & 63] : '=');
        out.push_back(i + 2 < len ? table[value & 63] : '=');
    }
    return out;
}

bool SendAll(SOCKET s, const char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(s, data + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

bool SendWebSocketText(SOCKET s, const std::string& payload) {
    std::string frame;
    frame.push_back((char)0x81);
    if (payload.size() < 126) {
        frame.push_back((char)payload.size());
    } else if (payload.size() <= 65535) {
        frame.push_back(126);
        frame.push_back((char)((payload.size() >> 8) & 0xFF));
        frame.push_back((char)(payload.size() & 0xFF));
    } else {
        frame.push_back(127);
        uint64_t len = payload.size();
        for (int i = 7; i >= 0; --i) frame.push_back((char)((len >> (i * 8)) & 0xFF));
    }
    frame += payload;
    return SendAll(s, frame.data(), (int)frame.size());
}

void BroadcastWebSocketText(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_clientsMutex);
    for (auto it = g_wsClients.begin(); it != g_wsClients.end();) {
        if (SendWebSocketText(*it, message)) ++it;
        else it = g_wsClients.erase(it);
    }
}

std::string ReceiveWebSocketText(SOCKET s) {
    uint8_t header[2];
    if (recv(s, (char*)header, 2, MSG_WAITALL) != 2) return "";
    uint8_t opcode = header[0] & 0x0F;
    if (opcode == 0x8) return "";
    bool masked = (header[1] & 0x80) != 0;
    uint64_t len = header[1] & 0x7F;
    if (len == 126) {
        uint8_t ext[2];
        if (recv(s, (char*)ext, 2, MSG_WAITALL) != 2) return "";
        len = ((uint64_t)ext[0] << 8) | ext[1];
    } else if (len == 127) {
        uint8_t ext[8];
        if (recv(s, (char*)ext, 8, MSG_WAITALL) != 8) return "";
        len = 0;
        for (uint8_t b : ext) len = (len << 8) | b;
    }
    if (len > 4 * 1024 * 1024) return "";
    uint8_t mask[4]{};
    if (masked && recv(s, (char*)mask, 4, MSG_WAITALL) != 4) return "";
    std::string payload((size_t)len, '\0');
    if (len && recv(s, payload.data(), (int)len, MSG_WAITALL) != (int)len) return "";
    if (masked) {
        for (size_t i = 0; i < payload.size(); ++i) payload[i] ^= mask[i % 4];
    }
    return payload;
}

std::string HeaderValue(const std::string& request, const std::string& key) {
    std::string lower = request;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    std::string needle = "\r\n" + key + ":";
    std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    size_t pos = lower.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    while (pos < request.size() && request[pos] == ' ') ++pos;
    size_t end = request.find("\r\n", pos);
    return request.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

std::string HandleWsMessage(const std::string& message) {
    if (message.rfind(CONFIG_PREFIX, 0) == 0) {
        wchar_t path[MAX_PATH] = L"";
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = g_hwnd;
        ofn.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrDefExt = L"txt";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        wcscpy_s(path, L"nexus_runtime_config.txt");
        if (!GetSaveFileNameW(&ofn)) return "Save cancelled.";
        std::string error;
        if (!WriteLocalConfig(message, "manual-save", &error, path)) return "Invalid config: " + error;
        return "Message saved successfully.";
    }
    if (message == "GET_CONFIG") return ReadLocalConfig();
    if (message == "IMPORT_CONFIG") {
        wchar_t path[MAX_PATH] = L"";
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = g_hwnd;
        ofn.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (!GetOpenFileNameW(&ofn)) return "";
        std::string data = ReadFileUtf8(path);
        std::string error;
        if (!WriteLocalConfig(data, "before-overwrite", &error)) return "Invalid config: " + error;
        return Trim(data);
    }
    if (message.rfind("SAVE_CONFIG:", 0) == 0) {
        std::string error;
        if (!WriteLocalConfig(message.substr(12), "before-overwrite", &error)) return "Invalid config: " + error;
        return "Local config saved successfully.";
    }
    if (message.rfind("SETTINGS:", 0) == 0) {
        std::string settings = message.substr(9);
        if (!IsStartupPlaceholder(settings)) DisableProfile();
        return "";
    }
    if (message.rfind("&WEP&1", 0) == 0) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_weaponIndex = 1;
        PostMessageW(g_hwnd, WM_APP + 1, 0, 0);
        return "";
    }
    if (message.rfind("&WEP&2", 0) == 0) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_weaponIndex = 2;
        PostMessageW(g_hwnd, WM_APP + 1, 0, 0);
        return "";
    }
    if (message.rfind("HKEY", 0) == 0) {
        ApplyHotkeys(message);
        return "Hotkey changes saved successfully.";
    }
    if (!message.empty() && message[0] == '#') {
        try { SetAccentColor(message); } catch (...) {}
        return "";
    }
    if (message == "sudoku") {
        PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
        return "";
    }
    if (!IsStartupPlaceholder(message)) DisableProfile();
    return "";
}

void WebSocketClientThread(SOCKET client) {
    char buffer[8192];
    int n = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        closesocket(client);
        return;
    }
    buffer[n] = 0;
    std::string request(buffer, n);
    std::string key = HeaderValue(request, "sec-websocket-key");
    if (key.empty()) {
        closesocket(client);
        return;
    }
    auto digest = Sha1(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    std::string accept = Base64(digest.data(), digest.size());
    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
    if (!SendAll(client, response.data(), (int)response.size())) {
        closesocket(client);
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_clientsMutex);
        g_wsClients.push_back(client);
    }
    while (g_running) {
        std::string message = ReceiveWebSocketText(client);
        if (message.empty()) break;
        std::string reply = HandleWsMessage(message);
        if (!reply.empty()) SendWebSocketText(client, reply);
    }
    {
        std::lock_guard<std::mutex> lock(g_clientsMutex);
        g_wsClients.erase(std::remove(g_wsClients.begin(), g_wsClients.end(), client), g_wsClients.end());
    }
    closesocket(client);
}

SOCKET CreateListenSocket(int port) {
    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) return INVALID_SOCKET;
    BOOL yes = TRUE;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, LOCAL_HTTP_HOST, &addr.sin_addr);
    addr.sin_port = htons((u_short)port);
    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR || listen(server, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(server);
        return INVALID_SOCKET;
    }
    return server;
}

void WebSocketServerThread(int port) {
    SOCKET server = CreateListenSocket(port);
    if (server == INVALID_SOCKET) return;
    while (g_running) {
        SOCKET client = accept(server, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;
        std::thread(WebSocketClientThread, client).detach();
    }
    closesocket(server);
}

void SendHttp(SOCKET client, int status, const std::string& statusText, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << " " << statusText << "\r\n"
             << "Content-Type: text/plain; charset=utf-8\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;
    std::string text = response.str();
    SendAll(client, text.data(), (int)text.size());
}

std::string MimeForPath(const fs::path& path) {
    std::wstring ext = path.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
    if (ext == L".html") return "text/html; charset=utf-8";
    if (ext == L".css") return "text/css; charset=utf-8";
    if (ext == L".js") return "application/javascript; charset=utf-8";
    if (ext == L".svg") return "image/svg+xml";
    if (ext == L".png") return "image/png";
    if (ext == L".ico") return "image/x-icon";
    return "application/octet-stream";
}

void SendHttpTyped(SOCKET client, int status, const std::string& statusText, const std::string& body, const std::string& contentType) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << " " << statusText << "\r\n"
             << "Content-Type: " << contentType << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n";
    std::string head = response.str();
    SendAll(client, head.data(), (int)head.size());
    if (!body.empty()) SendAll(client, body.data(), (int)body.size());
}

std::string JsonOk(const std::string& message = "OK") {
    return "{\"ok\":true,\"message\":\"" + JsonEscape(message) + "\"}";
}

std::string JsonError(const std::string& message) {
    return "{\"ok\":false,\"message\":\"" + JsonEscape(message) + "\"}";
}

void StartClientRuntimeFromLoader() {
    if (!g_runtimeStarted.exchange(true)) StartRuntimeThreads();
    PostMessageW(g_hwnd, WM_APP + 2, 0, 0);
}

std::string HandleLoaderApi(const std::string& path, const std::string& body) {
    if (path == "/api/session") {
        std::string email = JsonStringValue(ReadFileUtf8(SessionPath()), "email");
        bool remembered = JsonBoolValue(ReadFileUtf8(SessionPath()), "remember");
        return "{\"ok\":true,\"remember\":" + std::string(remembered ? "true" : "false") + ",\"email\":\"" + JsonEscape(email) + "\",\"installPath\":\"" + JsonEscape(WideToUtf8(LoadLastInstallPath())) + "\"}";
    }
    if (path == "/api/login") {
        g_email = Utf8ToWide(JsonStringValue(body, "username"));
        g_password = Utf8ToWide(JsonStringValue(body, "password"));
        g_remember = body.find("\"remember\":true") != std::string::npos || body.find("\"remember\": true") != std::string::npos;
        if (!BasicEmailValid(g_email)) return JsonError("Firebase sign-in uses your email address.");
        if (g_password.empty()) return JsonError("Enter your password.");
        bool ok = false;
        std::string payload = "{\"email\":\"" + JsonEscape(WideToUtf8(g_email)) + "\",\"password\":\"" + JsonEscape(WideToUtf8(g_password)) + "\",\"returnSecureToken\":true}";
        std::string result = FirebaseRequest("accounts:signInWithPassword", payload, &ok);
        if (!ok) return JsonError(FirebaseErrorMessage(result));
        std::string idToken = JsonStringValue(result, "idToken");
        if (!FirebaseEmailVerified(idToken)) return JsonError("Verify your email before signing in.");
        g_firebaseIdToken = idToken;
        g_firebaseRefreshToken = JsonStringValue(result, "refreshToken");
        g_authenticatedUser = g_email;
        if (g_remember) SaveSession(g_firebaseRefreshToken, g_authenticatedUser, true);
        else ClearSavedSession();
        return "{\"ok\":true,\"message\":\"Signed in successfully.\",\"username\":\"" + JsonEscape(WideToUtf8(g_authenticatedUser)) + "\"}";
    }
    if (path == "/api/register") {
        g_fullName = Utf8ToWide(JsonStringValue(body, "fullName"));
        g_username = Utf8ToWide(JsonStringValue(body, "username"));
        g_email = Utf8ToWide(JsonStringValue(body, "email"));
        g_password = Utf8ToWide(JsonStringValue(body, "password"));
        if (g_fullName.size() < 2) return JsonError("Enter your full name.");
        if (!UsernameValid(g_username)) return JsonError("Use 3-24 letters, numbers, underscores, or hyphens for the username.");
        if (!BasicEmailValid(g_email)) return JsonError("Enter a valid email address.");
        if (!PasswordMeetsRules(g_password)) return JsonError("Meet all four password requirements.");
        bool ok = false;
        std::string payload = "{\"email\":\"" + JsonEscape(WideToUtf8(g_email)) + "\",\"password\":\"" + JsonEscape(WideToUtf8(g_password)) + "\",\"returnSecureToken\":true}";
        std::string result = FirebaseRequest("accounts:signUp", payload, &ok);
        if (!ok) return JsonError(FirebaseErrorMessage(result));
        g_pendingIdToken = JsonStringValue(result, "idToken");
        g_pendingRefreshToken = JsonStringValue(result, "refreshToken");
        g_pendingEmail = g_email;
        g_pendingUsername = g_username;
        std::string display = WideToUtf8(g_fullName + L" (" + g_username + L")");
        FirebaseRequest("accounts:update", "{\"idToken\":\"" + JsonEscape(g_pendingIdToken) + "\",\"displayName\":\"" + JsonEscape(display) + "\",\"returnSecureToken\":true}", &ok);
        std::string emailBody = FirebaseRequest("accounts:sendOobCode", "{\"requestType\":\"VERIFY_EMAIL\",\"idToken\":\"" + JsonEscape(g_pendingIdToken) + "\"}", &ok);
        if (!ok) return JsonError(FirebaseErrorMessage(emailBody));
        return "{\"ok\":true,\"requiresVerification\":true,\"message\":\"Account details accepted. Check your email.\",\"username\":\"" + JsonEscape(WideToUtf8(g_username)) + "\",\"email\":\"" + JsonEscape(WideToUtf8(g_email)) + "\"}";
    }
    if (path == "/api/verify") {
        if (g_pendingIdToken.empty()) return JsonError("Your verification session expired. Create the account again.");
        if (!FirebaseEmailVerified(g_pendingIdToken)) return JsonError("Firebase has not marked this email verified yet. Click the email link, wait a moment, then try again.");
        g_firebaseIdToken = g_pendingIdToken;
        g_firebaseRefreshToken = g_pendingRefreshToken;
        g_authenticatedUser = g_pendingEmail;
        SaveSession(g_firebaseRefreshToken, g_authenticatedUser, true);
        return JsonOk("Email verified successfully.");
    }
    if (path == "/api/resend") {
        if (g_pendingIdToken.empty()) return JsonError("Your verification session expired. Create the account again.");
        bool ok = false;
        std::string result = FirebaseRequest("accounts:sendOobCode", "{\"requestType\":\"VERIFY_EMAIL\",\"idToken\":\"" + JsonEscape(g_pendingIdToken) + "\"}", &ok);
        return ok ? JsonOk("A new verification email was sent.") : JsonError(FirebaseErrorMessage(result));
    }
    if (path == "/api/select-path") {
        BROWSEINFOW bi{};
        bi.hwndOwner = g_hwnd;
        bi.lpszTitle = L"Choose installation folder";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (!pidl) return "{\"ok\":true,\"canceled\":true,\"path\":\"\"}";
        wchar_t selected[MAX_PATH] = L"";
        SHGetPathFromIDListW(pidl, selected);
        CoTaskMemFree(pidl);
        g_installDir = selected;
        SaveLastInstallPath(g_installDir);
        return "{\"ok\":true,\"canceled\":false,\"path\":\"" + JsonEscape(WideToUtf8(g_installDir)) + "\"}";
    }
    if (path == "/api/load") {
        g_installDir = Utf8ToWide(JsonStringValue(body, "installPath"));
        if (g_installDir.empty()) return JsonError("Select an installation folder first.");
        try {
            if (IsRainbowSixSiegeRunning()) {
                MessageBoxW(
                    g_hwnd,
                    L"Never load NEXUS while Rainbow Six Siege is open.\n\nRainbow Six Siege will be closed first.",
                    L"Rainbow Six Siege is open",
                    MB_OK | MB_ICONWARNING
                );
                KillRainbowSixSiegeProcesses();
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
            }
            SaveLastInstallPath(g_installDir);
            fs::path installedExe = InstallAndLaunchClientCopy(g_installDir);
            std::wstring gameLaunchError;
            LaunchRainbowSixSiege(&gameLaunchError);
            std::thread([] {
                std::this_thread::sleep_for(std::chrono::milliseconds(650));
                PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
            }).detach();
            return JsonOk("Installed and launched " + installedExe.filename().string());
        } catch (const std::exception& exc) {
            return JsonError(exc.what());
        }
    }
    if (path == "/api/logout") {
        HandleLogout();
        return JsonOk("Logged out.");
    }
    return JsonError("Unknown API route.");
}

void HttpClientThread(SOCKET client) {
    std::string request;
    char buffer[8192];
    int n = recv(client, buffer, sizeof(buffer), 0);
    if (n <= 0) {
        closesocket(client);
        return;
    }
    request.assign(buffer, n);
    size_t firstLineEnd = request.find("\r\n");
    std::string firstLine = request.substr(0, firstLineEnd);
    bool isGet = firstLine.rfind("GET ", 0) == 0;
    bool isPost = firstLine.rfind("POST ", 0) == 0;
    bool configPath = firstLine.find(" /config ") != std::string::npos ||
                      firstLine.find(" /nexus_runtime_config.txt ") != std::string::npos;
    int contentLength = std::stoi(HeaderValue(request, "content-length").empty() ? "0" : HeaderValue(request, "content-length"));
    size_t bodyStart = request.find("\r\n\r\n");
    std::string body = bodyStart == std::string::npos ? "" : request.substr(bodyStart + 4);
    while ((int)body.size() < contentLength) {
        n = recv(client, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        body.append(buffer, n);
    }
    if (isGet && configPath) {
        SendHttp(client, 200, "OK", ReadLocalConfig());
    } else if (isPost && firstLine.find(" /config ") != std::string::npos) {
        std::string error;
        if (WriteLocalConfig(body, "before-overwrite", &error)) SendHttp(client, 200, "OK", "OK");
        else SendHttp(client, 400, "Bad Request", "Invalid config: " + error);
    } else if (isPost && firstLine.find(" /operator ") != std::string::npos) {
        std::string operatorName = JsonStringValue(body, "operator");
        std::string error;
        if (ApplyOperatorByName(operatorName, &error)) {
            SendHttp(client, 200, "OK", "Activated operator: " + ToLowerAscii(Trim(operatorName)));
        } else {
            SendHttp(client, 400, "Bad Request", error);
        }
    } else if (isPost && firstLine.find(" /loadout ") != std::string::npos) {
        const std::string operatorName = JsonStringValue(body, "operator");
        const std::string primary = JsonStringValue(body, "primary");
        const std::string secondary = JsonStringValue(body, "secondary");
        if (TriggerLoadoutSpeechIfChanged(operatorName, primary, secondary)) {
            SendHttp(client, 200, "OK", "Loadout announced");
        } else {
            SendHttp(client, 200, "OK", "Loadout unchanged");
        }
    } else if (isPost && firstLine.find(" /settings ") != std::string::npos) {
        const std::string settings = Trim(body);
        if (settings == "DISABLE") {
            DisableProfile();
            SendHttp(client, 200, "OK", "Profile disabled");
        } else if (ApplySettings(settings)) {
            SendHttp(client, 200, "OK", "Settings applied");
        } else {
            SendHttp(client, 400, "Bad Request", "Error: Use seven comma-separated values.");
        }
    } else if (isPost && firstLine.find(" /keybind ") != std::string::npos) {
        const auto [key, value] = ParseKeyValueBody(body);
        if (ApplyRuntimeKeybind(key, value)) {
            SendHttp(client, 200, "OK", "Keybind applied");
        } else {
            SendHttp(client, 400, "Bad Request", "Unknown keybind");
        }
    } else if (isPost && firstLine.find(" /app-setting ") != std::string::npos) {
        const auto [key, value] = ParseKeyValueBody(body);
        if (ApplyRuntimeAppSetting(key, value)) {
            SendHttp(client, 200, "OK", "App setting applied");
        } else {
            SendHttp(client, 400, "Bad Request", "Unknown app setting");
        }
    } else if (isPost && firstLine.find(" /shutdown ") != std::string::npos) {
        std::wstring cleanupDir = g_installDir.empty() ? LoadLastInstallPath() : g_installDir;
        StopNexusRuntimeHelperProcesses(cleanupDir.empty() ? fs::path() : fs::path(cleanupDir));
        g_running = false;
        SendHttp(client, 200, "OK", "Runtime shutting down");
        PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
    } else {
        SendHttp(client, 404, "Not Found", "Not found");
    }
    closesocket(client);
}

void HttpServerThread() {
    SOCKET server = CreateListenSocket(LOCAL_HTTP_PORT);
    if (server == INVALID_SOCKET) return;
    while (g_running) {
        SOCKET client = accept(server, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;
        std::thread(HttpClientThread, client).detach();
    }
    closesocket(server);
}

std::string UrlDecodePath(std::string value) {
    size_t query = value.find('?');
    if (query != std::string::npos) value.resize(query);
    std::string out;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '%' && i + 2 < value.size()) {
            int code = 0;
            std::stringstream ss;
            ss << std::hex << value.substr(i + 1, 2);
            ss >> code;
            out.push_back((char)code);
            i += 2;
        } else if (value[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(value[i]);
        }
    }
    return out;
}

void LoaderHttpClientThread(SOCKET client) {
    std::string request;
    char buffer[16384];
    int n = recv(client, buffer, sizeof(buffer), 0);
    if (n <= 0) {
        closesocket(client);
        return;
    }
    request.assign(buffer, n);
    size_t firstLineEnd = request.find("\r\n");
    std::string firstLine = request.substr(0, firstLineEnd);
    std::string method, path;
    {
        std::stringstream ss(firstLine);
        ss >> method >> path;
    }
    if (method == "OPTIONS") {
        SendHttpTyped(client, 204, "No Content", "", "text/plain");
        closesocket(client);
        return;
    }
    int contentLength = 0;
    std::string cl = HeaderValue(request, "content-length");
    if (!cl.empty()) contentLength = std::stoi(cl);
    size_t bodyStart = request.find("\r\n\r\n");
    std::string body = bodyStart == std::string::npos ? "" : request.substr(bodyStart + 4);
    while ((int)body.size() < contentLength) {
        n = recv(client, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        body.append(buffer, n);
    }

    path = UrlDecodePath(path.empty() ? "/" : path);
    if (path.rfind("/api/", 0) == 0) {
        SendHttpTyped(client, 200, "OK", HandleLoaderApi(path, body), "application/json; charset=utf-8");
        closesocket(client);
        return;
    }

    fs::path root = ExeDir() / L"nexus-ui" / L"html";
    fs::path assetsRoot = ExeDir() / L"nexus-ui" / L"assets";
    fs::path file;
    if (path == "/" || path == "/index.html") file = root / L"index.html";
    else if (path.rfind("/assets/", 0) == 0) file = assetsRoot / Utf8ToWide(path.substr(8));
    else file = root / Utf8ToWide(path.substr(1));
    std::error_code ec;
    fs::path canonRoot = fs::weakly_canonical(ExeDir() / L"nexus-ui", ec);
    fs::path canonFile = fs::weakly_canonical(file, ec);
    std::wstring rootText = canonRoot.wstring();
    std::wstring fileText = canonFile.wstring();
    if (fileText.rfind(rootText, 0) != 0 || !fs::is_regular_file(canonFile, ec)) {
        SendHttpTyped(client, 404, "Not Found", "Not found", "text/plain; charset=utf-8");
        closesocket(client);
        return;
    }
    SendHttpTyped(client, 200, "OK", ReadFileUtf8(canonFile), MimeForPath(canonFile));
    closesocket(client);
}

void LoaderHttpServerThread() {
    SOCKET server = CreateListenSocket(LOADER_HTTP_PORT);
    if (server == INVALID_SOCKET) return;
    while (g_running) {
        SOCKET client = accept(server, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;
        std::thread(LoaderHttpClientThread, client).detach();
    }
    closesocket(server);
}

void ResizeEmbeddedWebView() {
    if (!g_webViewController) return;
    RECT bounds;
    GetClientRect(g_hwnd, &bounds);
    g_webViewController->put_Bounds(bounds);
}

std::wstring JsStringLiteral(const std::wstring& value) {
    std::wstring out = L"\"";
    for (wchar_t ch : value) {
        if (ch == L'\\') out += L"\\\\";
        else if (ch == L'"') out += L"\\\"";
        else if (ch == L'\n') out += L"\\n";
        else if (ch == L'\r') out += L"\\r";
        else out.push_back(ch);
    }
    out += L"\"";
    return out;
}

void SyncWindowAndDocumentTitle() {
    if (g_displayName.empty()) g_displayName = CurrentExeDisplayName();
    if (g_hwnd) SetWindowTextW(g_hwnd, g_displayName.c_str());
    if (g_webView) {
        std::wstring script = L"document.title=" + JsStringLiteral(g_displayName) + L";";
        g_webView->ExecuteScript(script.c_str(), nullptr);
    }
}

void NavigateEmbedded(const std::wstring& url) {
    if (g_webView) {
        g_webView->Navigate(url.c_str());
        SyncWindowAndDocumentTitle();
    }
}

class NavCompletedHandler final : public ICoreWebView2NavigationCompletedEventHandler {
public:
    NavCompletedHandler() : refCount_(1) {}
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refCount_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&refCount_);
        if (!count) delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2NavigationCompletedEventHandler) {
            *object = static_cast<ICoreWebView2NavigationCompletedEventHandler*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*) override {
        SyncWindowAndDocumentTitle();
        return S_OK;
    }
private:
    volatile ULONG refCount_;
};

class ControllerCompletedHandler final : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
public:
    explicit ControllerCompletedHandler(std::wstring url) : refCount_(1), url_(std::move(url)) {}
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refCount_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&refCount_);
        if (!count) delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler) {
            *object = static_cast<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override {
        if (FAILED(result) || !controller) {
            MessageBoxW(g_hwnd, L"Unable to create the local WebView2 view.", L"WebView Error", MB_OK | MB_ICONERROR);
            return result;
        }
        g_webViewController = controller;
        controller->get_CoreWebView2(&g_webView);
        ResizeEmbeddedWebView();
                            if (g_webView) {
                                ComPtr<ICoreWebView2Settings> settings;
                                if (SUCCEEDED(g_webView->get_Settings(&settings)) && settings) {
                                    settings->put_AreDefaultContextMenusEnabled(FALSE);
                                    settings->put_AreDevToolsEnabled(FALSE);
                                    settings->put_IsStatusBarEnabled(FALSE);
                                }
                                if (!g_displayName.empty()) {
                                    EventRegistrationToken token{};
                                    auto* navHandler = new NavCompletedHandler();
                                    g_webView->add_NavigationCompleted(navHandler, &token);
                                    navHandler->Release();
                                }
                                g_webView->Navigate(url_.c_str());
                                SyncWindowAndDocumentTitle();
                            }
        return S_OK;
    }
private:
    volatile ULONG refCount_;
    std::wstring url_;
};

class EnvironmentCompletedHandler final : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
public:
    explicit EnvironmentCompletedHandler(std::wstring url) : refCount_(1), url_(std::move(url)) {}
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&refCount_); }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = InterlockedDecrement(&refCount_);
        if (!count) delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler) {
            *object = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* environment) override {
        if (FAILED(result) || !environment) {
            MessageBoxW(g_hwnd, L"Unable to initialize the local WebView2 runtime.", L"WebView Error", MB_OK | MB_ICONERROR);
            return result;
        }
        g_webViewEnvironment = environment;
        auto* handler = new ControllerCompletedHandler(url_);
        HRESULT hr = environment->CreateCoreWebView2Controller(g_hwnd, handler);
        handler->Release();
        return hr;
    }
private:
    volatile ULONG refCount_;
    std::wstring url_;
};

void CreateEmbeddedWebView(const std::wstring& url) {
    fs::path userData = LocalAppDataPath() / L"NEXUS" / L"webview2";
    fs::create_directories(userData);
    using GetVersionFn = HRESULT (WINAPI*)(PCWSTR, LPWSTR*);
    using CreateEnvironmentFn = HRESULT (WINAPI*)(
        PCWSTR,
        PCWSTR,
        ICoreWebView2EnvironmentOptions*,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);

    HMODULE loader = LoadLibraryW(RuntimeFile(L"WebView2Loader.dll").c_str());
    if (!loader) loader = LoadLibraryW(L"WebView2Loader.dll");
    if (!loader) {
        MessageBoxW(g_hwnd, L"WebView2Loader.dll is missing. Rebuild the app so the local WebView runtime loader is copied into the runtime folder.", L"WebView Error", MB_OK | MB_ICONERROR);
        return;
    }
    auto createEnvironment = reinterpret_cast<CreateEnvironmentFn>(GetProcAddress(loader, "CreateCoreWebView2EnvironmentWithOptions"));
    if (!createEnvironment) {
        MessageBoxW(g_hwnd, L"WebView2Loader.dll does not expose the expected WebView2 entry point.", L"WebView Error", MB_OK | MB_ICONERROR);
        return;
    }
    auto getVersion = reinterpret_cast<GetVersionFn>(GetProcAddress(loader, "GetAvailableCoreWebView2BrowserVersionString"));
    if (getVersion) {
        LPWSTR version = nullptr;
        HRESULT versionHr = getVersion(nullptr, &version);
        if (FAILED(versionHr) || !version) {
            std::wstringstream message;
            message << L"Microsoft Edge WebView2 Runtime is not available on this machine.\n\nHRESULT: 0x"
                    << std::hex << std::uppercase << (unsigned long)versionHr
                    << L"\n\nInstall the WebView2 Runtime, then reopen the app.";
            MessageBoxW(g_hwnd, message.str().c_str(), L"WebView Error", MB_OK | MB_ICONERROR);
            if (version) CoTaskMemFree(version);
            return;
        }
        CoTaskMemFree(version);
    }

    auto* handler = new EnvironmentCompletedHandler(url);
    HRESULT hr = createEnvironment(nullptr, userData.wstring().c_str(), nullptr, handler);
    handler->Release();
    if (FAILED(hr)) {
        std::wstringstream message;
        message << L"Unable to start the local WebView2 environment.\n\nHRESULT: 0x"
                << std::hex << std::uppercase << (unsigned long)hr;
        MessageBoxW(g_hwnd, message.str().c_str(), L"WebView Error", MB_OK | MB_ICONERROR);
    }
}

void OpenHtmlUi() {
    std::wstring url = ClientCoreHtmlUrl();
    if (url.empty()) {
        MessageBoxW(g_hwnd, L"NexusRuntimeCore.html was not found beside the executable or in nexus-runtime-core.", APP_TITLE, MB_OK | MB_ICONERROR);
        return;
    }
    NavigateEmbedded(url);
}

std::wstring ClientCoreHtmlUrl() {
    fs::path html = ResourceRoot() / L"NexusRuntimeCore.html";
    if (!fs::exists(html)) {
        return L"";
    }
    std::wstring url = L"file:///" + html.wstring();
    std::replace(url.begin(), url.end(), L'\\', L'/');
    return url;
}

void ChooseAndImportConfig() {
    wchar_t path[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    std::string error;
    if (WriteLocalConfig(ReadFileUtf8(path), "before-overwrite", &error)) SetStatus(L"Imported config.");
    else MessageBoxW(g_hwnd, Utf8ToWide("Invalid config: " + error).c_str(), APP_TITLE, MB_OK | MB_ICONERROR);
}

void SaveConfigAs() {
    std::string config = ReadLocalConfig();
    if (config.empty()) {
        MessageBoxW(g_hwnd, L"No valid local config is available to save.", APP_TITLE, MB_OK | MB_ICONERROR);
        return;
    }
    wchar_t path[MAX_PATH] = L"nexus_runtime_config.txt";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return;
    std::string error;
    if (WriteLocalConfig(config, "manual-save", &error, path)) SetStatus(L"Saved config.");
}

void ChooseInstallDir() {
    BROWSEINFOW bi{};
    bi.hwndOwner = g_hwnd;
    bi.lpszTitle = L"Choose installation folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;
    wchar_t path[MAX_PATH];
    if (SHGetPathFromIDListW(pidl, path)) {
        g_installDir = path;
        SaveLastInstallPath(g_installDir);
        ShowInstall();
        SetStatusText(L"Installation path selected.");
    }
    CoTaskMemFree(pidl);
}

void BeginInstallation() {
    if (g_installDir.empty()) {
        SetStatusText(L"Select an installation folder first.");
        return;
    }
    try {
        SaveLastInstallPath(g_installDir);
        InstallAndLaunchClientCopy(g_installDir);
        SetStatusText(L"Installation started.");
        PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
    } catch (const std::exception& exc) {
        MessageBoxW(g_hwnd, Utf8ToWide(exc.what()).c_str(), L"Installation Failed", MB_OK | MB_ICONERROR);
        SetStatusText(Utf8ToWide(exc.what()));
    }
}

void HandleSignIn() {
    g_email = GetWindowTextString(GetDlgItem(g_hwnd, 2001));
    g_password = GetWindowTextString(GetDlgItem(g_hwnd, 2002));
    g_remember = SendMessageW(GetDlgItem(g_hwnd, 2004), BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (!BasicEmailValid(g_email)) {
        SetStatusText(L"Firebase sign-in uses your email address.");
        return;
    }
    if (g_password.empty()) {
        SetStatusText(L"Enter your password.");
        return;
    }
    if (GetEnvironmentVariableW(L"LOADER_ALLOW_DEMO_LOGIN", nullptr, 0) && g_email == L"user" && g_password == L"pass") {
        g_authenticatedUser = g_email;
        ShowInstall();
        return;
    }
    bool ok = false;
    std::string payload = "{\"email\":\"" + JsonEscape(WideToUtf8(g_email)) + "\",\"password\":\"" + JsonEscape(WideToUtf8(g_password)) + "\",\"returnSecureToken\":true}";
    std::string body = FirebaseRequest("accounts:signInWithPassword", payload, &ok);
    if (!ok) {
        SetStatusText(Utf8ToWide(FirebaseErrorMessage(body)));
        return;
    }
    std::string idToken = JsonStringValue(body, "idToken");
    if (!FirebaseEmailVerified(idToken)) {
        ClearSavedSession();
        SetStatusText(L"Verify your email before signing in.");
        return;
    }
    g_firebaseIdToken = idToken;
    g_firebaseRefreshToken = JsonStringValue(body, "refreshToken");
    g_authenticatedUser = g_email;
    if (g_remember) SaveSession(g_firebaseRefreshToken, g_authenticatedUser, true);
    else ClearSavedSession();
    ShowInstall();
}

void HandleRegister() {
    g_fullName = GetWindowTextString(GetDlgItem(g_hwnd, 2102));
    g_username = GetWindowTextString(GetDlgItem(g_hwnd, 2103));
    g_email = GetWindowTextString(GetDlgItem(g_hwnd, 2104));
    g_password = GetWindowTextString(GetDlgItem(g_hwnd, 2105));
    g_confirmPassword = GetWindowTextString(GetDlgItem(g_hwnd, 2107));
    g_termsAccepted = SendMessageW(GetDlgItem(g_hwnd, 2109), BM_GETCHECK, 0, 0) == BST_CHECKED;
    if (g_fullName.size() < 2) { SetStatusText(L"Enter your full name."); return; }
    if (!UsernameValid(g_username)) { SetStatusText(L"Use 3-24 letters, numbers, underscores, or hyphens for the username."); return; }
    if (!BasicEmailValid(g_email)) { SetStatusText(L"Enter a valid email address."); return; }
    if (!PasswordMeetsRules(g_password)) { SetStatusText(L"Meet all four password requirements."); return; }
    if (g_password != g_confirmPassword) { SetStatusText(L"Passwords do not match."); return; }
    if (!g_termsAccepted) { SetStatusText(L"Accept the Terms and Privacy Policy to continue."); return; }

    bool ok = false;
    std::string payload = "{\"email\":\"" + JsonEscape(WideToUtf8(g_email)) + "\",\"password\":\"" + JsonEscape(WideToUtf8(g_password)) + "\",\"returnSecureToken\":true}";
    std::string body = FirebaseRequest("accounts:signUp", payload, &ok);
    if (!ok) {
        SetStatusText(Utf8ToWide(FirebaseErrorMessage(body)));
        return;
    }
    g_pendingIdToken = JsonStringValue(body, "idToken");
    g_pendingRefreshToken = JsonStringValue(body, "refreshToken");
    g_pendingEmail = g_email;
    g_pendingUsername = g_username;
    std::string display = WideToUtf8(g_fullName + L" (" + g_username + L")");
    std::string updatePayload = "{\"idToken\":\"" + JsonEscape(g_pendingIdToken) + "\",\"displayName\":\"" + JsonEscape(display) + "\",\"returnSecureToken\":true}";
    std::string updateBody = FirebaseRequest("accounts:update", updatePayload, &ok);
    if (ok && !JsonStringValue(updateBody, "idToken").empty()) {
        g_pendingIdToken = JsonStringValue(updateBody, "idToken");
        std::string refresh = JsonStringValue(updateBody, "refreshToken");
        if (!refresh.empty()) g_pendingRefreshToken = refresh;
    }
    std::string emailPayload = "{\"requestType\":\"VERIFY_EMAIL\",\"idToken\":\"" + JsonEscape(g_pendingIdToken) + "\"}";
    std::string emailBody = FirebaseRequest("accounts:sendOobCode", emailPayload, &ok);
    if (!ok) {
        SetStatusText(Utf8ToWide(FirebaseErrorMessage(emailBody)));
        return;
    }
    ShowVerify();
}

void HandleVerify() {
    if (g_pendingIdToken.empty()) {
        SetStatusText(L"Your verification session expired. Create the account again.");
        return;
    }
    if (!FirebaseEmailVerified(g_pendingIdToken)) {
        SetStatusText(L"Firebase has not marked this email verified yet. Click the email link, wait a moment, then try again.");
        return;
    }
    g_firebaseIdToken = g_pendingIdToken;
    g_firebaseRefreshToken = g_pendingRefreshToken;
    g_authenticatedUser = g_pendingEmail;
    g_remember = true;
    SaveSession(g_firebaseRefreshToken, g_authenticatedUser, true);
    ShowAccountCreated();
}

void HandleResendCode() {
    if (g_pendingIdToken.empty()) {
        SetStatusText(L"Your verification session expired. Create the account again.");
        return;
    }
    bool ok = false;
    std::string payload = "{\"requestType\":\"VERIFY_EMAIL\",\"idToken\":\"" + JsonEscape(g_pendingIdToken) + "\"}";
    std::string body = FirebaseRequest("accounts:sendOobCode", payload, &ok);
    SetStatusText(ok ? L"A new verification email was sent." : Utf8ToWide(FirebaseErrorMessage(body)));
}

void HandleLogout() {
    if (MessageBoxW(g_hwnd, L"Log out of Nexus Loader?", L"Log out", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    g_firebaseIdToken.clear();
    g_firebaseRefreshToken.clear();
    g_pendingIdToken.clear();
    g_pendingRefreshToken.clear();
    g_authenticatedUser.clear();
    g_email.clear();
    g_password.clear();
    g_installDir.clear();
    g_remember = false;
    ClearSavedSession();
    ShowLogin();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_hwnd = hwnd;
        LoadFirebaseConfig();
        if (g_backendOnlyMode) {
            SetWindowTextW(g_hwnd, g_displayName.c_str());
            if (!g_runtimeStarted.exchange(true)) StartRuntimeThreads();
            return 0;
        }
        if (g_clientOnlyMode) {
            SetWindowTextW(g_hwnd, g_displayName.c_str());
            if (!g_runtimeStarted.exchange(true)) StartRuntimeThreads();
            std::wstring url = ClientCoreHtmlUrl();
            if (url.empty()) {
                MessageBoxW(g_hwnd, L"NexusRuntimeCore.html was not found beside the executable or in nexus-runtime-core.", APP_TITLE, MB_OK | MB_ICONERROR);
                PostQuitMessage(1);
                return 0;
            }
            CreateEmbeddedWebView(url);
            return 0;
        }
        {
            WSADATA wsa{};
            WSAStartup(MAKEWORD(2, 2), &wsa);
        }
        std::thread(LoaderHttpServerThread).detach();
        CreateEmbeddedWebView(L"http://127.0.0.1:20110/index.html");
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 2003:
            g_passwordVisible = !g_passwordVisible;
            g_email = GetWindowTextString(GetDlgItem(g_hwnd, 2001));
            ShowLogin();
            break;
        case 2005: HandleSignIn(); break;
        case 2006: ShowRegister(); break;
        case 2101: ShowLogin(); break;
        case 2106:
            g_registerPasswordVisible = !g_registerPasswordVisible;
            g_fullName = GetWindowTextString(GetDlgItem(g_hwnd, 2102));
            g_username = GetWindowTextString(GetDlgItem(g_hwnd, 2103));
            g_email = GetWindowTextString(GetDlgItem(g_hwnd, 2104));
            g_termsAccepted = SendMessageW(GetDlgItem(g_hwnd, 2109), BM_GETCHECK, 0, 0) == BST_CHECKED;
            ShowRegister();
            break;
        case 2108:
            g_registerConfirmVisible = !g_registerConfirmVisible;
            g_fullName = GetWindowTextString(GetDlgItem(g_hwnd, 2102));
            g_username = GetWindowTextString(GetDlgItem(g_hwnd, 2103));
            g_email = GetWindowTextString(GetDlgItem(g_hwnd, 2104));
            g_termsAccepted = SendMessageW(GetDlgItem(g_hwnd, 2109), BM_GETCHECK, 0, 0) == BST_CHECKED;
            ShowRegister();
            break;
        case 2110:
            SetStatusText(L"Terms are not configured for this installer.");
            break;
        case 2111:
            SetStatusText(L"Privacy Policy is not configured for this installer.");
            break;
        case 2112: HandleRegister(); break;
        case 2201: ShowRegister(); break;
        case 2202: HandleVerify(); break;
        case 2203: HandleResendCode(); break;
        case 2301: ShowInstall(); break;
        case 2401: HandleLogout(); break;
        case 2403: ChooseInstallDir(); break;
        case 2404: BeginInstallation(); break;
        case 1001: OpenHtmlUi(); break;
        case 1002: ChooseAndImportConfig(); break;
        case 1003: SaveConfigAs(); break;
        case 1004:
            { std::lock_guard<std::mutex> lock(g_stateMutex); g_weaponIndex = 1; }
            RefreshStateLabels();
            break;
        case 1005:
            { std::lock_guard<std::mutex> lock(g_stateMutex); g_weaponIndex = 2; }
            RefreshStateLabels();
            break;
        case 1006:
            { std::lock_guard<std::mutex> lock(g_stateMutex); g_paused = SendMessageW(g_pause, BM_GETCHECK, 0, 0) == BST_CHECKED; }
            RefreshStateLabels();
            BroadcastWebSocketText(g_paused ? "Paused" : "Resumed");
            break;
        }
        return 0;
    case WM_APP + 1:
        RefreshStateLabels();
        return 0;
    case WM_APP + 2:
        OpenHtmlUi();
        return 0;
    case WM_SIZE:
        ResizeEmbeddedWebView();
        return 0;
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT);
        SetBkColor(hdc, COLOR_BG);
        return (LRESULT)g_bgBrush;
    }
    case WM_DESTROY:
        g_running = false;
        if (g_mouseHook) UnhookWindowsHookEx(g_mouseHook);
        {
            std::wstring cleanupDir = g_installDir.empty() ? LoadLastInstallPath() : g_installDir;
            StopNexusRuntimeHelperProcesses(cleanupDir.empty() ? fs::path() : fs::path(cleanupDir));
        }
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void StartRuntimeThreads() {
    timeBeginPeriod(1);
    g_startupTime = std::chrono::steady_clock::now();
    WSADATA wsa{};
    WSAStartup(MAKEWORD(2, 2), &wsa);
    ReadLocalConfig();
    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, nullptr, 0);
    std::thread(InputControlThread).detach();
    std::thread(HotkeyThread).detach();
    std::thread(RapidFireThread).detach();
    std::thread(HttpServerThread).detach();
    for (int port : WS_PORTS) std::thread(WebSocketServerThread, port).detach();
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    std::wstring commandLine = GetCommandLineW();
    g_clientOnlyMode = commandLine.find(L"--nexus") != std::wstring::npos;
    g_backendOnlyMode = commandLine.find(L"--backend-only") != std::wstring::npos;
    g_displayName = (g_clientOnlyMode || g_backendOnlyMode) ? CurrentExeDisplayName() : L"Nexus Loader";
    HRESULT comHr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comHr)) {
        std::wstringstream message;
        message << L"Unable to initialize Windows COM services.\n\nHRESULT: 0x"
                << std::hex << std::uppercase << (unsigned long)comHr;
        MessageBoxW(nullptr, message.str().c_str(), L"Startup Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    std::wstring appId = L"NexusLoader.NativeCpp";
    if (g_clientOnlyMode || g_backendOnlyMode) appId = L"Installed." + g_displayName;
    SetCurrentProcessExplicitAppUserModelID(appId.c_str());
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&icc);
    g_font = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_titleFont = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_smallBoldFont = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_bgBrush = CreateSolidBrush(COLOR_BG);
    g_panelBrush = CreateSolidBrush(COLOR_PANEL);
    g_inputBrush = CreateSolidBrush(COLOR_INPUT);

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"NEXUSWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    wc.hbrBackground = g_bgBrush;
    RegisterClassW(&wc);

    const bool loaderUiMode = !g_clientOnlyMode && !g_backendOnlyMode;
    const DWORD windowStyle = loaderUiMode
        ? WS_POPUP
        : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    const int windowWidth = loaderUiMode ? 640 : 1120;
    const int windowHeight = loaderUiMode ? 900 : 760;
    g_hwnd = CreateWindowW(wc.lpszClassName, g_displayName.c_str(),
                           windowStyle,
                           CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
                           nullptr, nullptr, instance, nullptr);
    if (!g_hwnd) return 1;
    if (loaderUiMode) {
        const int x = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
        const int y = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;
        SetWindowPos(g_hwnd, nullptr, std::max(0, x), std::max(0, y), windowWidth, windowHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (g_backendOnlyMode) {
        ShowWindow(g_hwnd, SW_HIDE);
    } else {
        ShowWindow(g_hwnd, show);
        UpdateWindow(g_hwnd);
    }
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    WSACleanup();
    CoUninitialize();
    return (int)msg.wParam;
}

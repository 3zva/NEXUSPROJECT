#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <opencv2/imgproc.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

namespace fs = std::filesystem;

constexpr int MAIN_APP_HTTP_PORT = 20112;
constexpr int VISIBLE_UI_HTTP_PORT = 20113;
constexpr wchar_t DEFAULT_CONFIG_FILENAME[] = L"vision_overlay_config.txt";

std::atomic_bool g_running{true};
HANDLE g_singleInstanceMutex = nullptr;
std::string g_processOcrLanguage = "eng";
std::string g_processOcrTessdataPath = "C:/msys64/mingw64/share/tessdata";

void EnableProcessDpiAwareness() {
    using SetDpiAwarenessContextFn = BOOL(WINAPI*)(HANDLE);
    HMODULE user32 = LoadLibraryW(L"user32.dll");
    if (user32 != nullptr) {
        auto setDpiAwarenessContext = reinterpret_cast<SetDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext")
        );
        if (setDpiAwarenessContext != nullptr
            && setDpiAwarenessContext(reinterpret_cast<HANDLE>(-4))) {
            FreeLibrary(user32);
            return;
        }
        FreeLibrary(user32);
    }
    SetProcessDPIAware();
}

struct Region {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct OverlayConfig {
    Region region{800, 420, 320, 120};
    bool regionConfigured = false;
    double currentAspectRatio = 4.0 / 3.0;
    double nativeAspectRatio = 16.0 / 9.0;
    int fps = 10;
    int cooldownMs = 1000;
    int minimumOcrConfidence = 55;
    bool pauseWhenCursorHidden = false;
    std::string language = "eng";
    std::string tessdataPath = "C:/msys64/mingw64/share/tessdata";
    std::vector<std::string> operators;
};

Region ClampRegionToVirtualScreen(Region region);

const char* DEFAULT_OPERATORS[] = {
    "striker", "sledge", "thatcher", "ash", "thermite", "twitch", "montagne", "glaz", "fuze", "blitz",
    "iq", "buck", "blackbeard", "capitao", "hibana", "jackal", "ying", "zofia", "dokkaebi", "lion",
    "finka", "maverick", "nomad", "gridlock", "nokk", "amaru", "kali", "iana", "ace", "zero",
    "flores", "osa", "sens", "grim", "brava", "ram", "deimos", "rauora", "sentry", "smoke",
    "mute", "castle", "pulse", "doc", "rook", "kapkan", "tachanka", "jager", "bandit", "frost",
    "valkyrie", "caveira", "echo", "mira", "lesion", "ela", "vigil", "maestro", "alibi", "clash",
    "kaid", "mozzie", "warden", "goyo", "wamai", "oryx", "melusi", "aruni", "thunderbird", "thorn",
    "azami", "solis", "fenrir", "tubarao", "skopos", "denari"
};

void Log(const std::string& message) {
    std::ofstream out("vision_overlay.log", std::ios::app);
    out << message << "\n";
}

std::wstring CurrentExeDir() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return fs::path(path).parent_path().wstring();
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return "";
    int needed = WideCharToMultiByte(CP_UTF8, 0, value.data(), (int)value.size(), nullptr, 0, nullptr, nullptr);
    std::string out(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), (int)value.size(), out.data(), needed, nullptr, nullptr);
    return out;
}

std::string Trim(std::string value) {
    while (!value.empty() && std::isspace((unsigned char)value.front())) value.erase(value.begin());
    while (!value.empty() && std::isspace((unsigned char)value.back())) value.pop_back();
    return value;
}

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return value;
}

std::string ReplaceAll(std::string value, const std::string& from, const std::string& to) {
    if (from.empty()) return value;
    size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos) {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
    return value;
}

std::string FoldOperatorTextToAscii(std::string value) {
    value = ReplaceAll(value, "Ã", "A");
    value = ReplaceAll(value, "ã", "a");
    value = ReplaceAll(value, "Á", "A");
    value = ReplaceAll(value, "á", "a");
    value = ReplaceAll(value, "À", "A");
    value = ReplaceAll(value, "à", "a");
    value = ReplaceAll(value, "Â", "A");
    value = ReplaceAll(value, "â", "a");
    value = ReplaceAll(value, "Ä", "A");
    value = ReplaceAll(value, "ä", "a");
    value = ReplaceAll(value, "Ç", "C");
    value = ReplaceAll(value, "ç", "c");
    value = ReplaceAll(value, "É", "E");
    value = ReplaceAll(value, "é", "e");
    value = ReplaceAll(value, "È", "E");
    value = ReplaceAll(value, "è", "e");
    value = ReplaceAll(value, "Ê", "E");
    value = ReplaceAll(value, "ê", "e");
    value = ReplaceAll(value, "Í", "I");
    value = ReplaceAll(value, "í", "i");
    value = ReplaceAll(value, "Ñ", "N");
    value = ReplaceAll(value, "ñ", "n");
    value = ReplaceAll(value, "Ó", "O");
    value = ReplaceAll(value, "ó", "o");
    value = ReplaceAll(value, "Ô", "O");
    value = ReplaceAll(value, "ô", "o");
    value = ReplaceAll(value, "Õ", "O");
    value = ReplaceAll(value, "õ", "o");
    value = ReplaceAll(value, "Ö", "O");
    value = ReplaceAll(value, "ö", "o");
    value = ReplaceAll(value, "Ø", "O");
    value = ReplaceAll(value, "ø", "o");
    value = ReplaceAll(value, "Ú", "U");
    value = ReplaceAll(value, "ú", "u");
    value = ReplaceAll(value, "Ü", "U");
    value = ReplaceAll(value, "ü", "u");
    return value;
}

std::vector<std::string> SplitCsv(const std::string& value) {
    std::vector<std::string> out;
    std::stringstream ss(value);
    std::string part;
    while (std::getline(ss, part, ',')) {
        part = ToLowerAscii(Trim(part));
        if (!part.empty()) out.push_back(part);
    }
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

std::string ReadFileUtf8(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

OverlayConfig LoadConfig(const fs::path& path) {
    OverlayConfig config;
    config.operators.assign(std::begin(DEFAULT_OPERATORS), std::end(DEFAULT_OPERATORS));
    std::string text = ReadFileUtf8(path);
    if (text.empty()) {
        Log("Config missing; waiting for a saved region.");
        return config;
    }

    auto parseInt = [&](const std::string& key, int fallback) {
        std::string value = ConfigValueLine(text, key);
        if (value.empty()) return fallback;
        try { return std::stoi(value); } catch (...) { return fallback; }
    };
    auto parseBool = [&](const std::string& key, bool fallback) {
        std::string value = ToLowerAscii(ConfigValueLine(text, key));
        if (value.empty()) return fallback;
        return value == "1" || value == "true" || value == "yes" || value == "on";
    };
    auto parseDouble = [&](const std::string& key, double fallback) {
        std::string value = ConfigValueLine(text, key);
        if (value.empty()) return fallback;
        try {
            double parsed = std::stod(value);
            return std::isfinite(parsed) && parsed > 0.0 ? parsed : fallback;
        } catch (...) {
            return fallback;
        }
    };

    config.regionConfigured = parseBool("region_configured", false);
    config.region.x = parseInt("region_x", parseInt("x", config.region.x));
    config.region.y = parseInt("region_y", parseInt("y", config.region.y));
    config.region.width = parseInt("region_width", parseInt("width", config.region.width));
    config.region.height = parseInt("region_height", parseInt("height", config.region.height));
    config.currentAspectRatio = parseDouble("current_aspect_ratio", config.currentAspectRatio);
    config.nativeAspectRatio = parseDouble("native_aspect_ratio", config.nativeAspectRatio);
    config.fps = std::max(1, parseInt("fps", config.fps));
    config.cooldownMs = std::max(0, parseInt("cooldown_ms", config.cooldownMs));
    config.minimumOcrConfidence = std::clamp(parseInt("minimum_ocr_confidence", config.minimumOcrConfidence), 0, 100);
    config.pauseWhenCursorHidden = parseBool("pause_when_cursor_hidden", config.pauseWhenCursorHidden);

    std::string language = ConfigValueLine(text, "language");
    if (!language.empty()) config.language = language;
    std::string tessdata = ConfigValueLine(text, "tessdata");
    if (!tessdata.empty()) config.tessdataPath = tessdata;
    std::string operators = ConfigValueLine(text, "operators");
    if (!operators.empty()) {
        std::vector<std::string> parsed = SplitCsv(operators);
        if (!parsed.empty()) config.operators = parsed;
    }

    config.region = ClampRegionToVirtualScreen(config.region);
    if (config.region.width <= 0 || config.region.height <= 0) {
        config.regionConfigured = false;
    }
    return config;
}

bool HasUsableRegion(const OverlayConfig& config) {
    return config.regionConfigured
        && config.region.width > 0
        && config.region.height > 0;
}

Region ClampRegionToVirtualScreen(Region region) {
    const int virtualX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int virtualY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    const int virtualRight = virtualX + virtualWidth;
    const int virtualBottom = virtualY + virtualHeight;

    const int left = std::clamp(region.x, virtualX, virtualRight);
    const int top = std::clamp(region.y, virtualY, virtualBottom);
    const int right = std::clamp(region.x + region.width, virtualX, virtualRight);
    const int bottom = std::clamp(region.y + region.height, virtualY, virtualBottom);

    return Region{
        left,
        top,
        std::max(0, right - left),
        std::max(0, bottom - top)
    };
}

class CaptureManager {
public:
    explicit CaptureManager(Region region) : region_(region) {
        screenDc_.reset(GetDC(nullptr));
        if (!screenDc_) throw std::runtime_error("GetDC failed.");
        memoryDc_.reset(CreateCompatibleDC(screenDc_.get()));
        if (!memoryDc_) throw std::runtime_error("CreateCompatibleDC failed.");

        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = region_.width;
        info.bmiHeader.biHeight = -region_.height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* rawPixels = nullptr;
        captureBitmap_.reset(CreateDIBSection(screenDc_.get(), &info, DIB_RGB_COLORS, &rawPixels, nullptr, 0));
        if (!captureBitmap_ || !rawPixels) throw std::runtime_error("CreateDIBSection failed.");
        previousBitmap_ = static_cast<HBITMAP>(SelectObject(memoryDc_.get(), captureBitmap_.get()));
        pixels_ = rawPixels;
        bgraFrame_ = cv::Mat(region_.height, region_.width, CV_8UC4, pixels_);
    }

    ~CaptureManager() {
        if (memoryDc_ && previousBitmap_) SelectObject(memoryDc_.get(), previousBitmap_);
    }

    bool IsCursorVisible() const {
        CURSORINFO cursorInfo{};
        cursorInfo.cbSize = sizeof(CURSORINFO);
        if (!GetCursorInfo(&cursorInfo)) return false;
        return (cursorInfo.flags & CURSOR_SHOWING) != 0;
    }

    cv::Mat CaptureBgr() {
        if (!BitBlt(memoryDc_.get(), 0, 0, region_.width, region_.height, screenDc_.get(), region_.x, region_.y, SRCCOPY | CAPTUREBLT)) {
            throw std::runtime_error("BitBlt failed.");
        }
        cv::Mat bgr;
        cv::cvtColor(bgraFrame_, bgr, cv::COLOR_BGRA2BGR);
        return bgr;
    }

private:
    struct ScreenDcDeleter { void operator()(HDC dc) const { if (dc) ReleaseDC(nullptr, dc); } };
    struct MemoryDcDeleter { void operator()(HDC dc) const { if (dc) DeleteDC(dc); } };
    struct BitmapDeleter { void operator()(HBITMAP bitmap) const { if (bitmap) DeleteObject(bitmap); } };

    Region region_;
    std::unique_ptr<std::remove_pointer_t<HDC>, ScreenDcDeleter> screenDc_;
    std::unique_ptr<std::remove_pointer_t<HDC>, MemoryDcDeleter> memoryDc_;
    std::unique_ptr<std::remove_pointer_t<HBITMAP>, BitmapDeleter> captureBitmap_;
    HBITMAP previousBitmap_ = nullptr;
    void* pixels_ = nullptr;
    cv::Mat bgraFrame_;
};

cv::Mat PrepareForOcr(const cv::Mat& frame) {
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::Mat scaled;
    cv::resize(gray, scaled, cv::Size(), 3.0, 3.0, cv::INTER_CUBIC);
    cv::Mat thresholded;
    cv::threshold(scaled, thresholded, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    return thresholded;
}

cv::Mat UndistortStretchedCapture(
    const cv::Mat& inputMat,
    double currentAspectRatio,
    double nativeAspectRatio
) {
    if (inputMat.empty()) return inputMat;

    double scaleX = 1.0;
    if (std::isfinite(currentAspectRatio)
        && std::isfinite(nativeAspectRatio)
        && currentAspectRatio > 0.0
        && nativeAspectRatio > 0.0
        && std::abs(currentAspectRatio - nativeAspectRatio) > 0.000001) {
        // A narrower render stretched to a wider panel must be compressed back on X.
        scaleX = currentAspectRatio / nativeAspectRatio;
    }

    if (std::abs(scaleX - 1.0) <= 0.000001) return inputMat;

    const int targetWidth = std::max(1, static_cast<int>(std::round(inputMat.cols * scaleX)));
    try {
        cv::Mat corrected;
        cv::resize(
            inputMat,
            corrected,
            cv::Size(targetWidth, inputMat.rows),
            0.0,
            0.0,
            cv::INTER_LANCZOS4
        );
        return corrected.empty() ? inputMat : corrected;
    } catch (const cv::Exception& ex) {
        Log(std::string("Warning: aspect correction failed; using original OCR frame. ") + ex.what());
        return inputMat;
    } catch (const std::exception& ex) {
        Log(std::string("Warning: aspect correction failed; using original OCR frame. ") + ex.what());
        return inputMat;
    } catch (...) {
        Log("Warning: aspect correction failed; using original OCR frame.");
        return inputMat;
    }
}

std::vector<cv::Mat> PrepareOcrCandidates(
    const cv::Mat& frame,
    double currentAspectRatio,
    double nativeAspectRatio
) {
    std::vector<cv::Mat> candidates;
    candidates.push_back(PrepareForOcr(frame));

    cv::Mat aspectCorrected = UndistortStretchedCapture(frame, currentAspectRatio, nativeAspectRatio);
    if (!aspectCorrected.empty()
        && (aspectCorrected.cols != frame.cols || aspectCorrected.rows != frame.rows)) {
        candidates.push_back(PrepareForOcr(aspectCorrected));
    }

    cv::Mat inverted;
    cv::bitwise_not(candidates.front(), inverted);
    candidates.push_back(inverted);
    return candidates;
}

class OcrEngine {
public:
    explicit OcrEngine(const OverlayConfig& config) {
        if (api_.Init(config.tessdataPath.c_str(), config.language.c_str()) != 0) {
            throw std::runtime_error("Tesseract OCR failed to initialize.");
        }
        api_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
        api_.SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ-");
    }

    std::pair<std::string, int> ReadText(const cv::Mat& image) {
        api_.SetImage(image.data, image.cols, image.rows, 1, (int)image.step);
        char* raw = api_.GetUTF8Text();
        std::string text = raw ? raw : "";
        if (raw) delete[] raw;
        text = Trim(text);
        return {ToLowerAscii(NormalizeText(text)), api_.MeanTextConf()};
    }

private:
    static std::string NormalizeText(const std::string& value) {
        std::string folded = FoldOperatorTextToAscii(value);
        std::string out;
        for (unsigned char ch : folded) {
            if (std::isalnum(ch)) out.push_back((char)std::tolower(ch));
            else if (!out.empty() && out.back() != ' ') out.push_back(' ');
        }
        return Trim(out);
    }

    tesseract::TessBaseAPI api_;
};

struct OcrRead {
    std::string text;
    int confidence = 0;
};

OcrRead ReadBestText(OcrEngine& ocr, const std::vector<cv::Mat>& candidates) {
    OcrRead best;
    for (const cv::Mat& candidate : candidates) {
        auto [text, confidence] = ocr.ReadText(candidate);
        if (!text.empty() && (best.text.empty() || confidence > best.confidence)) {
            best = {text, confidence};
        }
    }
    return best;
}

struct OperatorMatch {
    std::string name;
    bool exact = false;
    int distance = 0;
};

std::string ToUpperAscii(std::string value);
std::string ToTitleAscii(std::string value);

std::string CompactText(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    std::string folded = FoldOperatorTextToAscii(value);
    for (unsigned char ch : folded) {
        if (std::isalnum(ch)) out.push_back((char)std::tolower(ch));
    }
    return out;
}

std::string processAndValidateOCR(
    const cv::Mat& rawCapture,
    double currentRatio,
    double nativeRatio,
    const std::vector<std::string>& validConfigWords
) {
    try {
        if (rawCapture.empty() || validConfigWords.empty()) {
            return "";
        }

        auto cleanText = [](const std::string& value) {
            return CompactText(ToLowerAscii(Trim(FoldOperatorTextToAscii(value))));
        };

        std::unordered_map<std::string, std::string> validWords;
        validWords.reserve(validConfigWords.size());
        for (const std::string& word : validConfigWords) {
            const std::string cleaned = cleanText(word);
            if (!cleaned.empty()) {
                validWords.emplace(cleaned, word);
            }
        }
        if (validWords.empty()) {
            return "";
        }

        auto runOcr = [](const cv::Mat& image) -> std::string {
            if (image.empty()) return "";

            thread_local std::unique_ptr<tesseract::TessBaseAPI> api;
            thread_local std::string loadedLanguage;
            thread_local std::string loadedTessdata;
            if (!api
                || loadedLanguage != g_processOcrLanguage
                || loadedTessdata != g_processOcrTessdataPath) {
                api = std::make_unique<tesseract::TessBaseAPI>();
                if (api->Init(g_processOcrTessdataPath.c_str(), g_processOcrLanguage.c_str()) != 0) {
                    api.reset();
                    throw std::runtime_error("Tesseract OCR failed to initialize in self-healing pipeline.");
                }
                loadedLanguage = g_processOcrLanguage;
                loadedTessdata = g_processOcrTessdataPath;
                api->SetPageSegMode(tesseract::PSM_SINGLE_LINE);
                api->SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ-");
            }

            cv::Mat prepared;
            if (image.channels() == 1 || image.channels() == 3 || image.channels() == 4) {
                prepared = image.isContinuous() ? image : image.clone();
            } else {
                image.convertTo(prepared, CV_8U);
            }

            if (prepared.empty()) return "";
            api->SetImage(
                prepared.data,
                prepared.cols,
                prepared.rows,
                prepared.channels(),
                static_cast<int>(prepared.step)
            );
            char* rawText = api->GetUTF8Text();
            std::string text = rawText ? rawText : "";
            if (rawText) delete[] rawText;
            text = Trim(text);
            return text;
        };

        auto exactMatch = [&](const std::string& ocrText) -> std::string {
            const std::string cleaned = cleanText(ocrText);
            if (cleaned.empty()) return "";
            auto exact = validWords.find(cleaned);
            return exact == validWords.end() ? std::string{} : exact->second;
        };

        cv::Mat unstretched = rawCapture;
        double scaleX = 1.0;
        if (std::isfinite(currentRatio)
            && std::isfinite(nativeRatio)
            && currentRatio > 0.0
            && nativeRatio > 0.0
            && std::abs(currentRatio - nativeRatio) > 0.000001) {
            // Horizontal-only correction restores 4:3-stretched text geometry on a 16:9 panel.
            scaleX = currentRatio / nativeRatio;
        }
        if (std::abs(scaleX - 1.0) > 0.000001) {
            const int targetWidth = std::max(1, static_cast<int>(std::round(rawCapture.cols * scaleX)));
            cv::resize(rawCapture, unstretched, cv::Size(targetWidth, rawCapture.rows), 0.0, 0.0, cv::INTER_LANCZOS4);
        }

        if (std::string match = exactMatch(runOcr(unstretched)); !match.empty()) {
            return match;
        }

        auto toGray = [](const cv::Mat& image) {
            cv::Mat gray;
            if (image.channels() == 1) {
                gray = image;
            } else if (image.channels() == 4) {
                cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
            } else {
                cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
            }
            return gray;
        };

        cv::Mat gray = toGray(unstretched);
        cv::Mat equalized;
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        clahe->apply(gray, equalized);

        cv::Mat enlarged;
        cv::resize(equalized, enlarged, cv::Size(), 3.0, 3.0, cv::INTER_CUBIC);

        cv::Mat sharpened;
        cv::GaussianBlur(enlarged, sharpened, cv::Size(0, 0), 0.85);
        cv::addWeighted(enlarged, 1.55, sharpened, -0.55, 0.0, sharpened);

        cv::Mat binary;
        cv::threshold(sharpened, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        cv::Mat connected;
        cv::morphologyEx(
            binary,
            connected,
            cv::MORPH_CLOSE,
            cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 1))
        );

        if (std::string match = exactMatch(runOcr(connected)); !match.empty()) {
            return match;
        }

        return "";
    } catch (const cv::Exception& ex) {
        Log(std::string("Self-healing OCR pipeline failed with OpenCV error: ") + ex.what());
        return "";
    } catch (const std::exception& ex) {
        Log(std::string("Self-healing OCR pipeline failed: ") + ex.what());
        return "";
    } catch (...) {
        Log("Self-healing OCR pipeline failed with an unknown error.");
        return "";
    }
}

std::vector<char> OcrConfusionReplacements(char ch) {
    static const std::vector<std::string> groups{
        "o0",
        "il1",
        "b8",
        "s5",
        "z2",
        "g6",
        "a4",
        "e3",
        "t7"
    };
    ch = (char)std::tolower((unsigned char)ch);
    for (const std::string& group : groups) {
        if (group.find(ch) == std::string::npos) {
            continue;
        }
        std::vector<char> replacements;
        for (char replacement : group) {
            if (replacement != ch) {
                replacements.push_back(replacement);
            }
        }
        return replacements;
    }
    return {};
}

std::vector<std::string> GenerateStrictOcrAliasesForOperator(const std::string& op) {
    const std::string base = CompactText(op);
    std::vector<std::string> aliases;
    std::unordered_set<std::string> seen;
    auto addAlias = [&](const std::string& alias) {
        const std::string compact = CompactText(alias);
        if (!compact.empty() && compact != base && seen.insert(compact).second) {
            aliases.push_back(compact);
        }
    };

    std::vector<size_t> replaceablePositions;
    for (size_t i = 0; i < base.size(); ++i) {
        if (!OcrConfusionReplacements(base[i]).empty()) {
            replaceablePositions.push_back(i);
        }
    }

    auto generateSubstitutions = [&](auto&& self, size_t start, int remaining, std::string alias) -> void {
        if (remaining == 0) {
            addAlias(alias);
            return;
        }
        for (size_t positionIndex = start; positionIndex < replaceablePositions.size(); ++positionIndex) {
            const size_t textIndex = replaceablePositions[positionIndex];
            for (char replacement : OcrConfusionReplacements(base[textIndex])) {
                std::string next = alias;
                next[textIndex] = replacement;
                self(self, positionIndex + 1, remaining - 1, next);
            }
        }
    };

    const int maxSubstitutions = std::min<int>(3, (int)replaceablePositions.size());
    for (int count = 1; count <= maxSubstitutions; ++count) {
        generateSubstitutions(generateSubstitutions, 0, count, base);
    }

    if (base.size() >= 5) {
        for (size_t i = 0; i < base.size(); ++i) {
            std::string alias = base;
            alias.erase(i, 1);
            addAlias(alias);
        }
    }

    return aliases;
}

std::unordered_map<std::string, std::string> BuildOperatorAliasMap(const std::vector<std::string>& operators) {
    std::unordered_map<std::string, std::string> aliases;
    std::unordered_set<std::string> ambiguous;
    std::unordered_set<std::string> exactOperators;
    for (const std::string& op : operators) {
        exactOperators.insert(CompactText(op));
    }

    auto add = [&](const std::string& alias, const std::string& op) {
        if (std::find(operators.begin(), operators.end(), op) != operators.end()) {
            const std::string compact = CompactText(alias);
            if (compact.empty()) {
                return;
            }
            if (exactOperators.find(compact) != exactOperators.end() && compact != op) {
                ambiguous.insert(compact);
                aliases.erase(compact);
                return;
            }
            auto existing = aliases.find(compact);
            if (existing != aliases.end() && existing->second != op) {
                ambiguous.insert(compact);
                aliases.erase(existing);
                return;
            }
            if (ambiguous.find(compact) == ambiguous.end()) {
                aliases[compact] = op;
            }
        }
    };

    for (const std::string& op : operators) {
        add(op, op);
        add(ToUpperAscii(op), op);
        add(ToTitleAscii(op), op);
        for (const std::string& alias : GenerateStrictOcrAliasesForOperator(op)) {
            add(alias, op);
        }
    }

    add("capitão", "capitao");
    add("CAPITÃO", "capitao");
    add("capitao", "capitao");
    add("capita0", "capitao");
    add("capitao", "capitao");

    add("iq", "iq");
    add("IQ", "iq");
    add("1q", "iq");
    add("lq", "iq");
    add("LQ", "iq");
    add("i0", "iq");
    add("IO", "iq");

    add("glaz", "glaz");
    add("GLAZ", "glaz");
    add("gla2", "glaz");
    add("GIAZ", "glaz");
    add("6LAZ", "glaz");

    add("iana", "iana");
    add("IANA", "iana");
    add("lana", "iana");
    add("LANA", "iana");
    add("1ana", "iana");
    add("lANA", "iana");

    add("nokk", "nokk");
    add("nøkk", "nokk");
    add("NØKK", "nokk");
    add("n0kk", "nokk");

    add("tubarão", "tubarao");
    add("TUBARÃO", "tubarao");
    add("tubarao", "tubarao");
    add("tubara0", "tubarao");

    add("jäger", "jager");
    add("JÄGER", "jager");
    add("jager", "jager");

    add("dokkaebi", "dokkaebi");
    add("DOKKAEBI", "dokkaebi");
    add("dokkaeb1", "dokkaebi");
    add("DOKKAEB1", "dokkaebi");
    add("dokkaebl", "dokkaebi");
    add("DOKKAEBL", "dokkaebi");
    add("d0kkaebi", "dokkaebi");
    add("D0KKAEBI", "dokkaebi");
    add("d0kkaeb1", "dokkaebi");
    add("D0KKAEB1", "dokkaebi");
    add("d0kkaebl", "dokkaebi");
    add("D0KKAEBL", "dokkaebi");
    add("dokkaeb", "dokkaebi");
    add("DOKKAEB", "dokkaebi");

    add("alibi", "alibi");
    add("ALIBI", "alibi");
    add("alibl", "alibi");
    add("ALIBL", "alibi");
    add("ali8i", "alibi");
    add("ALI8I", "alibi");
    add("al1bi", "alibi");
    add("AL1BI", "alibi");
    add("al1b1", "alibi");
    add("AL1B1", "alibi");
    add("allbl", "alibi");
    add("ALLBL", "alibi");
    add("allbi", "alibi");
    add("ALLBI", "alibi");
    add("a1ibi", "alibi");
    add("A1IBI", "alibi");

    return aliases;
}

std::optional<OperatorMatch> MatchOperator(const std::string& text, const std::vector<std::string>& operators) {
    if (text.empty()) return std::nullopt;
    const std::string compact = CompactText(text);
    for (const std::string& op : operators) {
        if (compact == op) {
            return OperatorMatch{op, true, 0};
        }
    }
    const auto aliases = BuildOperatorAliasMap(operators);
    auto alias = aliases.find(compact);
    if (alias != aliases.end()) {
        return OperatorMatch{alias->second, true, 0};
    }
    return std::nullopt;
}

bool DetectionConfidentEnough(const OcrRead& read, const OperatorMatch& op, int minimumOcrConfidence) {
    (void)minimumOcrConfidence;
    return op.exact && (op.name.size() >= 4 || read.confidence >= 20);
}

int MatchScore(const OcrRead& read, const OperatorMatch& op, int minimumOcrConfidence) {
    int score = op.exact ? 10000 : 7000;
    score -= op.distance * 1000;
    score += std::clamp(read.confidence, 0, 100);
    if (read.confidence >= minimumOcrConfidence) score += 500;
    return score;
}

struct DetectionRead {
    OcrRead read;
    OperatorMatch match;
    bool hasMatch = false;
    int score = 0;
};

DetectionRead ReadBestDetection(
    OcrEngine& ocr,
    const std::vector<cv::Mat>& candidates,
    const std::vector<std::string>& operators,
    int minimumOcrConfidence
) {
    DetectionRead best;
    int secondBestDifferentOperatorScore = 0;
    OcrRead bestRaw;
    for (const cv::Mat& candidate : candidates) {
        auto [text, confidence] = ocr.ReadText(candidate);
        OcrRead read{text, confidence};
        if (!read.text.empty() && (bestRaw.text.empty() || read.confidence > bestRaw.confidence)) {
            bestRaw = read;
        }

        std::optional<OperatorMatch> match = MatchOperator(read.text, operators);
        if (!match || !DetectionConfidentEnough(read, *match, minimumOcrConfidence)) {
            continue;
        }

        const int score = MatchScore(read, *match, minimumOcrConfidence);
        if (!best.hasMatch || score > best.score) {
            if (best.hasMatch && best.match.name != match->name) {
                secondBestDifferentOperatorScore = std::max(secondBestDifferentOperatorScore, best.score);
            }
            best = DetectionRead{read, *match, true, score};
        } else if (best.hasMatch && best.match.name != match->name) {
            secondBestDifferentOperatorScore = std::max(secondBestDifferentOperatorScore, score);
        }
    }

    if (best.hasMatch && secondBestDifferentOperatorScore > 0) {
        const int requiredMargin = best.match.exact ? 250 : 1200;
        if (best.score - secondBestDifferentOperatorScore < requiredMargin) {
            return DetectionRead{best.read, best.match, false, best.score};
        }
    }

    if (!best.hasMatch) {
        best.read = bestRaw;
    }
    return best;
}

std::string JsonEscape(const std::string& value) {
    std::string out;
    for (char ch : value) {
        if (ch == '\\') out += "\\\\";
        else if (ch == '"') out += "\\\"";
        else out.push_back(ch);
    }
    return out;
}

bool PostOperatorDetectionToPort(int port, const std::string& operatorName, int confidence) {
    HINTERNET session = WinHttpOpen(L"VisionOverlay/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        Log("Post failed on port " + std::to_string(port) + ": WinHttpOpen failed.");
        return false;
    }
    HINTERNET connect = WinHttpConnect(session, L"127.0.0.1", (INTERNET_PORT)port, 0);
    if (!connect) {
        WinHttpCloseHandle(session);
        Log("Post failed on port " + std::to_string(port) + ": WinHttpConnect failed.");
        return false;
    }
    HINTERNET request = WinHttpOpenRequest(connect, L"POST", L"/operator", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!request) {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        Log("Post failed on port " + std::to_string(port) + ": WinHttpOpenRequest failed.");
        return false;
    }

    std::ostringstream body;
    body << "{\"operator\":\"" << JsonEscape(operatorName) << "\",\"confidence\":" << confidence << "}";
    std::string payload = body.str();
    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL ok = WinHttpSendRequest(request, headers.c_str(), (DWORD)-1L, payload.data(), (DWORD)payload.size(), (DWORD)payload.size(), 0);
    ok = ok && WinHttpReceiveResponse(request, nullptr);
    DWORD status = 0;
    DWORD size = sizeof(status);
    if (ok) WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &size, nullptr);
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    const bool accepted = ok && status >= 200 && status < 300;
    if (!accepted) {
        Log("Post failed on port " + std::to_string(port) + ": status=" + std::to_string(status));
    }
    return accepted;
}

bool PostOperatorDetection(const std::string& operatorName, int confidence) {
    const bool uiOk = PostOperatorDetectionToPort(VISIBLE_UI_HTTP_PORT, operatorName, confidence);
    const bool backendOk = PostOperatorDetectionToPort(MAIN_APP_HTTP_PORT, operatorName, confidence);
    return uiOk || backendOk;
}

std::string ToUpperAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return (char)std::toupper(c); });
    return value;
}

std::string ToTitleAscii(std::string value) {
    value = ToLowerAscii(value);
    if (!value.empty()) value[0] = (char)std::toupper((unsigned char)value[0]);
    return value;
}

bool RunOperatorMatchSelfTest() {
    std::vector<std::string> operators(std::begin(DEFAULT_OPERATORS), std::end(DEFAULT_OPERATORS));
    bool ok = true;
    for (const std::string& op : operators) {
        const std::vector<std::string> variants{op, ToUpperAscii(op), ToTitleAscii(op)};
        for (const std::string& variant : variants) {
            std::optional<OperatorMatch> match = MatchOperator(variant, operators);
            if (!match || match->name != op) {
                Log("Self-test failed: " + variant + " mapped to " + (match ? match->name : std::string("<none>")) + ", expected " + op);
                ok = false;
            }
        }
    }

    const auto aliases = BuildOperatorAliasMap(operators);
    std::unordered_map<std::string, int> generatedAliasCounts;
    for (const auto& [alias, expected] : aliases) {
        std::optional<OperatorMatch> match = MatchOperator(alias, operators);
        if (!match || match->name != expected) {
            Log("Self-test failed: generated alias " + alias + " mapped to " + (match ? match->name : std::string("<none>")) + ", expected " + expected);
            ok = false;
        }
        if (alias != expected) {
            ++generatedAliasCounts[expected];
        }
    }
    for (const std::string& op : operators) {
        if (generatedAliasCounts[op] <= 0) {
            Log("Self-test failed: no strict OCR aliases generated for " + op);
            ok = false;
        }
    }

    const std::vector<std::pair<std::string, std::string>> ocrVariants{
        {"CAPITÃO", "capitao"},
        {"Capitão", "capitao"},
        {"CAPITA0", "capitao"},
        {"1Q", "iq"},
        {"LQ", "iq"},
        {"IO", "iq"},
        {"GLA2", "glaz"},
        {"GIAZ", "glaz"},
        {"LANA", "iana"},
        {"1ANA", "iana"},
        {"NØKK", "nokk"},
        {"N0KK", "nokk"},
        {"TUBARÃO", "tubarao"},
        {"TUBARA0", "tubarao"},
        {"JÄGER", "jager"},
        {"VIGIL", "vigil"},
        {"V1GIL", "vigil"},
        {"V1G1L", "vigil"},
        {"V1G11", "vigil"},
        {"VlGlL", "vigil"},
        {"VIG1L", "vigil"},
        {"DOKKAEB1", "dokkaebi"},
        {"DOKKAEBL", "dokkaebi"},
        {"D0KKAEBI", "dokkaebi"},
        {"D0KKAEB1", "dokkaebi"},
        {"DOKKAEB", "dokkaebi"},
        {"ALIBL", "alibi"},
        {"ALI8I", "alibi"},
        {"AL1BI", "alibi"},
        {"AL1B1", "alibi"},
        {"ALLBL", "alibi"},
        {"A1IBI", "alibi"}
    };
    for (const auto& [variant, expected] : ocrVariants) {
        std::optional<OperatorMatch> match = MatchOperator(variant, operators);
        if (!match || match->name != expected) {
            Log("Self-test failed: OCR variant " + variant + " mapped to " + (match ? match->name : std::string("<none>")) + ", expected " + expected);
            ok = false;
        }
    }
    Log(ok ? "Self-test passed: all 76 exact operator names and strict OCR aliases map correctly." : "Self-test failed.");
    return ok;
}

int RunOverlay(const OverlayConfig& config) {
    g_processOcrLanguage = config.language;
    g_processOcrTessdataPath = config.tessdataPath;
    CaptureManager capture(config.region);
    OcrEngine ocr(config);
    const auto frameInterval = std::chrono::milliseconds(1000 / std::max(1, config.fps));
    const auto cooldown = std::chrono::milliseconds(config.cooldownMs);
    auto nextAllowedUpdate = std::chrono::steady_clock::time_point{};
    auto nextRejectedLog = std::chrono::steady_clock::time_point{};
    std::string lastOperator;
    std::string pendingOperator;
    int pendingCount = 0;

    while (g_running) {
        auto frameStart = std::chrono::steady_clock::now();
        if (config.pauseWhenCursorHidden && !capture.IsCursorVisible()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            continue;
        }

        cv::Mat rawCapture = capture.CaptureBgr();
        const std::string healedOperator = processAndValidateOCR(
            rawCapture,
            config.currentAspectRatio,
            config.nativeAspectRatio,
            config.operators
        );
        DetectionRead detection;
        if (!healedOperator.empty()) {
            detection = DetectionRead{
                OcrRead{healedOperator, 100},
                OperatorMatch{healedOperator, true, 0},
                true,
                11000
            };
        } else {
            detection = ReadBestDetection(
                ocr,
                PrepareOcrCandidates(
                    rawCapture,
                    config.currentAspectRatio,
                    config.nativeAspectRatio
                ),
                config.operators,
                config.minimumOcrConfidence
            );
        }
        if (detection.hasMatch) {
            if (detection.match.name == pendingOperator) {
                ++pendingCount;
            } else {
                pendingOperator = detection.match.name;
                pendingCount = 1;
            }
        } else {
            pendingOperator.clear();
            pendingCount = 0;
        }

        const int requiredConfirmations = 3;
        if (detection.hasMatch
            && pendingCount >= requiredConfirmations
            && (frameStart >= nextAllowedUpdate || detection.match.name != lastOperator)) {
            if (PostOperatorDetection(detection.match.name, detection.read.confidence)) {
                lastOperator = detection.match.name;
                nextAllowedUpdate = frameStart + cooldown;
                pendingCount = 0;
                Log(
                    "Detected operator: " + detection.match.name
                    + " conf=" + std::to_string(detection.read.confidence)
                    + " distance=" + std::to_string(detection.match.distance)
                    + " confirmations=" + std::to_string(requiredConfirmations)
                    + " text=" + detection.read.text
                );
            }
        } else if (!detection.read.text.empty() && frameStart >= nextRejectedLog) {
            Log("OCR read rejected: conf=" + std::to_string(detection.read.confidence) + " text=" + detection.read.text);
            nextRejectedLog = frameStart + std::chrono::seconds(2);
        }

        auto elapsed = std::chrono::steady_clock::now() - frameStart;
        if (elapsed < frameInterval) std::this_thread::sleep_for(frameInterval - elapsed);
    }
    return 0;
}

fs::path ConfigPathFromArgs(int argc, wchar_t** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::wstring(argv[i]) == L"--config") return fs::path(argv[i + 1]);
    }
    return fs::path(CurrentExeDir()) / DEFAULT_CONFIG_FILENAME;
}

OverlayConfig WaitForConfiguredRegion(const fs::path& configPath) {
    auto nextLog = std::chrono::steady_clock::time_point{};
    while (g_running) {
        OverlayConfig config = LoadConfig(configPath);
        if (HasUsableRegion(config)) {
            Log(
                "Vision overlay loaded saved region: x=" + std::to_string(config.region.x)
                + " y=" + std::to_string(config.region.y)
                + " w=" + std::to_string(config.region.width)
                + " h=" + std::to_string(config.region.height)
            );
            return config;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= nextLog) {
            Log("Vision overlay idle: waiting for region_configured=true in " + configPath.string());
            nextLog = now + std::chrono::seconds(5);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return LoadConfig(configPath);
}

bool HasArg(int argc, wchar_t** argv, const std::wstring& wanted) {
    for (int i = 1; i < argc; ++i) {
        if (std::wstring(argv[i]) == wanted) return true;
    }
    return false;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    EnableProcessDpiAwareness();
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    try {
        if (HasArg(argc, argv, L"--self-test")) {
            if (argv) LocalFree(argv);
            return RunOperatorMatchSelfTest() ? 0 : 2;
        }
        g_singleInstanceMutex = CreateMutexW(nullptr, TRUE, L"Global\\NexusVisionAutomationToolSingleton");
        if (g_singleInstanceMutex != nullptr && GetLastError() == ERROR_ALREADY_EXISTS) {
            Log("Vision overlay exited: another monitor instance is already running.");
            if (argv) LocalFree(argv);
            CloseHandle(g_singleInstanceMutex);
            return 0;
        }
        fs::path configPath = ConfigPathFromArgs(argc, argv);
        if (argv) LocalFree(argv);
        OverlayConfig config = WaitForConfiguredRegion(configPath);
        if (!HasUsableRegion(config)) return 0;
        Log("Vision overlay started with " + std::to_string(config.operators.size()) + " operators.");
        const int result = RunOverlay(config);
        if (g_singleInstanceMutex) CloseHandle(g_singleInstanceMutex);
        return result;
    } catch (const std::exception& error) {
        Log(std::string("Fatal error: ") + error.what());
        if (argv) LocalFree(argv);
        if (g_singleInstanceMutex) CloseHandle(g_singleInstanceMutex);
        return 1;
    }
}

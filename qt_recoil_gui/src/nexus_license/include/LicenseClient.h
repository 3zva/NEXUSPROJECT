#pragma once

#include "LicenseTypes.h"

#include <chrono>
#include <string>
#include <string_view>

namespace nexus::license {

class LicenseClient {
public:
    explicit LicenseClient(
        std::wstring apiBaseUrl,
        std::chrono::milliseconds timeout = std::chrono::seconds(10));

    [[nodiscard]] LicenseResult Activate(std::string_view licenseKey) const;
    [[nodiscard]] LicenseResult Validate(std::string_view licenseKey) const;

private:
    [[nodiscard]] LicenseResult Request(
        std::wstring_view endpoint,
        std::string_view licenseKey) const;

    std::wstring apiBaseUrl_;
    std::chrono::milliseconds timeout_;
};

} // namespace nexus::license

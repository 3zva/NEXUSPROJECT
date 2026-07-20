#pragma once

#include <optional>
#include <stdexcept>
#include <string>

namespace nexus::license {

struct LicenseResult {
    bool valid{false};
    std::string licenseType;
    std::string status;
    std::string activatedAt;
    std::string expiresAt;
    std::string serverTime;
    std::string validationToken;
    std::string tokenExpiresAt;
    std::string message;
    std::string code;
};

class LicenseException final : public std::runtime_error {
public:
    explicit LicenseException(const std::string& message)
        : std::runtime_error(message) {}
};

} // namespace nexus::license

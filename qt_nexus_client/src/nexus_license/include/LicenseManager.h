#pragma once

#include "LicenseClient.h"
#include "SecureLicenseStore.h"

#include <optional>
#include <string_view>

namespace nexus::license {

class LicenseManager {
public:
    LicenseManager(LicenseClient client, SecureLicenseStore store = SecureLicenseStore{});

    // Activates a newly entered key and stores it with Windows DPAPI only if
    // the server confirms that it is valid for this device.
    [[nodiscard]] LicenseResult ActivateAndSave(std::string_view licenseKey);

    // Validates the securely stored key on app startup. Returns std::nullopt
    // when no saved key exists.
    [[nodiscard]] std::optional<LicenseResult> ValidateSaved() const;

    void ClearSavedLicense() const;

private:
    LicenseClient client_;
    SecureLicenseStore store_;
};

} // namespace nexus::license

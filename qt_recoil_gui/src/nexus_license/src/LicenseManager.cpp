#include "LicenseManager.h"

#include <string>
#include <utility>

namespace nexus::license {

LicenseManager::LicenseManager(LicenseClient client, SecureLicenseStore store)
    : client_(std::move(client)), store_(std::move(store)) {}

LicenseResult LicenseManager::ActivateAndSave(std::string_view licenseKey) {
    LicenseResult result = client_.Activate(licenseKey);
    if (result.valid) store_.Save(std::string(licenseKey));
    return result;
}

std::optional<LicenseResult> LicenseManager::ValidateSaved() const {
    const auto key = store_.Load();
    if (!key) return std::nullopt;
    return client_.Validate(*key);
}

void LicenseManager::ClearSavedLicense() const {
    store_.Remove();
}

} // namespace nexus::license

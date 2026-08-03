#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace nexus::license {

class SecureLicenseStore {
public:
    // Default path: %LOCALAPPDATA%\HighCloudNEXUS\NEXUS\license.dat
    explicit SecureLicenseStore(std::filesystem::path storagePath = {});

    void Save(const std::string& licenseKey) const;
    [[nodiscard]] std::optional<std::string> Load() const;
    void Remove() const;
    [[nodiscard]] const std::filesystem::path& Path() const noexcept;

private:
    std::filesystem::path storagePath_;
};

} // namespace nexus::license

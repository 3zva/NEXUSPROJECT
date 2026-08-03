#include "LicenseClient.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: license_probe.exe <worker-url> <license-key>\n";
        return 2;
    }

    try {
        const std::string urlUtf8 = argv[1];
        const std::wstring url(urlUtf8.begin(), urlUtf8.end());
        nexus::license::LicenseClient client(url);
        std::cout << client.DebugActivateRaw(argv[2]) << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}

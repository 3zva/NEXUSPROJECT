#pragma once

#include <string>

namespace nexus::license {

// Returns a privacy-reduced, lowercase SHA-256 identifier derived locally from
// the Windows MachineGuid. The raw MachineGuid is never sent to the server.
std::string GetHashedDeviceId();

} // namespace nexus::license

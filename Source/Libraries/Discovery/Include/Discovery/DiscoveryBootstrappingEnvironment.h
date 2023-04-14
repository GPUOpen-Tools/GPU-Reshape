#pragma once

// Std
#include <vector>
#include <string>

struct DiscoveryBootstrappingEnvironment {
    /// All environment keys to be added
    std::vector<std::pair<std::string, std::string>> environmentKeys;

    /// All dlls to be injected
    std::vector<std::string> dlls;
};

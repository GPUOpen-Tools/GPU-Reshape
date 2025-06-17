// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <Services/Loader/Loader.h>
#include <Services/Discovery/DiscoveryService.h>
#include <Services/HostResolver/HostResolverService.h>

// Message
#include <Message/MessageStream.h>

// Schemas
#include <Schemas/PDB.h>

// Std
#include <iostream>

/// Per-process reserved token
static GlobalUID GProcessReservedToken = GlobalUID::New();

static void CreateSymbolStartupEnvironment(const GRSLoaderInstallInfo* info, MessageStreamView<>& view) {
    // Set config
    auto* config = view.Add<SetPDBConfigMessage>();
    config->recursive = info->symbol.includeSubDirectories;
    config->pathCount = info->symbol.pathCount;

    // Push all paths
    for (uint32_t i = 0; i < info->symbol.pathCount; i++) {
        auto* path = view.Add<SetPDBPathMessage>(SetPDBPathMessage::AllocationInfo { .pathLength = std::strlen(info->symbol.paths[i]) });
        path->path.Set(info->symbol.paths[i]);
    }

    // Index all paths
    view.Add<IndexPDPathsMessage>();
}

DLL_EXPORT_C bool GRSLoaderInstall(const GRSLoaderInstallInfo* info) {
    // Install the system wide host resolver
    HostResolverService resolverService;
    if (!resolverService.Install()) {
        std::cerr << "[GRS] Failed to install the host resolver service" << std::endl;
        return false;
    }

    // Install the process discovery services
    DiscoveryService discoveryService;
    if (!discoveryService.Install()) {
        std::cerr << "[GRS] Failed to install the discovery service" << std::endl;
        return false;
    }

    // Warn the user in case that there are multiple instances
    if (discoveryService.HasConflictingInstances()) {
        std::cerr << "[GRS] Conflicting discovery instances detected, please clean up local environment" << std::endl;
    }

    // Assign token
    DiscoveryProcessLocalInfo localInfo;
    localInfo.reservedToken = GProcessReservedToken;

    // Startup environment
    MessageStream stream;

    // Setup config env
    MessageStreamView<> view(stream);
    CreateSymbolStartupEnvironment(info, view);

    // Finally, install GRS locally
    if (!discoveryService.InstallLocal(localInfo, stream)) {
        std::cerr << "[GRS] Failed to install local discovery services" << std::endl;
        return false;
    }

    // OK
    return true;
}

DLL_EXPORT_C void GRSLoaderGetReservedToken(char* output, uint32_t* length) {
    std::string str = GProcessReservedToken.ToString();
    if (output) {
        std::memcpy(output, str.data(), *length);
    } else {
        *length = static_cast<unsigned int>(str.length()) + 1;
    }
}

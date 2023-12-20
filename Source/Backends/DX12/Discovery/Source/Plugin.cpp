// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

// Common
#include <Common/Registry.h>
#include <Common/Plugin/Plugin.h>
#include <Common/Plugin/PluginInfo.h>

// Discovery
#include <Discovery/IDiscoveryHost.h>

// DX12
#include <Backends/DX12/DX12DiscoveryListener.h>

static ComRef<DX12DiscoveryListener> listener{nullptr};

DLL_EXPORT_C void PLUGIN_INFO(PluginInfo* info) {
    info->name = "DX12Discovery";
    info->description = "Application discovery for DX12";
}

DLL_EXPORT_C bool PLUGIN_INSTALL(Registry* registry) {
    auto host = registry->Get<IDiscoveryHost>();
    if (!host) {
        return false;
    }

    // Install the listener
    listener = registry->New<DX12DiscoveryListener>();
    host->Register(listener);

    // OK
    return true;
}

DLL_EXPORT_C void PLUGIN_UNINSTALL(Registry* registry) {
    auto host = registry->Get<IDiscoveryHost>();
    if (!host) {
        return;
    }

    // Uninstall the listener
    host->Deregister(listener);
    listener.Release();
}

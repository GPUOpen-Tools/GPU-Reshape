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

// Common
#include <Common/Registry.h>
#include <Common/ComponentTemplate.h>
#include <Common/Plugin/Plugin.h>
#include <Common/Plugin/PluginInfo.h>

// Backend
#include <Backend/IFeatureHost.h>
#include <Backend/StartupContainer.h>
#include <Backend/ShaderData/IShaderDataHost.h>

// Message
#include <Message/MessageStreamCommon.h>

// Schemas
#include <Schemas/ConfigCommon.h>

// Initialization
#include <Features/Initialization/TexelAddressingFeature.h>
#include <Features/Initialization/ResourceAddressingFeature.h>

class InitializationComponentTemplate final : public IComponentTemplate {
public:
    ComRef<> Instantiate(Registry* registry) {
        // Get components
        auto startup = registry->Get<Backend::StartupContainer>();
        auto dataHost = registry->Get<IShaderDataHost>();

        // Is texel addressing enabled?
        SetTexelAddressingMessage config = CollapseOrDefault<SetTexelAddressingMessage>(startup->GetView(), SetTexelAddressingMessage { .enabled = true });

        // Create feature
        // Check that tiled resources is supported at all
        if (config.enabled && dataHost->GetCapabilityTable().supportsTiledResources) {
            return registry->New<TexelAddressingInitializationFeature>();
        } else {
            return registry->New<ResourceAddressingInitializationFeature>();
        }
    }
};

static ComRef<InitializationComponentTemplate> feature{nullptr};

DLL_EXPORT_C void PLUGIN_INFO(PluginInfo* info) {
    info->name = "Initialization";
    info->description = "Instrumentation and validation of resource initialization prior to reads";
}

DLL_EXPORT_C bool PLUGIN_INSTALL(Registry* registry) {
    auto host = registry->Get<IFeatureHost>();
    if (!host) {
        return false;
    }

    // Install the Initialization feature
    feature = registry->New<InitializationComponentTemplate>();
    host->Register(feature);

    // OK
    return true;
}

DLL_EXPORT_C void PLUGIN_UNINSTALL(Registry* registry) {
    auto host = registry->Get<IFeatureHost>();
    if (!host) {
        return;
    }

    // Uninstall the feature
    host->Deregister(feature);
    feature.Release();
}

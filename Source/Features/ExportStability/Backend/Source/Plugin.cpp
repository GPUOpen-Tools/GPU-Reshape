// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

// ExportStability
#include <Features/ExportStability/Feature.h>

static ComRef<ComponentTemplate<ExportStabilityFeature>> feature{nullptr};

DLL_EXPORT_C void PLUGIN_INFO(PluginInfo* info) {
    info->name = "ExportStability";
    info->description = "Instrumentation and validation of exporting operations";
}

DLL_EXPORT_C bool PLUGIN_INSTALL(Registry* registry) {
    auto host = registry->Get<IFeatureHost>();
    if (!host) {
        return false;
    }

    // Install the resource bounds feature
    feature = registry->New<ComponentTemplate<ExportStabilityFeature>>();
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

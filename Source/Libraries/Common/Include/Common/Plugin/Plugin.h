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

#pragma once

// Common
#include <Common/Common.h>

// Forward declarations
class Registry;

// Forward declarations
struct PluginInfo;

/// Plugin info type
using PluginInfoDelegate = void(*)(PluginInfo* info);

/// Plugin installation type
using PluginInstallDelegate = bool(*)(Registry* registry);

/// Plugin de-installation type
using PluginUninstallDelegate = void(*)(Registry* registry);

/// Plugin entrypoint names
#define PLUGIN_INFO GetPluginInfo
#define PLUGIN_INSTALL InstallPlugin
#define PLUGIN_UNINSTALL UninstallPlugin

/// Plugin entrypoint strings
#define PLUGIN_INFO_S "GetPluginInfo"
#define PLUGIN_INSTALL_S "InstallPlugin"
#define PLUGIN_UNINSTALL_S "UninstallPlugin"

// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Discovery
#include "DiscoveryProcessInfo.h"

// Common
#include <Common/IComponent.h>

// Forward declarations
struct DiscoveryBootstrappingEnvironment;

class IDiscoveryListener : public TComponent<IDiscoveryListener> {
public:
    COMPONENT(IDiscoveryListener);

    /// Check if this discovery is running
    /// \return if running, returns true
    virtual bool IsRunning() = 0;

    /// Check if this discovery is installed globally
    /// \return if installed, returns true
    virtual bool IsGloballyInstalled() = 0;

    /// Starts this listener
    /// \return success state
    virtual bool Start() = 0;

    /// Stops this listener
    /// \return success state
    virtual bool Stop() = 0;

    /// Setup the expected bootstrapping environment
    /// \param info process information
    /// \param environment destination environment
    virtual void SetupBootstrappingEnvironment(const DiscoveryProcessInfo& info, DiscoveryBootstrappingEnvironment& environment) = 0;

    /// Install this listener
    ///   ? Enables global hooking of respective discovery, always on for the end user
    /// \return success state
    virtual bool InstallGlobal() = 0;

    /// Uninstall this listener
    ///   ? Disables global hooking of respective discovery
    /// \return success state
    virtual bool UninstallGlobal() = 0;

    /// Check if conflicting instances are installed
    /// \return true if any are installed
    virtual bool HasConflictingInstances() = 0;

    /// Uninstall any conflicting instance
    /// \return false if failed
    virtual bool UninstallConflictingInstances() = 0;
};

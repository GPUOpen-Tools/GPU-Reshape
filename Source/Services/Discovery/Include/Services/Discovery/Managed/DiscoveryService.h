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

#pragma once

// Discovery
#include "DiscoveryProcessInfo.h"

class DiscoveryService;

namespace Discovery::CLR {
	public ref class DiscoveryService {
	public:
		DiscoveryService();
		~DiscoveryService();

        /// Install this service
		bool Install();

        /// Check if all listeners are installed globally
        /// \return if all are installed, returns true
        bool IsGloballyInstalled();

        /// Check if all listeners are running
        /// \return if all are running, returns true
        bool IsRunning();

        /// Starts all listeners
        /// \return success state
        bool Start();

        /// Stops all listeners
        /// \return success state
        bool Stop();

        /// Install all listeners
        ///   ? Enables global hooking of respective discovery, always on for the end user
        /// \return success state
        bool InstallGlobal();

        /// Uninstall all listeners
        ///   ? Disables global hooking of respective discovery
        /// \return success state
	    bool UninstallGlobal();

        /// Check if conflicting instances are installed
        /// \return true if any are installed
	    bool HasConflictingInstances();

        /// Uninstall any conflicting instance
        /// \return false if failed
	    bool UninstallConflictingInstances();

	    /// Start a bootstrapped service against all discovery backends
	    /// \param info process information
	    /// \param environment ordered message stream environment fed to the application
	    /// \return false if failed
	    bool StartBootstrappedProcess(const DiscoveryProcessInfo^ info, Message::CLR::IMessageStream^ environment);

	private:
		::DiscoveryService* service{ nullptr };
	};
}

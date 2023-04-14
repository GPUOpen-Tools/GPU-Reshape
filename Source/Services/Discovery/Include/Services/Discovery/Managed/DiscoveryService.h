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

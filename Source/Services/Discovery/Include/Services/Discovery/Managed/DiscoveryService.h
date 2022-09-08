#pragma once

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

	private:
		::DiscoveryService* service{ nullptr };
	};
}

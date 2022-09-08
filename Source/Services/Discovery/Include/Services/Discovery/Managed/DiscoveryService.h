#pragma once

class DiscoveryService;

namespace Discovery::CLR {
	public ref class DiscoveryService {
	public:
		DiscoveryService();
		~DiscoveryService();

        /// Install this service
		bool Install();

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

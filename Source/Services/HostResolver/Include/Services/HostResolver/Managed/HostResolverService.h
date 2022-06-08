#pragma once

class HostResolverService;

namespace HostResolver::CLR {
	public ref class HostResolverService {
	public:
		HostResolverService();
		~HostResolverService();

        /// Install this service
		bool Install();

	private:
		::HostResolverService* service{ nullptr };
	};
}

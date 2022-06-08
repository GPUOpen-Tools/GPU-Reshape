#include <Services/HostResolver/Managed/HostResolverService.h>
#include <Services/HostResolver/HostResolverService.h>

HostResolver::CLR::HostResolverService::HostResolverService()
{
	service = new ::HostResolverService();
}

HostResolver::CLR::HostResolverService::~HostResolverService()
{
	delete service;
}

bool HostResolver::CLR::HostResolverService::Install() 
{
	return service->Install();
}

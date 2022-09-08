#include <Services/Discovery/Managed/DiscoveryService.h>
#include <Services/Discovery/DiscoveryService.h>

Discovery::CLR::DiscoveryService::DiscoveryService()
{
	service = new ::DiscoveryService();
}

Discovery::CLR::DiscoveryService::~DiscoveryService()
{
	delete service;
}

bool Discovery::CLR::DiscoveryService::Install() 
{
	return service->Install();
}

bool Discovery::CLR::DiscoveryService::Start()
{
	return service->Start();
}

bool Discovery::CLR::DiscoveryService::Stop()
{
	return service->Stop();
}

bool Discovery::CLR::DiscoveryService::InstallGlobal()
{
	return service->InstallGlobal();
}

bool Discovery::CLR::DiscoveryService::UninstallGlobal()
{
	return service->UninstallGlobal();
}

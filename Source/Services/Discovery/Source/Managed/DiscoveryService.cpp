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

bool Discovery::CLR::DiscoveryService::IsGloballyInstalled()
{
    return service->IsGloballyInstalled();
}

bool Discovery::CLR::DiscoveryService::IsRunning()
{
    return service->IsRunning();
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

bool Discovery::CLR::DiscoveryService::HasConflictingInstances()
{
	return service->HasConflictingInstances();
}

bool Discovery::CLR::DiscoveryService::UninstallConflictingInstances()
{
	return service->UninstallConflictingInstances();
}

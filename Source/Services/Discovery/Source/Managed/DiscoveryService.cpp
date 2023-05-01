// CLR (odd-duck include order due to IServiceProvider clashes)
#include <vcclr.h>
#include <msclr/marshal_cppstd.h>

// Discovery
#include <Services/Discovery/Managed/DiscoveryService.h>
#include <Services/Discovery/DiscoveryService.h>

// Message
#include <Message/MessageStream.h>

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

bool Discovery::CLR::DiscoveryService::StartBootstrappedProcess(const DiscoveryProcessInfo ^ info, Message::CLR::IMessageStream ^ environment) {
    // Translate schema
    MessageSchema schema;
    schema.id = environment->GetSchema().id;
    schema.type = static_cast<MessageSchemaType>(environment->GetSchema().type);

    // Get memory span
    Message::CLR::ByteSpan span = environment->GetSpan();

    // Convert to native stream
    MessageStream nativeEnvironment;
    nativeEnvironment.SetSchema(schema);
    nativeEnvironment.SetData(span.Data, span.Length, environment->GetCount());

    // Convert to native info
    ::DiscoveryProcessInfo nativeInfo;
    nativeInfo.applicationPath = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(info->applicationPath).ToPointer());
    nativeInfo.workingDirectoryPath = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(info->workingDirectoryPath).ToPointer());
    nativeInfo.arguments = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(info->arguments).ToPointer());
    nativeInfo.reservedToken = GlobalUID::FromString(static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(info->reservedToken).ToPointer()));

    // Pass down!
    return service->StartBootstrappedProcess(nativeInfo, nativeEnvironment);
}

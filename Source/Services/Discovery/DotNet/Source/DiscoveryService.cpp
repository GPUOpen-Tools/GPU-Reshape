// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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

// CLR (odd-duck include order due to IServiceProvider clashes)
#include <vcclr.h>
#include <msclr/marshal_cppstd.h>

// Discovery
#include <Services/Discovery/Managed/DiscoveryService.h>
#include <Services/Discovery/DiscoveryService.h>

// Message
#include <Message/MessageStream.h>

#pragma warning(push)
#pragma warning(disable : 4691)

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

bool Discovery::CLR::DiscoveryService::StartBootstrappedProcess(const DiscoveryProcessCreateInfo^ createInfo, Message::CLR::IMessageStream^ environment, DiscoveryProcessInfo^% info) {
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

    // Convert optional token
    System::IntPtr hGlobalReservedToken = System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(createInfo->reservedToken);

    // Convert to native info
    ::DiscoveryProcessCreateInfo nativeInfo;
    nativeInfo.applicationPath = static_cast<char *>(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(createInfo->applicationPath).ToPointer());
    nativeInfo.workingDirectoryPath = static_cast<char *>(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(createInfo->workingDirectoryPath).ToPointer());
    nativeInfo.arguments = static_cast<char *>(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(createInfo->arguments).ToPointer());
    nativeInfo.reservedToken = GlobalUID::FromString(static_cast<char *>(hGlobalReservedToken.ToPointer()));
    nativeInfo.captureChildProcesses = createInfo->captureChildProcesses;
    nativeInfo.attachAllDevices = createInfo->attachAllDevices;

    // Convert environment to native
    for each (auto tuple in createInfo->environment) {
        nativeInfo.environment.emplace_back(
            static_cast<char *>(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(tuple->Item1).ToPointer()),
            static_cast<char *>(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(tuple->Item2).ToPointer())
        );
    }
    
    // Pass down!
    ::DiscoveryProcessInfo nativeProcessInfo;
    if (!service->StartBootstrappedProcess(nativeInfo, nativeEnvironment, nativeProcessInfo)) {
        return false;
    }

    // Convert to managed info
    info->processId = nativeProcessInfo.processId;

    // Cleanup paths
    System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr(reinterpret_cast<int64_t>(nativeInfo.applicationPath)));
    System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr(reinterpret_cast<int64_t>(nativeInfo.workingDirectoryPath)));
    System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr(reinterpret_cast<int64_t>(nativeInfo.arguments)));
    System::Runtime::InteropServices::Marshal::FreeHGlobal(hGlobalReservedToken);

    // Cleanup environment
    for (auto && kv : nativeInfo.environment) {
        System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr(reinterpret_cast<int64_t>(kv.first)));
        System::Runtime::InteropServices::Marshal::FreeHGlobal(System::IntPtr(reinterpret_cast<int64_t>(kv.second)));
    }

    // OK
    return true;
}

#pragma warning(pop)

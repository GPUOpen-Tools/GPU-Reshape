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
#include <Discovery/IDiscoveryListener.h>

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

Collections::Generic::List<Discovery::CLR::DiscoveryListenerCLR^>^ Discovery::CLR::DiscoveryService::GetListeners()
{
	uint32_t count;
	service->EnumerateListeners(&count, nullptr);

	std::vector<::IDiscoveryListener*> raw(count);
	service->EnumerateListeners(&count, raw.data());

	auto list = gcnew Collections::Generic::List<DiscoveryListenerCLR^>(count);
	for (uint32_t i = 0; i < count; ++i) {
		list->Add(gcnew DiscoveryListenerCLR(raw[i]));
	}

	return list;
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
    IntPtr hGlobalReservedToken = Runtime::InteropServices::Marshal::StringToHGlobalAnsi(createInfo->reservedToken);

    // Convert to native info
    ::DiscoveryProcessCreateInfo nativeInfo;
    nativeInfo.applicationPath = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(createInfo->applicationPath).ToPointer());
    nativeInfo.workingDirectoryPath = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(createInfo->workingDirectoryPath).ToPointer());
    nativeInfo.arguments = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(createInfo->arguments).ToPointer());
    nativeInfo.reservedToken = GlobalUID::FromString(static_cast<char *>(hGlobalReservedToken.ToPointer()));
    nativeInfo.captureChildProcesses = createInfo->captureChildProcesses;
    nativeInfo.attachAllDevices = createInfo->attachAllDevices;
    nativeInfo.suspendDeferredInitialization = createInfo->suspendDeferredInitialization;
    nativeInfo.waitForDebugger = createInfo->waitForDebugger;
    nativeInfo.redirectPipes = createInfo->redirectPipes;

    // Convert environment to native
    for each (auto tuple in createInfo->environment) {
        nativeInfo.environment.emplace_back(
            static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(tuple->Item1).ToPointer()),
            static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(tuple->Item2).ToPointer())
        );
    }
    
    // Pass down!
    ::DiscoveryProcessInfo nativeProcessInfo;
    if (!service->StartBootstrappedProcess(nativeInfo, nativeEnvironment, nativeProcessInfo)) {
        return false;
    }

    // Convert to managed info
    info->processId = nativeProcessInfo.processId;
    info->readPipe = nativeProcessInfo.readPipe;
    info->writePipe = nativeProcessInfo.writePipe;

    // Cleanup paths
    Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(reinterpret_cast<int64_t>(nativeInfo.applicationPath)));
    Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(reinterpret_cast<int64_t>(nativeInfo.workingDirectoryPath)));
    Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(reinterpret_cast<int64_t>(nativeInfo.arguments)));
    Runtime::InteropServices::Marshal::FreeHGlobal(hGlobalReservedToken);

    // Cleanup environment
    for (auto && kv : nativeInfo.environment) {
        Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(reinterpret_cast<int64_t>(kv.first)));
        Runtime::InteropServices::Marshal::FreeHGlobal(IntPtr(reinterpret_cast<int64_t>(kv.second)));
    }

    // OK
    return true;
}

Discovery::CLR::DiscoveryListenerCLR::DiscoveryListenerCLR(::IDiscoveryListener* listener)
	: _listener(listener)
{
}

String^ Discovery::CLR::DiscoveryListenerCLR::Name::get()
{
	const char* name = _listener->GetInfo().name;
	return name ? gcnew String(name) : nullptr;
}

bool Discovery::CLR::DiscoveryListenerCLR::IsRunning()
{
	return _listener->IsRunning();
}

bool Discovery::CLR::DiscoveryListenerCLR::IsGloballyInstalled()
{
	return _listener->IsGloballyInstalled();
}

bool Discovery::CLR::DiscoveryListenerCLR::Start()
{
	return _listener->Start();
}

bool Discovery::CLR::DiscoveryListenerCLR::Stop()
{
	return _listener->Stop();
}

bool Discovery::CLR::DiscoveryListenerCLR::InstallGlobal()
{
	return _listener->InstallGlobal();
}

bool Discovery::CLR::DiscoveryListenerCLR::UninstallGlobal()
{
	return _listener->UninstallGlobal();
}

#pragma warning(pop)

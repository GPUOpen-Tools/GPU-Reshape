// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Bridge
#include <Bridge/Managed/RemoteClientBridge.h>
#include <Bridge/Managed/BridgeListenerInterop.h>
#include <Bridge/Managed/BridgeMessageStorage.h>
#include <Bridge/RemoteClientBridge.h>

// Common
#include <Common/Registry.h>


struct Bridge::CLR::RemoteClientBridgePrivate {
    /// Self hosted registry
    Registry registry;

    /// Bridge
    ComRef<::RemoteClientBridge> bridge;
};

Bridge::CLR::RemoteClientBridge::RemoteClientBridge() {
    // Create private
    _private = new RemoteClientBridgePrivate();

    // Create bridge
    _private->bridge = _private->registry.AddNew<::RemoteClientBridge>();
}

Bridge::CLR::RemoteClientBridge::~RemoteClientBridge() {
    // Release private, releases all hosted objects
    delete _private;
}

bool Bridge::CLR::RemoteClientBridge::Install(const EndpointResolve^resolve) {
    // Convert to native resolve
    ::EndpointResolve nativeResolve;
    nativeResolve.config.sharedPort = resolve->config->sharedPort;
    nativeResolve.config.applicationName = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(resolve->config->applicationName).ToPointer());
    nativeResolve.ipvxAddress = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(resolve->ipvxAddress).ToPointer());

    // Pass down
    return _private->bridge->Install(nativeResolve);
}

void Bridge::CLR::RemoteClientBridge::InstallAsync(const EndpointResolve ^ resolve) {
    // Convert to native resolve
    ::EndpointResolve nativeResolve;
    nativeResolve.config.sharedPort = resolve->config->sharedPort;
    nativeResolve.config.applicationName = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(resolve->config->applicationName).ToPointer());
    nativeResolve.ipvxAddress = static_cast<char *>(Runtime::InteropServices::Marshal::StringToHGlobalAnsi(resolve->ipvxAddress).ToPointer());

    // Pass down
    _private->bridge->InstallAsync(nativeResolve);
}

static void SetAsyncConnectedDelegateMarshal(Bridge::CLR::RemoteClientBridgePrivate* _private, Bridge::CLR::RemoteClientBridgeAsyncConnectedDelegate ^ delegate) {
    gcroot wrapper(delegate);

    // Set handler on wrapped
    _private->bridge->SetAsyncConnectedDelegate([wrapper] {
        wrapper->Invoke();
    });
}

void Bridge::CLR::RemoteClientBridge::SetAsyncConnectedDelegate(RemoteClientBridgeAsyncConnectedDelegate ^ delegate) {
    SetAsyncConnectedDelegateMarshal(_private, delegate);
}

void Bridge::CLR::RemoteClientBridge::Cancel() {
    _private->bridge->Cancel();
}

void Bridge::CLR::RemoteClientBridge::Stop() {
    _private->bridge->Stop();
}

void Bridge::CLR::RemoteClientBridge::DiscoverAsync() {
    _private->bridge->DiscoverAsync();
}

void Bridge::CLR::RemoteClientBridge::RequestClientAsync(System::Guid^ guid) {
    // Translate to string
    auto ansi = static_cast<const char*>(System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(guid->ToString("B")).ToPointer());
    
    // Create token guid
    _private->bridge->RequestClientAsync(AsioHostClientToken::FromString(ansi));
}

void Bridge::CLR::RemoteClientBridge::Register(MessageID mid, IBridgeListener^listener) {
    MIDKey key(mid, listener);

    // Already registered?
    if (remoteKeyedInteropLookup.ContainsKey(key)) {
        return;
    }

    // Create interop
    ComRef interop = _private->registry.New<BridgeListenerInterop>(listener);

    // Create entry
    InteropEntry^ entry = gcnew InteropEntry();
    entry->component = interop.GetUnsafeAddUser();
    remoteKeyedInteropLookup.Add(key, entry);
    
    _private->bridge->Register(mid, interop);
}

void Bridge::CLR::RemoteClientBridge::Deregister(MessageID mid, IBridgeListener^listener) {
    InteropEntry^ interop;

    // Try to find interop
    if (!remoteKeyedInteropLookup.TryGetValue(MIDKey(mid, listener), interop)) {
        return;
    }

    // Remove
    _private->bridge->Deregister(mid, ComRef(interop->component));
    remoteInteropLookup.Remove(listener);

    // Remove implicit reference
    destroyRef(interop->component);
}

void Bridge::CLR::RemoteClientBridge::Register(IBridgeListener^listener) {
    // Already registered?
    if (remoteInteropLookup.ContainsKey(listener)) {
        return;
    }

    // Create interop
    ComRef interop = _private->registry.New<BridgeListenerInterop>(listener);
    
    // Create entry
    InteropEntry^ entry = gcnew InteropEntry();
    entry->component = interop.GetUnsafeAddUser();
    remoteInteropLookup.Add(listener, entry);
    
    _private->bridge->Register(interop);
}

void Bridge::CLR::RemoteClientBridge::Deregister(IBridgeListener^listener) {
    InteropEntry^ interop;

    // Try to find interop
    if (!remoteInteropLookup.TryGetValue(listener, interop)) {
        return;
    }

    // Remove
    _private->bridge->Deregister(ComRef(interop->component));
    remoteInteropLookup.Remove(listener);

    // Remove implicit reference
    destroyRef(interop->component);
}

Bridge::CLR::IMessageStorage ^ Bridge::CLR::RemoteClientBridge::GetInput() {
    return gcnew BridgeMessageStorage(_private->bridge->GetInput());
}

Bridge::CLR::IMessageStorage ^ Bridge::CLR::RemoteClientBridge::GetOutput() {
    return gcnew BridgeMessageStorage(_private->bridge->GetOutput());
}

Bridge::CLR::BridgeInfo^ Bridge::CLR::RemoteClientBridge::GetInfo() {
    const::BridgeInfo _privateInfo = _private->bridge->GetInfo();

    // To managed
    BridgeInfo^ info = gcnew BridgeInfo;
    info->bytesWritten = _privateInfo.bytesWritten;
    info->bytesRead = _privateInfo.bytesRead;
    return info;
}

void Bridge::CLR::RemoteClientBridge::Commit() {
    _private->bridge->Commit();
}

void Bridge::CLR::RemoteClientBridge::SetCommitOnAppend(bool enabled) {
    _private->bridge->SetCommitOnAppend(enabled);
}

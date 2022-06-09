#include <Bridge/Managed/RemoteClientBridge.h>
#include <Bridge/Managed/BridgeListenerInterop.h>
#include <Bridge/RemoteClientBridge.h>

// Common
#include <Common/Registry.h>

using namespace System;
using namespace System::Runtime::InteropServices;

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
    // Release private, relases all hosted objects
    delete _private;
}

bool Bridge::CLR::RemoteClientBridge::Install(const EndpointResolve^resolve) {
    // Convert to native resolve
    ::EndpointResolve nativeResolve;
    nativeResolve.config.sharedPort = resolve->config->sharedPort;
    nativeResolve.config.applicationName = static_cast<char *>(Marshal::StringToHGlobalAnsi(resolve->config->applicationName).ToPointer());
    nativeResolve.ipvxAddress = static_cast<char *>(Marshal::StringToHGlobalAnsi(resolve->ipvxAddress).ToPointer());

    // Pass down
    return _private->bridge->Install(nativeResolve);
}

void Bridge::CLR::RemoteClientBridge::DiscoverAsync() {
    _private->bridge->DiscoverAsync();
}

void Bridge::CLR::RemoteClientBridge::Register(MessageID mid, IBridgeListener^listener) {
    _private->bridge->Register(mid, _private->registry.New<BridgeListenerInterop>(listener));
}

void Bridge::CLR::RemoteClientBridge::Deregister(MessageID mid, IBridgeListener^listener) {
    throw gcnew System::NotImplementedException();
}

void Bridge::CLR::RemoteClientBridge::Register(IBridgeListener^listener) {
    _private->bridge->Register(_private->registry.New<BridgeListenerInterop>(listener));
}

void Bridge::CLR::RemoteClientBridge::Deregister(IBridgeListener^listener) {
    throw gcnew System::NotImplementedException();
}

Bridge::CLR::IMessageStorage ^ Bridge::CLR::RemoteClientBridge::GetInput() {
    throw gcnew System::NotImplementedException();
}

Bridge::CLR::IMessageStorage ^ Bridge::CLR::RemoteClientBridge::GetOutput() {
    throw gcnew System::NotImplementedException();
}

void Bridge::CLR::RemoteClientBridge::Commit() {
    throw gcnew System::NotImplementedException();
}

void Bridge::CLR::RemoteClientBridge::SetCommitOnAppend(bool enabled) {
    _private->bridge->SetCommitOnAppend(enabled);
}

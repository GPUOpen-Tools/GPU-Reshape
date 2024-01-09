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

#pragma once

// Common
#include <Common/Align.h>
#include <Common/GlobalUID.h>

// Std
#include <cstdint>

// MSVC tight packing
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

/// Generic local port
static constexpr const char* kAsioLocalhost = "127.0.0.1";

/// Client tokens are managed with GUIDs
using AsioHostClientToken = GlobalUID;

/// Header type
enum class AsioHeaderType {
    None,
    HostClientResolverAllocate,
    HostClientResolverDeallocate,
    HostClientResolverUpdate,
    HostClientResolverAllocateResponse,
    HostResolverClientRequest,
    HostResolverClientRequestResolveResponse,
    HostResolverClientRequestResolveRequest,
    HostResolverClientRequestServerResponse,
    RemoteServerResolverDiscoveryRequest,
    RemoteServerResolverDiscoveryResponse,
};

inline const char* ToString(AsioHeaderType type) {
    switch (type) {
        case AsioHeaderType::None:
            return "None";
        case AsioHeaderType::HostClientResolverAllocate:
            return "AsioHostClientResolverAllocate";
        case AsioHeaderType::HostClientResolverDeallocate:
            return "AsioHostClientResolverDeallocate";
        case AsioHeaderType::HostClientResolverUpdate:
            return "AsioHostClientResolverUpdate";
        case AsioHeaderType::HostClientResolverAllocateResponse:
            return "AsioHostClientResolverAllocate::Response";
        case AsioHeaderType::HostResolverClientRequest:
            return "AsioHostResolverClientRequest";
        case AsioHeaderType::HostResolverClientRequestResolveResponse:
            return "AsioHostResolverClientRequest::ResolveResponse";
        case AsioHeaderType::HostResolverClientRequestResolveRequest:
            return "AsioHostResolverClientRequest::ResolveRequest";
        case AsioHeaderType::HostResolverClientRequestServerResponse:
            return "AsioHostResolverClientRequest::ServerResponse";
        case AsioHeaderType::RemoteServerResolverDiscoveryRequest:
            return "AsioRemoteServerResolverDiscoveryRequest";
        case AsioHeaderType::RemoteServerResolverDiscoveryResponse:
            return "AsioRemoteServerResolverDiscoveryRequest::Response";
    }

    return nullptr;
}

/// Protocol header
struct ALIGN_PACK AsioHeader {
    AsioHeader(AsioHeaderType type = AsioHeaderType::None, uint64_t size = 0) : type(type), size(size) {

    }

    AsioHeaderType type;
    uint64_t size;
};

/// Protocol header
template<typename T>
struct ALIGN_PACK TAsioHeader : public AsioHeader {
    TAsioHeader() : AsioHeader(T::kType, sizeof(T)) {

    }
};

struct AsioHostClientInfo {
    /// Name of the process
    char processName[512]{0};

    /// Name of the application, if the API allows
    char applicationName[512]{0};

    /// Name of the backend API
    char apiName[256]{0};

    /// Process identifier
    uint32_t processId{0};

    /// Unique identifier of the device
    uint32_t deviceUid{0};

    /// Number of objects associated with the device
    uint32_t deviceObjects{0};
};

/// Host resolver to host client request
struct ALIGN_PACK AsioHostClientResolverAllocate : public TAsioHeader<AsioHostClientResolverAllocate> {
    static constexpr auto kType = AsioHeaderType::HostClientResolverAllocate;

    AsioHostClientInfo info{};

    /// Optional, token to allocate against
    AsioHostClientToken reservedToken{};

    struct Response : public TAsioHeader<Response> {
        static constexpr auto kType = AsioHeaderType::HostClientResolverAllocateResponse;

        AsioHostClientToken token{};
    };
};

/// Host resolver to host client request
struct ALIGN_PACK AsioHostClientResolverDeallocate : public TAsioHeader<AsioHostClientResolverDeallocate> {
    static constexpr auto kType = AsioHeaderType::HostClientResolverDeallocate;

    /// Token to be deallocated
    AsioHostClientToken token{};
};

/// Host resolver to host client request
struct ALIGN_PACK AsioHostClientResolverUpdate : public TAsioHeader<AsioHostClientResolverUpdate> {
    static constexpr auto kType = AsioHeaderType::HostClientResolverUpdate;

    /// Client token to update for
    AsioHostClientToken token{};

    /// Info to write
    AsioHostClientInfo info{};
};

/// Host resolver to host client request
struct ALIGN_PACK AsioHostResolverClientRequest : public TAsioHeader<AsioHostResolverClientRequest> {
    static constexpr auto kType = AsioHeaderType::HostResolverClientRequest;

    /// Immediate response from the resolver
    struct ResolveResponse : public TAsioHeader<ResolveResponse> {
        static constexpr auto kType = AsioHeaderType::HostResolverClientRequestResolveResponse;

        bool found{};
    };

    /// Resolver to server request
    struct ResolveServerRequest : public TAsioHeader<ResolveServerRequest> {
        static constexpr auto kType = AsioHeaderType::HostResolverClientRequestResolveRequest;

        /// Client requested
        AsioHostClientToken clientToken{};

        /// Owning GUID
        GlobalUID owner;
    };

    struct ServerResponse : public TAsioHeader<ServerResponse> {
        static constexpr auto kType = AsioHeaderType::HostResolverClientRequestServerResponse;

        /// Owning GUID
        GlobalUID owner;

        /// Was the request accepted?
        bool accepted{};

        /// Requested port to open on address, only valid if accepted
        uint16_t remotePort{0};
    };

    /// Client requested
    AsioHostClientToken clientToken{};
};

/// Remote server to host resolver discovery request
struct ALIGN_PACK AsioRemoteServerResolverDiscoveryRequest : public TAsioHeader<AsioRemoteServerResolverDiscoveryRequest> {
    static constexpr auto kType = AsioHeaderType::RemoteServerResolverDiscoveryRequest;

    struct Entry {
        AsioHostClientInfo info;
        AsioHostClientToken token{};
    };

    struct Response : public AsioHeader {
        static constexpr auto kType = AsioHeaderType::RemoteServerResolverDiscoveryResponse;

        Response(uint64_t size) : AsioHeader(kType, size) {

        }

        /// Number of entries
        uint64_t entryCount;

        /// All entries
#pragma warning(push)
#pragma warning(disable:4200)
        Entry entries[0];
#pragma warning(pop)
    };
};

// MSVC tight packing
#ifdef _MSC_VER
#pragma pack(pop)
#endif

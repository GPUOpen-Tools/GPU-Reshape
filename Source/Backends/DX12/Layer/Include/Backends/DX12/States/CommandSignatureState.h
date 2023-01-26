#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include "PipelineType.h"

// Common
#include <Common/Allocators.h>

struct __declspec(uuid("077406A2-E417-48A3-B2F1-A147CAEF4CB3")) CommandSignatureState {
    /// Parent state
    ID3D12Device* parent{nullptr};

    /// Owning allocator
    Allocators allocators;

    /// Object
    ID3D12CommandSignature* object{nullptr};

    /// Active stages
    PipelineTypeSet activeTypes = PipelineType::None;

    /// Unique ID
    uint64_t uid{0};
};

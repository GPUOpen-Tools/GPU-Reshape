#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>
#include "PipelineType.h"

// Common
#include <Common/Allocators.h>

struct CommandSignatureState {
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

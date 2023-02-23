#pragma once

// Layer
#include <Backends/DX12/Detour.Gen.h>

// Common
#include <Common/Allocators.h>

struct __declspec(uuid("2565FDAE-1154-41AC-A400-228EB9C33EF3")) PipelineLibraryState {
    /// Parent state
    ID3D12Device* parent{};

    /// Shared allocators
    Allocators allocators;

    /// User pipeline library
    ID3D12PipelineLibrary* object{nullptr};
};

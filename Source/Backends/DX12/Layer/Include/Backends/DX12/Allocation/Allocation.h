#pragma once

// D3D12MA
#include <D3D12MemAlloc.h>

/// A single allocation
struct Allocation {
    D3D12MA::Allocation* allocation{nullptr};
    ID3D12Resource* resource{nullptr};
};

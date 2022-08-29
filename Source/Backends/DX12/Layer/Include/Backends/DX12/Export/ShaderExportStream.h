#pragma once

// Layer
#include <Backends/DX12/DX12.h>

// Backend
#include <Backend/ShaderExport.h>

struct ShaderExportStream {
    ShaderExportID id = InvalidShaderExportID;

    /// Counter separated for residency management
    ID3D12Resource* counter{nullptr};

    /// Data separated for residency management
    ID3D12Resource* data{nullptr};
};

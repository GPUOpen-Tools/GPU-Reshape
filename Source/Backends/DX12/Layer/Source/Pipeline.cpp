#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>

/// Pretty print pipeline?
#define DXIL_PRETTY_PRINT 1

// Special includes
#if DXIL_PRETTY_PRINT
// Backend
#   include <Backend/IL/PrettyPrint.h>

// Std
#   include <iostream>
#endif

static DXModule *CreateImmediateDXModule(const Allocators &allocators, const D3D12_SHADER_BYTECODE &byteCode) {
    // Get type
    uint32_t type = *static_cast<const uint64_t *>(byteCode.pShaderBytecode);

    // Handle type
    switch (type) {
        default: {
            // Unknown type
            return nullptr;
        }
        case 'CBXD': {
            return new DXBCModule(allocators);
        }
    }
}

static ShaderState *CreateShaderState(DeviceState *device, const D3D12_SHADER_BYTECODE &byteCode) {
    auto shaderState = new(device->allocators) ShaderState();
    shaderState->byteCode = byteCode;
    shaderState->parent = device;

    // Create immediate module
    shaderState->module = CreateImmediateDXModule(device->allocators, byteCode);
    shaderState->module->Parse(byteCode.pShaderBytecode, byteCode.BytecodeLength);

#if DXIL_PRETTY_PRINT
    // Pretty print module
    IL::PrettyPrint(*shaderState->module->GetProgram(), std::cout);
#endif // DXIL_PRETTY_PRINT

    // OK
    return shaderState;
}

HRESULT HookID3D12DeviceCreatePipelineState(ID3D12Device2 *device, const D3D12_PIPELINE_STATE_STREAM_DESC *desc, const IID &riid, void **pPipeline) {
    auto table = GetTable(device);

    // Object
    ID3D12PipelineState *pipeline{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreatePipelineState(table.next, desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new PipelineState();
    state->parent = table.state;

    // Create detours
    pipeline = CreateDetour(Allocators{}, pipeline, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = pipeline->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateGraphicsPipelineState(ID3D12Device *device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc, const IID &riid, void **pPipeline) {
    auto table = GetTable(device);

    // Object
    ID3D12PipelineState *pipeline{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateGraphicsPipelineState(table.next, desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new GraphicsPipelineState();
    state->parent = table.state;
    state->type = PipelineType::Graphics;
    state->object = pipeline;

    // Perform deep copy
    state->deepCopy.DeepCopy(Allocators{}, *desc);

    // Add reference to signature
    desc->pRootSignature->AddRef();
    state->signature = GetState(desc->pRootSignature);

    // Create VS state
    if (desc->VS.BytecodeLength) {
        state->shaders.push_back(CreateShaderState(table.state, state->deepCopy->VS));
    }

    // Create VS state
    if (desc->GS.BytecodeLength) {
        state->shaders.push_back(CreateShaderState(table.state, state->deepCopy->GS));
    }

    // Create VS state
    if (desc->DS.BytecodeLength) {
        state->shaders.push_back(CreateShaderState(table.state, state->deepCopy->DS));
    }

    // Create VS state
    if (desc->HS.BytecodeLength) {
        state->shaders.push_back(CreateShaderState(table.state, state->deepCopy->HS));
    }

    // Create VS state
    if (desc->PS.BytecodeLength) {
        state->shaders.push_back(CreateShaderState(table.state, state->deepCopy->PS));
    }

    // Create detours
    pipeline = CreateDetour(Allocators{}, pipeline, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = pipeline->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateComputePipelineState(ID3D12Device *device, const D3D12_COMPUTE_PIPELINE_STATE_DESC *desc, const IID &riid, void **pPipeline) {
    auto table = GetTable(device);

    // Object
    ID3D12PipelineState *pipeline{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateComputePipelineState(table.next, desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new ComputePipelineState();
    state->parent = table.state;
    state->type = PipelineType::Compute;
    state->object = pipeline;

    // Perform deep copy
    state->deepCopy.DeepCopy(Allocators{}, *desc);

    // Add reference to signature
    desc->pRootSignature->AddRef();
    state->signature = GetState(desc->pRootSignature);

    // Create VS state
    if (desc->CS.BytecodeLength) {
        state->shaders.push_back(CreateShaderState(table.state, state->deepCopy->CS));
    }

    // Create detours
    pipeline = CreateDetour(Allocators{}, pipeline, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = pipeline->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

ULONG WINAPI HookID3D12PipelineStateRelease(ID3D12PipelineState *pipeline) {
    auto table = GetTable(pipeline);

    // Pass down callchain
    LONG users = table.bottom->next_Release(table.next);
    if (users) {
        return users;
    }

    // Cleanup
    delete table.state;

    // OK
    return 0;
}

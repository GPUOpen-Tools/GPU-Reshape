#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>

// Common
#include <Common/CRC.h>

// Special includes
#if DXIL_PRETTY_PRINT
// Backend
#   include <Backend/IL/PrettyPrint.h>

// Std
#   include <iostream>
#endif

static ShaderState *GetOrCreateShaderState(DeviceState *device, const D3D12_SHADER_BYTECODE &byteCode) {
    // Create key
    ShaderStateKey key;
    key.byteCode = byteCode;
    key.hash = BufferCRC64(byteCode.pShaderBytecode, byteCode.BytecodeLength);

    // Sync shader states
    std::lock_guard guard(device->states_Shaders.GetLock());

    // Attempt existing state
    ShaderState* shaderState = device->shaderSet.Get(key);
    if (shaderState) {
        shaderState->AddUser();
        return shaderState;
    }

    // Create new shader
    shaderState = new(device->allocators) ShaderState();
    shaderState->parent = device;
    shaderState->key = key;

    // Copy key memory
    auto shader = new (device->allocators) uint8_t[key.byteCode.BytecodeLength];
    std::memcpy(shader, key.byteCode.pShaderBytecode, key.byteCode.BytecodeLength);
    shaderState->key.byteCode.pShaderBytecode = shader;

    // Add owning user
    shaderState->AddUser();

    // Add to state
    device->states_Shaders.AddNoLock(shaderState);

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
    state->allocators = table.state->allocators;
    state->parent = device;

    // Add owning user
    state->AddUser();

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

    // Get root signature
    auto rootSignatureTable = GetTable(desc->pRootSignature);

    // Object
    ID3D12PipelineState *pipeline{nullptr};

    // Create state
    auto *state = new GraphicsPipelineState();
    state->allocators = table.state->allocators;
    state->parent = device;
    state->type = PipelineType::Graphics;
    state->object = pipeline;

    // Perform deep copy
    state->deepCopy.DeepCopy(Allocators{}, *desc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateGraphicsPipelineState(table.next, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
    if (FAILED(hr)) {
        delete state;
        return hr;
    }

    // Add reference to signature
    desc->pRootSignature->AddRef();
    state->signature = rootSignatureTable.state;

    // Create VS state
    if (desc->VS.BytecodeLength) {
        state->vs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->VS));

        // Add dependency, shader module -> pipeline
        table.state->dependencies_shaderPipelines.Add(state->vs, state);
    }

    // Create GS state
    if (desc->GS.BytecodeLength) {
        state->gs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->GS));

        // Add dependency, shader module -> pipeline
        table.state->dependencies_shaderPipelines.Add(state->gs, state);
    }

    // Create DS state
    if (desc->DS.BytecodeLength) {
        state->ds = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->DS));

        // Add dependency, shader module -> pipeline
        table.state->dependencies_shaderPipelines.Add(state->ds, state);
    }

    // Create HS state
    if (desc->HS.BytecodeLength) {
        state->hs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->HS));

        // Add dependency, shader module -> pipeline
        table.state->dependencies_shaderPipelines.Add(state->hs, state);
    }

    // Create PS state
    if (desc->PS.BytecodeLength) {
        state->ps = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->PS));

        // Add dependency, shader module -> pipeline
        table.state->dependencies_shaderPipelines.Add(state->ps, state);
    }

    // Add to state
    table.state->states_Pipelines.Add(state);

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

    // Get root signature
    auto rootSignatureTable = GetTable(desc->pRootSignature);

    // Object
    ID3D12PipelineState *pipeline{nullptr};

    // Create state
    auto *state = new ComputePipelineState();
    state->allocators = table.state->allocators;
    state->parent = device;
    state->type = PipelineType::Compute;
    state->object = pipeline;

    // Perform deep copy
    state->deepCopy.DeepCopy(Allocators{}, *desc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateComputePipelineState(table.next, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
    if (FAILED(hr)) {
        delete state;
        return hr;
    }

    // Add reference to signature
    desc->pRootSignature->AddRef();
    state->signature = rootSignatureTable.state;

    // Create VS state
    if (desc->CS.BytecodeLength) {
        state->cs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->CS));

        // Add dependency, shader module -> pipeline
        table.state->dependencies_shaderPipelines.Add(state->cs, state);
    }

    // Add to state
    table.state->states_Pipelines.Add(state);

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

HRESULT HookID3D12PipelineStateGetDevice(ID3D12PipelineState *_this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

PipelineState::~PipelineState() {
    auto device = GetTable(parent);

    // Remove state lookup
    device.state->states_Pipelines.Remove(this);

    // Release all instrumented objects
    for (auto&& kv : instrumentObjects) {
        kv.second->Release();
    }

    // Release all references to the shader modules
    for (ShaderState* shader : shaders) {
        // Release dependency
        device.state->dependencies_shaderPipelines.Remove(shader, this);

        // Release ref
        destroyRef(shader, allocators);
    }
}

ShaderState::~ShaderState() {
    // Remove tracked object
    parent->states_Shaders.Remove(this);
}

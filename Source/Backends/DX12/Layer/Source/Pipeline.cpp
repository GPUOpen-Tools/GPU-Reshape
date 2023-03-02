#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>
#include <Backends/DX12/SubObjectReader.h>

// Common
#include <Common/CRC.h>

// Special includes
#if DXIL_PRETTY_PRINT
// Backend
#   include <Backend/IL/PrettyPrint.h>

// Std
#   include <iostream>
#endif

ShaderState *GetOrCreateShaderState(DeviceState *device, const D3D12_SHADER_BYTECODE &byteCode) {
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
    shaderState = new(device->allocators, kAllocState) ShaderState();
    shaderState->parent = device;
    shaderState->key = key;

    // Copy key memory
    auto shader = new (device->allocators, kAllocState) uint8_t[key.byteCode.BytecodeLength];
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

    // Create reader
    SubObjectReader reader(desc);

    // Final pipeline
    PipelineState* opaqueState{nullptr};

    // Handle type
    switch (reader.GetPipelineType()) {
        default:
            ASSERT(false, "Invalid pipeline type");
            return E_FAIL;
        case PipelineType::Graphics: {
            // Create state
            auto state = new (table.state->allocators, kAllocState) GraphicsPipelineState(table.state->allocators);
            state->allocators = table.state->allocators;
            state->parent = device;
            state->type = PipelineType::Graphics;
            state->object = pipeline;

            // Consume all sub-objects
            while (reader.IsGood()) {
                auto type = reader.Consume<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE>();

                // Handle sub-object
                switch (type) {
                    default:
                        reader.Skip(SubObjectReader::GetSize(type));
                        break;
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE: {
                        auto&& rootSignature = reader.AlignedConsume<ID3D12RootSignature*>();

                        // Add reference to signature
                        rootSignature->AddRef();
                        state->signature = GetTable(rootSignature).state;
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
                        auto&& vs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamVSOffset);

                        // Create VS state
                        if (vs.BytecodeLength) {
                            state->vs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, vs));

                            // Add dependency, shader module -> pipeline
                            table.state->dependencies_shaderPipelines.Add(state->vs, state);
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
                        auto&& ps = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamPSOffset);

                        // Create VS state
                        if (ps.BytecodeLength) {
                            state->ps = state->shaders.emplace_back(GetOrCreateShaderState(table.state, ps));

                            // Add dependency, shader module -> pipeline
                            table.state->dependencies_shaderPipelines.Add(state->ps, state);
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
                        auto&& ds = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamDSOffset);

                        // Create VS state
                        if (ds.BytecodeLength) {
                            state->ds = state->shaders.emplace_back(GetOrCreateShaderState(table.state, ds));

                            // Add dependency, shader module -> pipeline
                            table.state->dependencies_shaderPipelines.Add(state->ds, state);
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
                        auto&& hs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamHSOffset);

                        // Create VS state
                        if (hs.BytecodeLength) {
                            state->hs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, hs));

                            // Add dependency, shader module -> pipeline
                            table.state->dependencies_shaderPipelines.Add(state->hs, state);
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
                        auto&& gs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamGSOffset);

                        // Create VS state
                        if (gs.BytecodeLength) {
                            state->gs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, gs));

                            // Add dependency, shader module -> pipeline
                            table.state->dependencies_shaderPipelines.Add(state->gs, state);
                        }
                        break;
                    }
                }

                // To void*
                reader.Align();
            }

            // OK
            opaqueState = state;
            break;
        }
        case PipelineType::Compute: {
            // Create state
            auto *state = new (table.state->allocators, kAllocState) ComputePipelineState(table.state->allocators);
            state->allocators = table.state->allocators;
            state->parent = device;
            state->type = PipelineType::Compute;
            state->object = pipeline;

            // Consume all sub-objects
            while (reader.IsGood()) {
                auto type = reader.Consume<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE>();

                // Handle sub-object
                switch (type) {
                    default:
                        reader.Skip(SubObjectReader::GetSize(type));
                        break;
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE: {
                        auto&& rootSignature = reader.AlignedConsume<ID3D12RootSignature*>();

                        // Add reference to signature
                        rootSignature->AddRef();
                        state->signature = GetTable(rootSignature).state;
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
                        auto&& cs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamCSOffset);

                        // Create VS state
                        if (cs.BytecodeLength) {
                            state->cs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, cs));

                            // Add dependency, shader module -> pipeline
                            table.state->dependencies_shaderPipelines.Add(state->cs, state);
                        }
                        break;
                    }
                }

                // To void*
                reader.Align();
            }

            // OK
            opaqueState = state;
            break;
        }
    }

    // Copy stream blob
    opaqueState->subObjectStreamBlob.resize(desc->SizeInBytes);
    std::memcpy(opaqueState->subObjectStreamBlob.data(), desc->pPipelineStateSubobjectStream, desc->SizeInBytes);

    // Add to state
    table.state->states_Pipelines.Add(opaqueState);

    // Create detours
    pipeline = CreateDetour(opaqueState->allocators, pipeline, opaqueState);

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

    // Create state
    auto *state = new (table.state->allocators, kAllocState) GraphicsPipelineState(table.state->allocators);
    state->allocators = table.state->allocators;
    state->parent = device;
    state->type = PipelineType::Graphics;

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *desc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateGraphicsPipelineState(table.next, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&state->object));
    if (FAILED(hr)) {
        destroy(state, state->allocators);
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
    state->object = CreateDetour(state->allocators, state->object, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = state->object->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    state->object->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateComputePipelineState(ID3D12Device *device, const D3D12_COMPUTE_PIPELINE_STATE_DESC *desc, const IID &riid, void **pPipeline) {
    auto table = GetTable(device);

    // Get root signature
    auto rootSignatureTable = GetTable(desc->pRootSignature);

    // Create state
    auto *state = new (table.state->allocators, kAllocState) ComputePipelineState(table.state->allocators);
    state->allocators = table.state->allocators;
    state->parent = device;
    state->type = PipelineType::Compute;

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *desc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateComputePipelineState(table.next, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&state->object));
    if (FAILED(hr)) {
        destroy(state, state->allocators);
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
    state->object = CreateDetour(state->allocators, state->object, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = state->object->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    state->object->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12PipelineStateGetDevice(ID3D12PipelineState *_this, const IID &riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

HRESULT WINAPI HookID3D12PipelineStateSetName(ID3D12PipelineState* _this, LPCWSTR name) {
    auto table = GetTable(_this);
    auto device = GetTable(table.state->parent);

    // Get length
    size_t length = std::wcslen(name) + 1;

    // Copy string
    table.state->debugName = new (device.state->allocators, kAllocState) char[length];
    wcstombs_s(&length, table.state->debugName, length * sizeof(char), name, length * sizeof(wchar_t));

    // Pass to down the call chain
    return table.next->SetName(name);
}

PipelineState::~PipelineState() {
    auto device = GetTable(parent);

    // Creation may have failed
    if (!object) {
        ASSERT(shaders.empty(), "Unexpected pipeline state on destruction");
        return;
    }

    // Remove state lookup
    device.state->states_Pipelines.Remove(this);

    // Release debug name
    if (debugName) {
        destroy(debugName, device.state->allocators);
    }

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

D3D12_SHADER_BYTECODE ShaderState::GetInstrument(const ShaderInstrumentationKey &instrumentationKey) {
    std::lock_guard lock(mutex);
    auto&& it = instrumentObjects.find(instrumentationKey);
    if (it == instrumentObjects.end()) {
        return {};
    }

    // To bytecode
    D3D12_SHADER_BYTECODE byteCode;
    byteCode.pShaderBytecode = it->second.GetData();
    byteCode.BytecodeLength = it->second.GetByteSize();
    return byteCode;
}

bool ShaderState::Reserve(const ShaderInstrumentationKey &instrumentationKey) {
    std::lock_guard lock(mutex);
    auto&& it = instrumentObjects.find(instrumentationKey);
    if (it == instrumentObjects.end()) {
        instrumentObjects.emplace(instrumentationKey, parent->allocators);
        return true;
    }

    return false;
}

void ShaderState::AddInstrument(const ShaderInstrumentationKey &instrumentationKey, const DXStream &instrument) {
    std::lock_guard lock(mutex);

    // Replace or add
    if (auto it = instrumentObjects.find(instrumentationKey); it != instrumentObjects.end()) {
        it->second = instrument;
    } else {
        instrumentObjects.emplace(instrumentationKey, instrument);
    }
}

// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>
#include <Backends/DX12/SubObjectReader.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>
#include <Backends/DX12/Compiler/DXBC/DXBCUtils.h>

// Common
#include <Common/Allocator/TrackedAllocator.h>
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
    key.hash = GetOrComputeDXBCShaderHash(byteCode.pShaderBytecode, byteCode.BytecodeLength);

    // Sync shader states
    std::lock_guard guard(device->states_Shaders.GetLock());

    // Debugging helper
#ifndef NDEBUG
    if (false) {
        extern TrackedAllocator trackedAllocator;

        // Format
        std::stringstream stream;
        trackedAllocator.Dump(stream);

        // Dump to console
        OutputDebugStringA(stream.str().c_str());
    }
#endif // NDEBUG

    // Attempt existing state
    ShaderState* shaderState = device->shaderSet.Get(key);
    if (shaderState) {
        shaderState->AddUser();
        return shaderState;
    }

    // Create new shader
    shaderState = new(device->allocators, kAllocStateShader) ShaderState();
    shaderState->parent = device;
    shaderState->key = key;

    // Copy key memory
    auto shader = new (device->allocators, kAllocStateShader) uint8_t[byteCode.BytecodeLength];
    std::memcpy(shader, byteCode.pShaderBytecode, byteCode.BytecodeLength);
    shaderState->byteCode.BytecodeLength = byteCode.BytecodeLength;
    shaderState->byteCode.pShaderBytecode = shader;

    // Add owning user
    shaderState->AddUser();

    // Add to state
    device->shaderSet.Add(key, shaderState);
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
            auto state = new (table.state->allocators, kAllocStatePipeline) GraphicsPipelineState(table.state->allocators.Tag(kAllocStatePipeline));
            state->allocators = table.state->allocators;
            state->parent = device;
            state->type = PipelineType::Graphics;
            state->object = pipeline;
    
            // External user
            state->AddUser();

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
            auto *state = new (table.state->allocators, kAllocStatePipeline) ComputePipelineState(table.state->allocators.Tag(kAllocStatePipeline));
            state->allocators = table.state->allocators;
            state->parent = device;
            state->type = PipelineType::Compute;
            state->object = pipeline;
    
            // External user
            state->AddUser();

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
    auto *state = new (table.state->allocators, kAllocStatePipeline) GraphicsPipelineState(table.state->allocators.Tag(kAllocStatePipeline));
    state->allocators = table.state->allocators;
    state->parent = device;
    state->type = PipelineType::Graphics;
    
    // External user
    state->AddUser();

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *desc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateGraphicsPipelineState(table.next, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&state->object));
    if (FAILED(hr)) {
        destroyRef(state, state->allocators);
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

    // Empty shader deep copies
    state->deepCopy->VS = {};
    state->deepCopy->GS = {};
    state->deepCopy->HS = {};
    state->deepCopy->DS = {};
    state->deepCopy->VS = {};

    // Add to state
    table.state->states_Pipelines.Add(state);

    // Create detours
    ID3D12PipelineState* detourPipeline = CreateDetour(state->allocators, state->object, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = detourPipeline->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        table.state->instrumentationController->CreatePipeline(state);
    }

    // Cleanup
    detourPipeline->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateComputePipelineState(ID3D12Device *device, const D3D12_COMPUTE_PIPELINE_STATE_DESC *desc, const IID &riid, void **pPipeline) {
    auto table = GetTable(device);

    // Get root signature
    auto rootSignatureTable = GetTable(desc->pRootSignature);

    // Create state
    auto *state = new (table.state->allocators, kAllocStatePipeline) ComputePipelineState(table.state->allocators.Tag(kAllocStatePipeline));
    state->allocators = table.state->allocators;
    state->parent = device;
    state->type = PipelineType::Compute;
    
    // External user
    state->AddUser();

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *desc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateComputePipelineState(table.next, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&state->object));
    if (FAILED(hr)) {
        destroyRef(state, state->allocators);
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

    // Empty shader deep copies
    state->deepCopy->CS = {};

    // Add to state
    table.state->states_Pipelines.Add(state);

    // Create detours
    ID3D12PipelineState* detourPipeline = CreateDetour(state->allocators, state->object, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = detourPipeline->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        table.state->instrumentationController->CreatePipeline(state);
    }

    // Cleanup
    detourPipeline->Release();

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
    table.state->debugName = new (device.state->allocators, kAllocStatePipeline) char[length];
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
    // Sync shader states
    std::lock_guard guard(parent->states_Shaders.GetLock());
    
    // Remove tracked objects
    parent->states_Shaders.RemoveNoLock(this);
    parent->shaderSet.Remove(key);
}

D3D12_SHADER_BYTECODE ShaderState::GetInstrument(const ShaderInstrumentationKey &instrumentationKey) {
    if (!instrumentationKey.featureBitSet) {
        return byteCode; 
    }

    // Instrumented request
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
    ASSERT(instrumentationKey.featureBitSet, "Invalid instrument reservation");
    
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
    ASSERT(instrumentationKey.featureBitSet, "Invalid instrument addition");

    // Replace or add
    if (auto it = instrumentObjects.find(instrumentationKey); it != instrumentObjects.end()) {
        it->second = instrument;
    } else {
        instrumentObjects.emplace(instrumentationKey, instrument);
    }
}

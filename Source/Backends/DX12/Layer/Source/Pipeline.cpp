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

#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/DeviceState.h>
#include <Backends/DX12/States/ShaderState.h>
#include <Backends/DX12/Compiler/DXBC/DXBCModule.h>
#include <Backends/DX12/PipelineSubObjectReader.h>
#include <Backends/DX12/StateSubObjectWriter.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>
#include <Backends/DX12/Compiler/DXBC/DXBCUtils.h>
#include <Backends/DX12/PipelineSubObjectWriter.h>

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

D3D12_PIPELINE_STATE_STREAM_DESC UnwrapPipelineStateStream(PipelineSubObjectWriter& writer, const D3D12_PIPELINE_STATE_STREAM_DESC* desc, ID3D12RootSignature** outRootSignature) {
    // Create reader
    PipelineSubObjectReader reader(desc);

    // Consume all sub-objects
    while (reader.IsGood()) {
        auto type = reader.Consume<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE>();

        // Set chunk
        writer.Write(type);

        // Handle sub-object
        switch (type) {
            default: {
                // Align chunk if needed
                if (PipelineSubObjectReader::ShouldAlign(type)) {
                    reader.Align();
                    writer.Align();
                }
                
                // No unwrapping needed
                size_t size = PipelineSubObjectReader::GetSize(type);
                writer.AppendChunk(type, reader.Skip(size), size);
                break;
            }
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: {
                // Ignore cached PSO data, modified root signature will cause a mismatch
                break;
            }
            case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE: {
                auto rootSignature = reader.AlignedConsume<ID3D12RootSignature *>();

                // Requested?
                if (rootSignature) {
                    *outRootSignature = rootSignature;
                }

                // Write the unwrapped signature
                writer.WriteAligned(Next(rootSignature));
                break;
            }
        }

        // To void*
        reader.Align();
        writer.Align();
    }

    // Get new description
    return writer.GetDesc();
}

HRESULT HookID3D12DeviceCreatePipelineState(ID3D12Device2 *device, const D3D12_PIPELINE_STATE_STREAM_DESC *desc, const IID &riid, void **pPipeline) {
    auto table = GetTable(device);

    // Object
    ID3D12PipelineState *pipeline{nullptr};

    // Unwrapping writer
    PipelineSubObjectWriter writer(table.state->allocators);

    // Hosted root signature
    ID3D12RootSignature* rootSignature{nullptr};

    // Unwrap the stream
    D3D12_PIPELINE_STATE_STREAM_DESC unwrappedDesc = UnwrapPipelineStateStream(writer, desc, &rootSignature);

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreatePipelineState(table.next, &unwrappedDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
    if (FAILED(hr)) {
        return hr;
    }

    // Create reader (use unwrapped stream for offset safety)
    PipelineSubObjectReader reader(&unwrappedDesc);

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
            state->signature = GetState(rootSignature);
    
            // Add reference to signature
            rootSignature->AddRef();
            
            // External users
            device->AddRef();
            state->AddUser();

            // Consume all sub-objects
            while (reader.IsGood()) {
                auto type = reader.Consume<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE>();

                // Handle sub-object
                switch (type) {
                    default:
                        reader.Skip(PipelineSubObjectReader::GetSize(type));
                        break;
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS: {
                        auto&& vs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamVSOffset);

                        // Create VS state
                        if (vs.BytecodeLength) {
                            state->vs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, vs));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
                        auto&& ps = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamPSOffset);

                        // Create VS state
                        if (ps.BytecodeLength) {
                            state->ps = state->shaders.emplace_back(GetOrCreateShaderState(table.state, ps));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
                        auto&& ds = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamDSOffset);

                        // Create VS state
                        if (ds.BytecodeLength) {
                            state->ds = state->shaders.emplace_back(GetOrCreateShaderState(table.state, ds));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
                        auto&& hs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamHSOffset);

                        // Create VS state
                        if (hs.BytecodeLength) {
                            state->hs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, hs));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
                        auto&& gs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamGSOffset);

                        // Create VS state
                        if (gs.BytecodeLength) {
                            state->gs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, gs));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS: {
                        auto&& as = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamASOffset);

                        // Create AS state
                        if (as.BytecodeLength) {
                            state->as = state->shaders.emplace_back(GetOrCreateShaderState(table.state, as));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS: {
                        auto &&ms = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamMSOffset);

                        // Create MS state
                        if (ms.BytecodeLength) {
                            state->ms = state->shaders.emplace_back(GetOrCreateShaderState(table.state, ms));
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
            state->signature = GetState(rootSignature);
    
            // Add reference to signature
            rootSignature->AddRef();
    
            // External users
            device->AddRef();
            state->AddUser();

            // Consume all sub-objects
            while (reader.IsGood()) {
                auto type = reader.Consume<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE>();

                // Handle sub-object
                switch (type) {
                    default:
                        reader.Skip(PipelineSubObjectReader::GetSize(type));
                        break;
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS: {
                        auto&& cs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamCSOffset);

                        // Create VS state
                        if (cs.BytecodeLength) {
                            state->cs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, cs));
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

    // Set stream blob
    writer.Swap(opaqueState->subObjectWriter);

    // Create detours
    pipeline = CreateDetour(opaqueState->allocators, pipeline, opaqueState);

    // Query to external object if requested
    if (pPipeline) {
        hr = pipeline->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        table.state->instrumentationController->CreatePipelineAndAdd(opaqueState);
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

static uint32_t GetSubObjectIndex(const D3D12_STATE_OBJECT_DESC* desc, const D3D12_STATE_SUBOBJECT* subObject) {
    for (uint32_t i = 0; i < desc->NumSubobjects; i++) {
        if (&desc->pSubobjects[i] == subObject) {
            return i;
        }
    }

    ASSERT(false, "Failed to find sub-object");
    return ~0u; 
}

static HRESULT CreateOrAddToStateObject(ID3D12Device2* device, const D3D12_STATE_OBJECT_DESC* pDesc, ID3D12StateObject* existingStateObject, const IID& riid, void** ppStateObject) {
    auto table = GetTable(device);

    // Create writer
    StateSubObjectWriter writer(table.state->allocators);

    // Reserve on the actual sub-object count
    writer.Reserve(pDesc->NumSubobjects);

    // Unwrap objects
    for (uint32_t i = 0; i < pDesc->NumSubobjects; i++) {
        const D3D12_STATE_SUBOBJECT& subObject = pDesc->pSubobjects[i];
        switch (subObject.Type) {
            default: {
                writer.Add(subObject.Type, subObject.pDesc);
                break;
            }
            case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION: {
                auto contained = StateSubObjectWriter::Read<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION>(subObject);

                // Rewrite sub-object association to future address
                contained.pSubobjectToAssociate = writer.FutureAddressOf(GetSubObjectIndex(pDesc, contained.pSubobjectToAssociate));
                
                writer.Add(subObject.Type, contained);
                break;
            }
            case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION: {
                auto object = StateSubObjectWriter::Read<D3D12_EXISTING_COLLECTION_DESC>(subObject);

                writer.Add(subObject.Type, D3D12_EXISTING_COLLECTION_DESC {
                    .pExistingCollection = Next(object.pExistingCollection),
                    .NumExports = object.NumExports,
                    .pExports = object.pExports
                });
                break;
            }
            case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE: {
                auto object = StateSubObjectWriter::Read<D3D12_GLOBAL_ROOT_SIGNATURE>(subObject);

                writer.Add(subObject.Type, D3D12_GLOBAL_ROOT_SIGNATURE {
                    .pGlobalRootSignature = GetState(object.pGlobalRootSignature)->object
                });
                break;
            }
            case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE: {
                auto object = StateSubObjectWriter::Read<D3D12_LOCAL_ROOT_SIGNATURE>(subObject);
                
                writer.Add(subObject.Type, D3D12_LOCAL_ROOT_SIGNATURE {
                    .pLocalRootSignature = GetState(object.pLocalRootSignature)->nativeObject
                });
                break;
            }
        }
    }

    // Create description
    D3D12_STATE_OBJECT_DESC desc = writer.GetDesc(pDesc->Type);

    // Object
    ID3D12StateObject* stateObject{nullptr};

    // Pass down callchain(s)
    if (existingStateObject) {
        HRESULT hr = table.next->AddToStateObject(&desc, Next(existingStateObject), __uuidof(ID3D12StateObject), reinterpret_cast<void**>(&stateObject));
        if (FAILED(hr)) {
            return hr;
        }
    } else {
        HRESULT hr = table.next->CreateStateObject(&desc, __uuidof(ID3D12StateObject), reinterpret_cast<void**>(&stateObject));
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Create state
    auto* state = new (table.state->allocators, kAllocStateFence) StateObjectState();
    state->allocators = table.state->allocators;
    state->parent = device;

    // Create detours
    stateObject = CreateDetour(state->allocators, stateObject, state);

    // Query to external object if requested
    if (ppStateObject) {
        HRESULT hr = stateObject->QueryInterface(riid, ppStateObject);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    stateObject->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12DeviceCreateStateObject(ID3D12Device2* device, const D3D12_STATE_OBJECT_DESC* pDesc, const IID& riid, void** ppStateObject) {
    return CreateOrAddToStateObject(device, pDesc, nullptr, riid, ppStateObject);
}

HRESULT WINAPI HookID3D12DeviceAddToStateObject(ID3D12Device2* device, const D3D12_STATE_OBJECT_DESC* pAddition, ID3D12StateObject* pStateObjectToGrowFrom, const IID& riid, void** ppNewStateObject) {
    return CreateOrAddToStateObject(device, pAddition, pStateObjectToGrowFrom, riid, ppNewStateObject);
}

HRESULT WINAPI HookID3D12StateObjectGetDevice(ID3D12StateObject* _this, REFIID riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

HRESULT HookID3D12DeviceCreateGraphicsPipelineState(ID3D12Device *device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc, const IID &riid, void **pPipeline) {
    auto table = GetTable(device);

    // Extract root signature from shader bytecode if not supplied
    // This is rather undocumented, but some titles depend on it.
    ID3D12RootSignature* signature = desc->pRootSignature;
    if (!signature) {
        if (HRESULT hr = device->CreateRootSignature(0x0, desc->VS.pShaderBytecode, desc->VS.BytecodeLength, __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&signature)); FAILED(hr)) {
            return hr;
        }
    }

    // Get root signature
    auto rootSignatureTable = GetTable(signature);

    // Create state
    auto *state = new (table.state->allocators, kAllocStatePipeline) GraphicsPipelineState(table.state->allocators.Tag(kAllocStatePipeline));
    state->allocators = table.state->allocators;
    state->parent = device;
    state->type = PipelineType::Graphics;
    state->signature = rootSignatureTable.state;

    // Add reference to signature if external
    if (desc->pRootSignature) {
        signature->AddRef();
    }
    
    // External users
    device->AddRef();
    state->AddUser();

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *desc);

    // Ignore cached PSO data, modified root signature will cause a mismatch
    state->deepCopy->CachedPSO = {};

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateGraphicsPipelineState(table.next, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&state->object));
    if (FAILED(hr)) {
        destroyRef(state, state->allocators);
        return hr;
    }

    // Create VS state
    if (desc->VS.BytecodeLength) {
        state->vs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->VS));
    }

    // Create GS state
    if (desc->GS.BytecodeLength) {
        state->gs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->GS));
    }

    // Create DS state
    if (desc->DS.BytecodeLength) {
        state->ds = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->DS));
    }

    // Create HS state
    if (desc->HS.BytecodeLength) {
        state->hs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->HS));
    }

    // Create PS state
    if (desc->PS.BytecodeLength) {
        state->ps = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->PS));
    }

    // Empty shader deep copies
    state->deepCopy->VS = {};
    state->deepCopy->GS = {};
    state->deepCopy->HS = {};
    state->deepCopy->DS = {};
    state->deepCopy->VS = {};

    // Create detours
    ID3D12PipelineState* detourPipeline = CreateDetour(state->allocators, state->object, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = detourPipeline->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        table.state->instrumentationController->CreatePipelineAndAdd(state);
    }

    // Cleanup
    detourPipeline->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateComputePipelineState(ID3D12Device *device, const D3D12_COMPUTE_PIPELINE_STATE_DESC *desc, const IID &riid, void **pPipeline) {
    auto table = GetTable(device);

    // Extract root signature from shader bytecode if not supplied
    // This is rather undocumented, but some titles depend on it.
    ID3D12RootSignature* signature = desc->pRootSignature;
    if (!signature) {
        if (HRESULT hr = device->CreateRootSignature(0x0, desc->CS.pShaderBytecode, desc->CS.BytecodeLength, __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&signature)); FAILED(hr)) {
            return hr;
        }
    }
    
    // Get root signature
    auto rootSignatureTable = GetTable(signature);

    // Create state
    auto *state = new (table.state->allocators, kAllocStatePipeline) ComputePipelineState(table.state->allocators.Tag(kAllocStatePipeline));
    state->allocators = table.state->allocators;
    state->parent = device;
    state->type = PipelineType::Compute;
    state->signature = rootSignatureTable.state;

    // Add reference to signature if external
    if (desc->pRootSignature) {
        signature->AddRef();
    }
    
    // External users
    device->AddRef();
    state->AddUser();

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *desc);

    // Ignore cached PSO data, modified root signature will cause a mismatch
    state->deepCopy->CachedPSO = {};

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateComputePipelineState(table.next, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&state->object));
    if (FAILED(hr)) {
        destroyRef(state, state->allocators);
        return hr;
    }

    // Create VS state
    if (desc->CS.BytecodeLength) {
        state->cs = state->shaders.emplace_back(GetOrCreateShaderState(table.state, state->deepCopy->CS));
    }

    // Empty shader deep copies
    state->deepCopy->CS = {};

    // Create detours
    ID3D12PipelineState* detourPipeline = CreateDetour(state->allocators, state->object, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = detourPipeline->QueryInterface(riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        table.state->instrumentationController->CreatePipelineAndAdd(state);
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
        parent->Release();

        // Validate
        ASSERT(shaders.empty(), "Unexpected pipeline state on destruction");
        return;
    }

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
        // Note: May not exist if the object was not queried, that's fine
        device.state->dependencies_shaderPipelines.Remove(shader, this);

        // Release ref
        destroyRef(shader, allocators);
    }

    // Release parent
    parent->Release();
}

void PipelineState::ReleaseHost() {
    auto device = GetTable(parent);
    
    // Remove state lookup
    // May not have a slot if it's not queried
    if (uid != kInvalidPipelineUID) {
        // Reference host has locked this
        device.state->states_Pipelines.RemoveNoLock(this);
    }
}

ShaderState::~ShaderState() {
    
}

void ShaderState::ReleaseHost() {
    // Remove tracked objects
    // Reference host has locked these
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

bool ShaderState::RemoveInstrument(const ShaderInstrumentationKey &key) {
    ASSERT(key.featureBitSet, "Invalid instrument reservation");
    
    std::lock_guard lock(mutex);
    if (auto&& it = instrumentObjects.find(key); it != instrumentObjects.end()) {
        instrumentObjects.erase(it);
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

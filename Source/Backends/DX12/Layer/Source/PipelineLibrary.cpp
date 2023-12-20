// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#include <Backends/DX12/PipelineLibrary.h>
#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/PipelineLibraryState.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>
#include <Backends/DX12/PipelineSubObjectReader.h>
#include <Backends/DX12/PipelineSubObjectWriter.h>

HRESULT WINAPI HookID3D12DeviceCreatePipelineLibrary(ID3D12Device* device, const void *pLibraryBlob, SIZE_T BlobLength, REFIID riid, void **ppPipelineLibrary) {
    auto table = GetTable(device);

    // Object
    ID3D12PipelineLibrary* library{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreatePipelineLibrary(table.next, pLibraryBlob, BlobLength, __uuidof(ID3D12PipelineLibrary), reinterpret_cast<void**>(&library));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto *state = new (table.state->allocators, kAllocStatePipelineLibrary) PipelineLibraryState();
    state->parent = device;
    state->allocators = table.state->allocators;
    state->object = library;

    // Create detours
    library = CreateDetour(state->allocators, library, state);
    
    // Query to external object if requested
    if (ppPipelineLibrary) {
        hr = library->QueryInterface(riid, ppPipelineLibrary);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    library->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12PipelineLibraryStorePipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, ID3D12PipelineState *pPipeline) {
    auto table = GetTable(library);

    // Pass down callchain
    HRESULT result = table.next->StorePipeline(pName, Next(pPipeline));
    if (FAILED(result)) {
        return result;
    }

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12PipelineLibraryLoadGraphicsPipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc, REFIID riid, void **ppPipelineState) {
    auto table = GetTable(library);

    // Get device
    auto deviceTable = GetTable(table.state->parent);

    // Get root signature
    auto rootSignatureTable = GetTable(pDesc->pRootSignature);

    // Create state
    auto *state = new (table.state->allocators, kAllocStatePipeline) GraphicsPipelineState(table.state->allocators.Tag(kAllocStatePipeline));
    state->allocators = deviceTable.state->allocators;
    state->parent = table.state->parent;
    state->type = PipelineType::Graphics;

    // Keep reference
    table.state->parent->AddRef();

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *pDesc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;
    
    // External user
    state->AddUser();

    // Final object
    ID3D12PipelineState* pipeline{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_LoadGraphicsPipeline(table.next, pName, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
    if (FAILED(hr)) {
        destroyRef(state, state->allocators);
        return hr;
    }

    // Set object
    state->object = pipeline;

    // Add reference to signature
    pDesc->pRootSignature->AddRef();
    state->signature = rootSignatureTable.state;

    // Create VS state
    if (pDesc->VS.BytecodeLength) {
        state->vs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->VS));
    }

    // Create GS state
    if (pDesc->GS.BytecodeLength) {
        state->gs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->GS));
    }

    // Create DS state
    if (pDesc->DS.BytecodeLength) {
        state->ds = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->DS));
    }

    // Create HS state
    if (pDesc->HS.BytecodeLength) {
        state->hs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->HS));
    }

    // Create PS state
    if (pDesc->PS.BytecodeLength) {
        state->ps = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->PS));
    }

    // Empty shader deep copies
    state->deepCopy->VS = {};
    state->deepCopy->GS = {};
    state->deepCopy->HS = {};
    state->deepCopy->DS = {};
    state->deepCopy->VS = {};

    // Create detours
    pipeline = CreateDetour(state->allocators, pipeline, state);

    // Query to external object if requested
    if (ppPipelineState) {
        hr = pipeline->QueryInterface(riid, ppPipelineState);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        deviceTable.state->instrumentationController->CreatePipelineAndAdd(state);
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12PipelineLibraryLoadComputePipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc, REFIID riid, void **ppPipelineState) {
    auto table = GetTable(library);

    // Get device
    auto deviceTable = GetTable(table.state->parent);

    // Get root signature
    auto rootSignatureTable = GetTable(pDesc->pRootSignature);

    // Create state
    auto *state = new (table.state->allocators, kAllocStatePipeline) ComputePipelineState(table.state->allocators.Tag(kAllocStatePipeline));
    state->allocators = deviceTable.state->allocators;
    state->parent = table.state->parent;
    state->type = PipelineType::Compute;

    // Keep reference
    table.state->parent->AddRef();

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *pDesc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;
    
    // External user
    state->AddUser();

    // Final object
    ID3D12PipelineState* pipeline{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_LoadComputePipeline(table.next, pName, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
    if (FAILED(hr)) {
        destroyRef(state, state->allocators);
        return hr;
    }

    // Set object
    state->object = pipeline;

    // Add reference to signature
    pDesc->pRootSignature->AddRef();
    state->signature = rootSignatureTable.state;

    // Create VS state
    if (pDesc->CS.BytecodeLength) {
        state->cs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->CS));
    }

    // Empty shader deep copies
    state->deepCopy->CS = {};

    // Create detours
    pipeline = CreateDetour(state->allocators, pipeline, state);

    // Query to external object if requested
    if (ppPipelineState) {
        hr = pipeline->QueryInterface(riid, ppPipelineState);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        deviceTable.state->instrumentationController->CreatePipelineAndAdd(state);
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12PipelineLibraryLoadPipeline(ID3D12PipelineLibrary* library, LPCWSTR pName, const D3D12_PIPELINE_STATE_STREAM_DESC* pDesc, const IID& riid, void** ppPipelineState) {
    auto table       = GetTable(library);
    auto deviceTable = GetTable(table.state->parent);

    // Get device
    ID3D12Device* device = table.state->parent;

    // Unwrapping writer
    PipelineSubObjectWriter writer(table.state->allocators);

    // Hosted root signature
    ID3D12RootSignature* rootSignature{nullptr};

    // Unwrap the stream
    D3D12_PIPELINE_STATE_STREAM_DESC unwrappedDesc = UnwrapPipelineStateStream(writer, pDesc, &rootSignature);

    // Object
    ID3D12PipelineState *pipeline{nullptr};

    // Pass down callchain
    HRESULT hr = table.next->LoadPipeline(pName, &unwrappedDesc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&pipeline));
    if (FAILED(hr)) {
        return hr;
    }

    // Final pipeline
    PipelineState* opaqueState{nullptr};
    
    // Create reader (use unwrapped stream for offset safety)
    PipelineSubObjectReader reader(&unwrappedDesc);

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
                            state->vs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, vs));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: {
                        auto&& ps = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamPSOffset);

                        // Create VS state
                        if (ps.BytecodeLength) {
                            state->ps = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, ps));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: {
                        auto&& ds = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamDSOffset);

                        // Create VS state
                        if (ds.BytecodeLength) {
                            state->ds = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, ds));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: {
                        auto&& hs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamHSOffset);

                        // Create VS state
                        if (hs.BytecodeLength) {
                            state->hs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, hs));
                        }
                        break;
                    }
                    case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: {
                        auto&& gs = reader.AlignedConsumeWithOffset<D3D12_SHADER_BYTECODE>(state->streamGSOffset);

                        // Create VS state
                        if (gs.BytecodeLength) {
                            state->gs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, gs));
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
                            state->cs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, cs));
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

    // Set unwrapped stream blob
    writer.Swap(opaqueState->subObjectWriter);

    // Create detours
    pipeline = CreateDetour(opaqueState->allocators, pipeline, opaqueState);

    // Query to external object if requested
    if (ppPipelineState) {
        hr = pipeline->QueryInterface(riid, ppPipelineState);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        deviceTable.state->instrumentationController->CreatePipelineAndAdd(opaqueState);
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12PipelineLibraryGetDevice(ID3D12PipelineLibrary* _this, REFIID riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

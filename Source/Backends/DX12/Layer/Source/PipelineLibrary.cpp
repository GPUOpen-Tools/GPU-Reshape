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

#include <Backends/DX12/PipelineLibrary.h>
#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/PipelineLibraryState.h>
#include <Backends/DX12/Controllers/InstrumentationController.h>

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

HRESULT WINAPI HookID3D12PipelineLibraryGetDevice(ID3D12PipelineLibrary* _this, REFIID riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

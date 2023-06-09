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
    state->object = state->object;

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *pDesc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;
    
    // External user
    state->AddUser();

    // Pass down callchain
    HRESULT hr = table.bottom->next_LoadGraphicsPipeline(table.next, pName, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&state->object));
    if (FAILED(hr)) {
        destroyRef(state, state->allocators);
        return hr;
    }

    // Add reference to signature
    pDesc->pRootSignature->AddRef();
    state->signature = rootSignatureTable.state;

    // Create VS state
    if (pDesc->VS.BytecodeLength) {
        state->vs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->VS));

        // Add dependency, shader module -> pipeline
        deviceTable.state->dependencies_shaderPipelines.Add(state->vs, state);
    }

    // Create GS state
    if (pDesc->GS.BytecodeLength) {
        state->gs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->GS));

        // Add dependency, shader module -> pipeline
        deviceTable.state->dependencies_shaderPipelines.Add(state->gs, state);
    }

    // Create DS state
    if (pDesc->DS.BytecodeLength) {
        state->ds = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->DS));

        // Add dependency, shader module -> pipeline
        deviceTable.state->dependencies_shaderPipelines.Add(state->ds, state);
    }

    // Create HS state
    if (pDesc->HS.BytecodeLength) {
        state->hs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->HS));

        // Add dependency, shader module -> pipeline
        deviceTable.state->dependencies_shaderPipelines.Add(state->hs, state);
    }

    // Create PS state
    if (pDesc->PS.BytecodeLength) {
        state->ps = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->PS));

        // Add dependency, shader module -> pipeline
        deviceTable.state->dependencies_shaderPipelines.Add(state->ps, state);
    }

    // Empty shader deep copies
    state->deepCopy->VS = {};
    state->deepCopy->GS = {};
    state->deepCopy->HS = {};
    state->deepCopy->DS = {};
    state->deepCopy->VS = {};

    // Add to state
    deviceTable.state->states_Pipelines.Add(state);

    // Create detours
    state->object = CreateDetour(state->allocators, state->object, state);

    // Query to external object if requested
    if (ppPipelineState) {
        hr = state->object->QueryInterface(riid, ppPipelineState);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        deviceTable.state->instrumentationController->CreatePipeline(state);
    }

    // Cleanup
    state->object->Release();

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

    // Perform deep copy
    state->deepCopy.DeepCopy(state->allocators, *pDesc);

    // Unwrap description states
    state->deepCopy->pRootSignature = rootSignatureTable.next;
    
    // External user
    state->AddUser();

    // Pass down callchain
    HRESULT hr = table.bottom->next_LoadComputePipeline(table.next, pName, &state->deepCopy.desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void **>(&state->object));
    if (FAILED(hr)) {
        destroyRef(state, state->allocators);
        return hr;
    }

    // Add reference to signature
    pDesc->pRootSignature->AddRef();
    state->signature = rootSignatureTable.state;

    // Create VS state
    if (pDesc->CS.BytecodeLength) {
        state->cs = state->shaders.emplace_back(GetOrCreateShaderState(deviceTable.state, state->deepCopy->CS));

        // Add dependency, shader module -> pipeline
        deviceTable.state->dependencies_shaderPipelines.Add(state->cs, state);
    }

    // Empty shader deep copies
    state->deepCopy->CS = {};

    // Add to state
    deviceTable.state->states_Pipelines.Add(state);

    // Create detours
    state->object = CreateDetour(state->allocators, state->object, state);

    // Query to external object if requested
    if (ppPipelineState) {
        hr = state->object->QueryInterface(riid, ppPipelineState);
        if (FAILED(hr)) {
            return hr;
        }

        // Inform the controller
        deviceTable.state->instrumentationController->CreatePipeline(state);
    }

    // Cleanup
    state->object->Release();

    // OK
    return S_OK;
}

HRESULT WINAPI HookID3D12PipelineLibraryGetDevice(ID3D12PipelineLibrary* _this, REFIID riid, void **ppDevice) {
    auto table = GetTable(_this);

    // Pass to device query
    return table.state->parent->QueryInterface(riid, ppDevice);
}

#include <Backends/DX12/Pipeline.h>
#include <Backends/DX12/Table.Gen.h>
#include <Backends/DX12/States/PipelineState.h>
#include <Backends/DX12/States/DeviceState.h>

HRESULT HookID3D12DeviceCreatePipelineState(ID3D12Device2 *device, const D3D12_PIPELINE_STATE_STREAM_DESC * desc, const IID *const riid, void ** pPipeline) {
    auto table = GetTable(device);

    // Object
    ID3D12PipelineState* pipeline{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreatePipelineState(table.next, desc, &__uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&pipeline));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new PipelineState();
    state->parent = table.state;

    // Create detours
    pipeline = CreateDetour(Allocators{}, pipeline, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = pipeline->QueryInterface(*riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateGraphicsPipelineState(ID3D12Device *device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc, const IID *const riid, void ** pPipeline) {
    auto table = GetTable(device);

    // Object
    ID3D12PipelineState* pipeline{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateGraphicsPipelineState(table.next, desc, &__uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&pipeline));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new GraphicsPipelineState();
    state->parent = table.state;
    state->type = PipelineType::Graphics;

    // Perform deep copy
    state->deepCopy.DeepCopy(Allocators{}, *desc);

    // Create detours
    pipeline = CreateDetour(Allocators{}, pipeline, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = pipeline->QueryInterface(*riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

HRESULT HookID3D12DeviceCreateComputePipelineState(ID3D12Device *device, const D3D12_COMPUTE_PIPELINE_STATE_DESC *desc, const IID *const riid, void ** pPipeline) {
    auto table = GetTable(device);

    // Object
    ID3D12PipelineState* pipeline{nullptr};

    // Pass down callchain
    HRESULT hr = table.bottom->next_CreateComputePipelineState(table.next, desc, &__uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&pipeline));
    if (FAILED(hr)) {
        return hr;
    }

    // Create state
    auto* state = new ComputePipelineState();
    state->parent = table.state;
    state->type = PipelineType::Compute;

    // Perform deep copy
    state->deepCopy.DeepCopy(Allocators{}, *desc);

    // Create detours
    pipeline = CreateDetour(Allocators{}, pipeline, state);

    // Query to external object if requested
    if (pPipeline) {
        hr = pipeline->QueryInterface(*riid, pPipeline);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // Cleanup
    pipeline->Release();

    // OK
    return S_OK;
}

ULONG WINAPI HookID3D12PipelineStateRelease(ID3D12PipelineState* pipeline) {
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

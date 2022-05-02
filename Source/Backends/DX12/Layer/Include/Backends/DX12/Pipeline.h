#pragma once

// Backend
#include <Backends/DX12/DX12.h>

/// Hooks
HRESULT WINAPI HookID3D12DeviceCreatePipelineState(ID3D12Device2*, const D3D12_PIPELINE_STATE_STREAM_DESC*, const IID* const, void**);
HRESULT WINAPI HookID3D12DeviceCreateGraphicsPipelineState(ID3D12Device*, const D3D12_GRAPHICS_PIPELINE_STATE_DESC*, const IID* const, void**);
HRESULT WINAPI HookID3D12DeviceCreateComputePipelineState(ID3D12Device*, const D3D12_COMPUTE_PIPELINE_STATE_DESC*, const IID* const, void**);
ULONG WINAPI HookID3D12PipelineStateRelease(ID3D12PipelineState* pipeline);

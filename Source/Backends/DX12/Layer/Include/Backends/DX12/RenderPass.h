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

#pragma once

// Layer
#include <Backends/DX12/Export/ShaderExportStreamState.h>
#include <Backends/DX12/Table.Gen.h>

/// Get a "preserving", i.e. no side effects, render pass begin operation
static D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE GetRenderPassBeginTypePreserve(D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE type) {
    switch (type) {
        default:
            return type;
        case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD:
        case D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR:
            return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
    }
}

/// Get a "preserving", i.e. no side effects, render pass end operation
static D3D12_RENDER_PASS_ENDING_ACCESS_TYPE GetRenderPassEndTypePreserve(D3D12_RENDER_PASS_ENDING_ACCESS_TYPE type) {
    switch (type) {
        default:
            return type;
        case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE:
        case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD:
            return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
    }
}

/// Unwrap any pending render pass operations
/// @param access modified access
/// @return true if a pending operation
static bool UnwrapPendingRenderPassEnd(D3D12_RENDER_PASS_ENDING_ACCESS& access) {
    switch (access.Type) {
        default: {
            return false;
        }
        case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE: {
            access.Resolve.pSrcResource = Next(access.Resolve.pSrcResource);
            access.Resolve.pDstResource = Next(access.Resolve.pDstResource);
            return true;
        }
        case D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD: {
            return true;
        }
    }
}

/// Start the user render pass, this should always be called before any others
/// This removes all ending side effects
/// @param commandList target list
/// @param state streaming state, must be owned by list
static void BeginRenderPassForUser(ID3D12GraphicsCommandList4* commandList, ShaderExportRenderPassState* state) {
    D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
    std::memcpy(renderTargets, state->renderTargets, sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * state->renderTargetCount);

    // Remove color ending effects
    for (uint32_t i = 0; i < state->renderTargetCount; i++) {
        renderTargets[i].EndingAccess.Type = GetRenderPassEndTypePreserve(renderTargets[i].EndingAccess.Type);
    }

    // Remove ds ending effects
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencil = state->depthStencil;
    depthStencil.DepthEndingAccess.Type = GetRenderPassEndTypePreserve(depthStencil.DepthEndingAccess.Type);
    depthStencil.StencilEndingAccess.Type = GetRenderPassEndTypePreserve(depthStencil.StencilEndingAccess.Type);

    // Start
    commandList->BeginRenderPass(
        state->renderTargetCount,
        renderTargets,
        depthStencil.cpuDescriptor.ptr != 0 ? &depthStencil : nullptr,
        state->flags
    );
}

/// Reconstruct a user render pass, must be called after a user pass has ended
/// This removes all begin and end side effects
/// @param commandList target list
/// @param state streaming state, must be owned by list
static void BeginRenderPassForReconstruction(ID3D12GraphicsCommandList4* commandList, ShaderExportRenderPassState* state) {
    D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
    std::memcpy(renderTargets, state->renderTargets, sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * state->renderTargetCount);

    // Remove color begin and end effects
    for (uint32_t i = 0; i < state->renderTargetCount; i++) {
        renderTargets[i].BeginningAccess.Type = GetRenderPassBeginTypePreserve(renderTargets[i].BeginningAccess.Type);
        renderTargets[i].EndingAccess.Type = GetRenderPassEndTypePreserve(renderTargets[i].EndingAccess.Type);
    }

    // Remove ds begin and end effects
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencil = state->depthStencil;
    depthStencil.DepthBeginningAccess.Type = GetRenderPassBeginTypePreserve(depthStencil.DepthBeginningAccess.Type);
    depthStencil.StencilBeginningAccess.Type = GetRenderPassBeginTypePreserve(depthStencil.StencilBeginningAccess.Type);
    depthStencil.DepthEndingAccess.Type = GetRenderPassEndTypePreserve(depthStencil.DepthEndingAccess.Type);
    depthStencil.StencilEndingAccess.Type = GetRenderPassEndTypePreserve(depthStencil.StencilEndingAccess.Type);

    // Start
    commandList->BeginRenderPass(
        state->renderTargetCount,
        renderTargets,
        depthStencil.cpuDescriptor.ptr != 0 ? &depthStencil : nullptr,
        state->flags
    );
}

/// Resolve all pending user render pass operations, must be called after all user operations have finished
/// This only executes if any operations are pending
/// @param commandList target list
/// @param state streaming state, must be owned by list
static void ResolveRenderPassForUserEnd(ID3D12GraphicsCommandList4* commandList, ShaderExportRenderPassState* state) {
    D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
    std::memcpy(renderTargets, state->renderTargets, sizeof(D3D12_RENDER_PASS_RENDER_TARGET_DESC) * state->renderTargetCount);

    // Does any user operation warrant this?
    bool any = false;

    // Remove color begin effects, and unwrap all ending effects
    for (uint32_t i = 0; i < state->renderTargetCount; i++) {
        renderTargets[i].BeginningAccess.Type = GetRenderPassBeginTypePreserve(renderTargets[i].BeginningAccess.Type);
        any |= UnwrapPendingRenderPassEnd(renderTargets[i].EndingAccess);
    }

    // Remove ds begin effects, and unwrap all ending effects
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencil = state->depthStencil;
    depthStencil.DepthBeginningAccess.Type = GetRenderPassBeginTypePreserve(depthStencil.DepthBeginningAccess.Type);
    depthStencil.StencilBeginningAccess.Type = GetRenderPassBeginTypePreserve(depthStencil.StencilBeginningAccess.Type);
    any |= UnwrapPendingRenderPassEnd(depthStencil.DepthEndingAccess);
    any |= UnwrapPendingRenderPassEnd(depthStencil.StencilEndingAccess);

    // Just early out if nothing's needed
    if (!any) {
        return;
    }

    // Start and stop a patch render pass
    commandList->BeginRenderPass(
        state->renderTargetCount,
        renderTargets,
        depthStencil.cpuDescriptor.ptr != 0 ? &depthStencil : nullptr,
        state->flags
    );

    commandList->EndRenderPass();
}

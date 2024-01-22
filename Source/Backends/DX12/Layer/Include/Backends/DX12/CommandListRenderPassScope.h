﻿// 
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

class CommandListRenderPassScope {
public:
    CommandListRenderPassScope(ID3D12GraphicsCommandList4* commandList, ShaderExportRenderPassState* streamState) : commandList(commandList), streamState(streamState) {
        // Temporarily end render pass
        if (streamState->insideRenderPass) {
            commandList->EndRenderPass();
        }
    }

    ~CommandListRenderPassScope() {
        // Reconstruct render pass if needed
        if (streamState->insideRenderPass) {
            commandList->BeginRenderPass(
                streamState->renderTargetCount,
                streamState->renderTargets,
                streamState->depthStencil.cpuDescriptor.ptr != 0 ? &streamState->depthStencil : nullptr,
                streamState->flags
            );
        }
    }

private:
    ID3D12GraphicsCommandList4* commandList;
    ShaderExportRenderPassState* streamState;
};

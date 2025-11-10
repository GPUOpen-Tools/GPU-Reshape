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

// Backend
#include <Backend/IL/Metadata/KernelMetadata.h>
#include <Backend/IL/ShaderStruct.h>
#include <Backend/IL/Emitters/Emitter.h>

namespace IL {
    /// Write the traceback chunk
    /// @param message message to write chunk to
    /// @param emitter emitter to use
    template<typename T, typename E>
    static void AppendTracebackChunk(typename T::ShaderExport& message, Emitter<E>& emitter) {
        Program *program = emitter.GetProgram();

        // Mark chunk as resident
        message.chunks |= T::Chunk::Traceback;

        // Get the current execution
        ShaderStruct<ExecutionInfo> execution(emitter.ExecutionInfo());

        // Kernel info
        message.traceback.executionFlag = execution.Get<&ExecutionInfo::executionFlags>(emitter);
        message.traceback.pipelineUid = execution.Get<&ExecutionInfo::pipelineUID>(emitter);
        message.traceback.queueUid = execution.Get<&ExecutionInfo::queueUID>(emitter);

        // Marker info
        for (uint32_t i = 0; i < kMaxExecutionInfoMarkerCount; i++) {
            message.traceback.markerHashes32[i] = execution.Get<&ExecutionInfo::markerHashes32>(emitter, i);
        }
        
        // Group counts
        message.traceback.kernelLaunchX = execution.Get<&ExecutionInfo::dispatch>(emitter, 0);
        message.traceback.kernelLaunchY = execution.Get<&ExecutionInfo::dispatch>(emitter, 1);
        message.traceback.kernelLaunchZ = execution.Get<&ExecutionInfo::dispatch>(emitter, 2);

        // Thread indices
        auto* kernelTypeMd = program->GetMetadataMap().GetMetadata<KernelTypeMetadata>(program->GetEntryPoint()->GetID());
        if (kernelTypeMd && kernelTypeMd->type == KernelType::Compute) {
            ID threadId = emitter.KernelValue(Backend::IL::KernelValue::DispatchThreadID);
            message.traceback.threadX = emitter.Extract(threadId, emitter.UInt32(0));
            message.traceback.threadY = emitter.Extract(threadId, emitter.UInt32(1));
            message.traceback.threadZ = emitter.Extract(threadId, emitter.UInt32(2));
        } else {
            // TODO: Need vs, ps, etc. support
            message.traceback.threadX = emitter.UInt32(0);
            message.traceback.threadY = emitter.UInt32(0);
            message.traceback.threadZ = emitter.UInt32(0);
        }
    }
}

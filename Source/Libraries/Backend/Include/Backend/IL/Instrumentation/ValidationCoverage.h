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
#include <Backend/IL/ID.h>
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/ShaderData/ShaderDataValidationCoverage.h>

// Schemas
#include <Schemas/Instrumentation.h>

namespace IL {
    /// Get the coverage buffer id
    /// @return invalid if not enabled
    static ID GetValidationCoverageBufferID(Program& program, const SetInstrumentationConfigMessage& config, ComRef<ShaderDataValidationCoverage>& coverage) {
        if (!config.validationCoverage) {
            return InvalidID;
        }
        
        return program.GetShaderDataMap().Get(coverage->validationCoverage)->id;
    }

    /// Apply the coverage condition, and's false if already reported
    /// @param bufferPtr from GetValidationCoverageBufferID
    /// @param sguid shader guid
    /// @param condition existing condition
    /// @return modified condition
    template<typename T>
    static ID ApplyValidationCoverage(Emitter<T>& emitter, ID bufferPtr, ShaderSGUID sguid, ID condition) {
        // If not enabled, keep the condition as is
        if (bufferPtr == InvalidID) {
            return condition;
        }

        // Load the coverage sample
        // Note that this isn't coherent, but that's fine, we'll overshoot for a few samples before it hits the right caches
        ID previousCoverage = emitter.Extract(
            emitter.LoadBuffer(emitter.Load(bufferPtr), emitter.UInt32(sguid)),
            emitter.UInt32(0)
        );

        // And the condition with cov==0
        return emitter.And(condition, emitter.Equal(previousCoverage, emitter.UInt32(0)));
    }

    /// Store the coverage sample, must happen after ApplyValidationCoverage passes
    /// @param bufferPtr from GetValidationCoverageBufferID
    /// @param sguid shader guid
    template<typename T>
    static void StoreValidationCoverage(Emitter<T>& emitter, ID bufferPtr, ShaderSGUID sguid) {
        // If not enabled, skip
        if (bufferPtr == InvalidID) {
            return;
        }

        // In an error scope, mark as hit
        emitter.StoreBuffer(
            emitter.Load(bufferPtr),
            emitter.UInt32(sguid),
            emitter.UInt32(1)
        );
    }
}

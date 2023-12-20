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

#pragma once

// Layer
#include <Backends/DX12/States/ShaderInstrumentationKey.h>
#include <Backends/DX12/Compiler/Diagnostic/DiagnosticType.h>

// Backend
#include <Backend/Diagnostic/DiagnosticBucketScope.h>

// Common
#include <Common/ComRef.h>

// Forward declarations
class DXILSigner;
class DXBCSigner;

/// Job description
struct DXCompileJob {
    /// The instrumentation key
    ShaderInstrumentationKey instrumentationKey{};

    /// The number of streams
    uint32_t streamCount{0};

    /// Signers
    ComRef<DXILSigner> dxilSigner;
    ComRef<DXBCSigner> dxbcSigner;

    /// Diagnostic
    DiagnosticBucketScope<DiagnosticType, uint64_t> messages;

    /// Compatability states
    struct {
        bool useViewportAndRTArray{false};
    } compatabilityTable;
};

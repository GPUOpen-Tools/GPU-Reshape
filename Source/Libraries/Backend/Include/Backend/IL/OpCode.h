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

namespace IL {
    enum class OpCode {
        /// Invalid op code
        None,

        /// Unexposed by the abstraction layer
        Unexposed,

        /// Literal value
        Literal,

        /// Logical
        Not,
        Any,
        All,

        /// Math
        Add,
        Sub,
        Div,
        Mul,
        Rem,
        Trunc,

        /// Comparison
        Or,
        And,
        Equal,
        NotEqual,
        LessThan,
        LessThanEqual,
        GreaterThan,
        GreaterThanEqual,

        /// Special
        IsInf,
        IsNaN,

        /// Flow control
        Select,
        Branch,
        BranchConditional,
        Switch,
        Phi,
        Return,

        /// Procedure
        Call,

        /// Atomic
        AtomicOr,
        AtomicXOr,
        AtomicAnd,
        AtomicAdd,
        AtomicMin,
        AtomicMax,
        AtomicExchange,
        AtomicCompareExchange,

        /// Full wave intrinsics
        WaveAnyTrue,
        WaveAllTrue,
        WaveBallot,
        WaveRead,
        WaveReadFirst,
        WaveAllEqual,
        WaveBitAnd,
        WaveBitOr,
        WaveBitXOr,
        WaveCountBits,
        WaveMax,
        WaveMin,
        WaveProduct,
        WaveSum,
        WavePrefixCountBits,
        WavePrefixProduct,
        WavePrefixSum,

        /// Bit manipulation
        BitOr,
        BitXOr,
        BitAnd,
        BitShiftLeft,
        BitShiftRight,

        /// Structural
        AddressChain,
        Extract,
        Insert,

        /// Casting
        FloatToInt,
        IntToFloat,
        BitCast,

        /// Messages
        Export,

        /// SSA
        Alloca,
        Load,
        Store,

        /// Program
        StoreOutput,
        StoreVertexOutput,

        /// Resource
        SampleTexture,
        StoreTexture,
        LoadTexture,
        StoreBuffer,
        LoadBuffer,

        /// Resource queries
        ResourceToken,
        ResourceSize
    };
}

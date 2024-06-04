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

namespace Studio.Models.IL
{
    /// <summary>
    /// Mirror of Cxx OpCode, keep up to date
    /// </summary>
    public enum OpCode
    {
        None,
        Unexposed,
        Literal,
        Not,
        Any,
        All,
        Add,
        Sub,
        Div,
        Mul,
        Rem,
        Trunc,
        Or,
        And,
        Equal,
        NotEqual,
        LessThan,
        LessThanEqual,
        GreaterThan,
        GreaterThanEqual,
        IsInf,
        IsNaN,
        KernelValue,
        Select,
        Branch,
        BranchConditional,
        Switch,
        Phi,
        Return,
        Call,
        AtomicOr,
        AtomicXOr,
        AtomicAnd,
        AtomicAdd,
        AtomicMin,
        AtomicMax,
        AtomicExchange,
        AtomicCompareExchange,
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
        BitOr,
        BitXOr,
        BitAnd,
        BitShiftLeft,
        BitShiftRight,
        AddressChain,
        Extract,
        Insert,
        FloatToInt,
        IntToFloat,
        BitCast,
        Export,
        Alloca,
        Load,
        Store,
        StoreOutput,
        StoreVertexOutput,
        StorePrimitiveOutput,
        EmitIndices,
        SampleTexture,
        StoreTexture,
        LoadTexture,
        StoreBuffer,
        LoadBuffer,
        ResourceToken,
        ResourceSize
    }
}
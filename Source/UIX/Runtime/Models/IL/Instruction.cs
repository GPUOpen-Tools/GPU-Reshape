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

using System;

namespace Studio.Models.IL
{
    public class Instruction
    {
        /// <summary>
        /// Op code of this instruction
        /// </summary>
        public OpCode OpCode = OpCode.None;
        
        /// <summary>
        /// Identifier of this instruction
        /// </summary>
        public uint ID = uint.MaxValue;

        /// <summary>
        /// Type of this instruction
        /// </summary>
        public Type? Type;
    }
    
    public class UnexposedInstruction : Instruction
    {
        /// <summary>
        /// Unexposed symbol
        /// </summary>
        public string? Symbol;

        /// <summary>
        /// If no symbol, underlying operation
        /// </summary>
        public uint? UnexposedOp;
    }
    
    public class LiteralInstruction : Instruction
    {
        /// <summary>
        /// Type of the literal
        /// </summary>
        public LiteralType LiteralType;

        /// <summary>
        /// Integral value
        /// </summary>
        public long Integral;
        
        /// <summary>
        /// Floating point value
        /// </summary>
        public double FP;
    }
    
    public class BinaryInstruction : Instruction
    {
        /// <summary>
        /// Left hand side operand
        /// </summary>
        public uint LHS;

        /// <summary>
        /// Right hand side operand
        /// </summary>
        public uint RHS;
    }
    
    public class UnaryInstruction : Instruction
    {
        /// <summary>
        /// Value for unary instruction
        /// </summary>
        public uint Value;
    }
    
    public class WaveReadInstruction : Instruction
    {
        /// <summary>
        /// Value to read
        /// </summary>
        public uint Value;
        
        /// <summary>
        /// Lane to read from
        /// </summary>
        public uint Lane;
    }
    
    public class StoreInstruction : Instruction
    {
        /// <summary>
        /// Target address
        /// </summary>
        public uint Address;

        /// <summary>
        /// Value to be stored
        /// </summary>
        public uint Value;
    }
    
    public class LoadInstruction : Instruction
    {
        /// <summary>
        /// Target address
        /// </summary>
        public uint Address;
    }
    
    public class AtomicInstruction : Instruction
    {
        /// <summary>
        /// Target address
        /// </summary>
        public uint Address;

        /// <summary>
        /// Value to be used
        /// </summary>
        public uint Value;
    }
    
    public class BitShiftInstruction : Instruction
    {
        /// <summary>
        /// Value to be shifted
        /// </summary>
        public uint Value;

        /// <summary>
        /// Shift value
        /// </summary>
        public uint Shift;
    }
    
    public class AtomicCompareInstruction : Instruction
    {
        /// <summary>
        /// Target address
        /// </summary>
        public uint Address;

        /// <summary>
        /// Value to be used
        /// </summary>
        public uint Value;

        /// <summary>
        /// Comparator value
        /// </summary>
        public uint Comparator;
    }
    
    public class SelectInstruction : Instruction
    {
        /// <summary>
        /// Condition to be satisfied
        /// </summary>
        public uint Condition;

        /// <summary>
        /// Passing value
        /// </summary>
        public uint Pass;

        /// <summary>
        /// Failing value
        /// </summary>
        public uint Fail;
    }

    public class ControlFlow
    {
        /// <summary>
        /// Merge control flow branch
        /// </summary>
        public uint MergeBranch = uint.MaxValue;
        
        /// <summary>
        /// Continue control flow branch
        /// </summary>
        public uint ContinueBranch = uint.MaxValue;
    }
    
    public class BranchInstruction : Instruction
    {
        /// <summary>
        /// Target branch
        /// </summary>
        public uint Branch;

        /// <summary>
        /// Optional control flow
        /// </summary>
        public ControlFlow? ControlFlow;
    }
    
    public class BranchConditionalInstruction : Instruction
    {
        /// <summary>
        /// Condition to be satisfied
        /// </summary>
        public uint Condition;

        /// <summary>
        /// Passing branch
        /// </summary>
        public uint Pass;

        /// <summary>
        /// Failing branch
        /// </summary>
        public uint Fail;

        /// <summary>
        /// Optional control flow
        /// </summary>
        public ControlFlow? ControlFlow;
    }
    
    public class SwitchInstruction : Instruction
    {
        /// <summary>
        /// Value to switch on
        /// </summary>
        public uint Value;
        
        /// <summary>
        /// Default branch
        /// </summary>
        public uint Default;

        /// <summary>
        /// Branch - Value associations
        /// </summary>
        public Tuple<uint, uint>[] BranchValues;

        /// <summary>
        /// Optional control flow
        /// </summary>
        public ControlFlow? ControlFlow;
    }
    
    public class PhiInstruction : Instruction
    {
        /// <summary>
        /// Branch - Value associations
        /// </summary>
        public Tuple<uint, uint>[] BranchValues;
    }
    
    public class ReturnInstruction : Instruction
    {
        /// <summary>
        /// Optional return value
        /// </summary>
        public uint? Value;
    }
    
    public class CallInstruction : Instruction
    {
        /// <summary>
        /// Target function
        /// </summary>
        public uint Target;

        /// <summary>
        /// All arguments
        /// </summary>
        public uint[] Arguments;
    }
    
    public class AddressChainInstruction : Instruction
    {
        /// <summary>
        /// Composite to address
        /// </summary>
        public uint Composite;

        /// <summary>
        /// All chains into the composite
        /// </summary>
        public uint[] Chains;
    }
    
    public class FloatToIntInstruction : Instruction
    {
        /// <summary>
        /// Value to convert
        /// </summary>
        public uint Value;
    }
    
    public class IntToFloatInstruction : Instruction
    {
        /// <summary>
        /// Value to convert
        /// </summary>
        public uint Value;
    }
    
    public class BitCastInstruction : Instruction
    {
        /// <summary>
        /// Value to cast
        /// </summary>
        public uint Value;
    }
    
    public class ExportInstruction : Instruction
    {
        /// <summary>
        /// Export stream ID
        /// </summary>
        public uint ExportID;
        
        /// <summary>
        /// Values to export
        /// </summary>
        public uint[] Values;
    }
    
    public class AllocaInstruction : Instruction
    {
    }
    
    public class ExtractInstruction : Instruction
    {
        /// <summary>
        /// Composite to extract
        /// </summary>
        public uint Composite;

        /// <summary>
        /// All chains into the composite
        /// </summary>
        public uint[] Chains;
    }
    
    public class InsertInstruction : Instruction
    {
        /// <summary>
        /// Composite to insert
        /// </summary>
        public uint Composite;
        
        /// <summary>
        /// Value to insert
        /// </summary>
        public uint Value;
    }
    
    public class StoreOutputInstruction : Instruction
    {
        /// <summary>
        /// Output index
        /// </summary>
        public uint Index;
        
        /// <summary>
        /// Row into output
        /// </summary>
        public uint Row;
        
        /// <summary>
        /// Column into output
        /// </summary>
        public uint Column;
        
        /// <summary>
        /// Value to be stored
        /// </summary>
        public uint Value;
    }
    
    public class SampleTextureInstruction : Instruction
    {
        /// <summary>
        /// Sample mode
        /// </summary>
        public TextureSampleMode SampleMode;
        
        /// <summary>
        /// Texture to sample
        /// </summary>
        public uint Texture;
        
        /// <summary>
        /// Optional sampler
        /// </summary>
        public uint? Sampler;
        
        /// <summary>
        /// Coordinates
        /// </summary>
        public uint Coordinate;
        
        /// <summary>
        /// Optional reference for comparison
        /// </summary>
        public uint? Reference;
        
        /// <summary>
        /// Optional explicit LOD
        /// </summary>
        public uint? Lod;
        
        /// <summary>
        /// Optional explicit bias
        /// </summary>
        public uint? Bias;
        
        /// <summary>
        /// Optional explicit ddx
        /// </summary>
        public uint? DDx;
        
        /// <summary>
        /// Optional explicit ddy
        /// </summary>
        public uint? DDy;
        
        /// <summary>
        /// Optional explicit offset
        /// </summary>
        public uint? Offset;
    }
    
    public class LoadTextureInstruction : Instruction
    {
        /// <summary>
        /// Texture to load
        /// </summary>
        public uint Texture;
        
        /// <summary>
        /// Index to load from
        /// </summary>
        public uint Index;
        
        /// <summary>
        /// Optional offset
        /// </summary>
        public uint? Offset;
        
        /// <summary>
        /// Optional mip
        /// </summary>
        public uint? Mip;
    }
    
    public class StoreTextureInstruction : Instruction
    {
        /// <summary>
        /// Texture to store to
        /// </summary>
        public uint Texture;
        
        /// <summary>
        /// Index to store to
        /// </summary>
        public uint Index;
        
        /// <summary>
        /// Texel value to store
        /// </summary>
        public uint Texel;
        
        /// <summary>
        /// Component write mask
        /// </summary>
        public uint ComponentMask;
    }
    
    public class LoadBufferInstruction : Instruction
    {
        /// <summary>
        /// Buffer to load from
        /// </summary>
        public uint Buffer;
        
        /// <summary>
        /// Index to load from
        /// </summary>
        public uint Index;
        
        /// <summary>
        /// Optional offset
        /// </summary>
        public uint? Offset;
    }
    
    public class StoreBufferInstruction : Instruction
    {
        /// <summary>
        /// Buffer to store to
        /// </summary>
        public uint Buffer;
        
        /// <summary>
        /// Index to store to
        /// </summary>
        public uint Index;
        
        /// <summary>
        /// Value to store
        /// </summary>
        public uint Value;
        
        /// <summary>
        /// Component write mask
        /// </summary>
        public uint ComponentMask;
    }
    
    public class LoadBufferRawInstruction : Instruction
    {
        /// <summary>
        /// Buffer to load from
        /// </summary>
        public uint Buffer;
        
        /// <summary>
        /// Index to load from
        /// </summary>
        public uint Index;
        
        /// <summary>
        /// Optional offset
        /// </summary>
        public uint? Offset;
        
        /// <summary>
        /// Component write mask
        /// </summary>
        public uint ComponentMask;
        
        /// <summary>
        /// Byte alignment
        /// </summary>
        public uint Alignment;
    }
    
    public class StoreBufferRawInstruction : Instruction
    {
        /// <summary>
        /// Buffer to store to
        /// </summary>
        public uint Buffer;
        
        /// <summary>
        /// Index to store to
        /// </summary>
        public uint Index;
        
        /// <summary>
        /// Value to store
        /// </summary>
        public uint Value;
        
        /// <summary>
        /// Component write mask
        /// </summary>
        public uint ComponentMask;
        
        /// <summary>
        /// Byte alignment
        /// </summary>
        public uint Alignment;
    }
    
    public class ResourceTokenInstruction : Instruction
    {
        /// <summary>
        /// Target resource
        /// </summary>
        public uint Resource;
    }
    
    public class ResourceSizeInstruction : Instruction
    {
        /// <summary>
        /// Target resource
        /// </summary>
        public uint Resource;
    }
}
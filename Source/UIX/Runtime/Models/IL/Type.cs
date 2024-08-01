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
    public class Type
    {
        /// <summary>
        /// Identifier of the type
        /// </summary>
        public uint ID;

        /// <summary>
        /// Kind of the type
        /// </summary>
        public TypeKind Kind = TypeKind.None;
    }
    
    public class UnexposedType : Type
    {
        
    }
    
    public class BoolType : Type
    {
        
    }
    
    public class VoidType : Type
    {
        
    }
    
    public class IntType : Type
    {
        /// <summary>
        /// Primitive bit width
        /// </summary>
        public int BitWidth;

        /// <summary>
        /// Signed integer?
        /// </summary>
        public bool Signedness;
    }
    
    public class FPType : Type
    {
        /// <summary>
        /// Primitive bit width
        /// </summary>
        public int BitWidth;
    }
    
    public class VectorType : Type
    {
        /// <summary>
        /// Vectorized type
        /// </summary>
        public Type ContainedType;

        /// <summary>
        /// Dimension of this vector
        /// </summary>
        public int Dimension;
    }
    
    public class MatrixType : Type
    {
        /// <summary>
        /// Vectorized type
        /// </summary>
        public Type ContainedType;

        /// <summary>
        /// Number of matrix rows
        /// </summary>
        public int Rows;
        
        /// <summary>
        /// Number of matrix columns
        /// </summary>
        public int Columns;
    }
    
    public class PointerType : Type
    {
        /// <summary>
        /// Target pointee type
        /// </summary>
        public Type? Pointee;

        /// <summary>
        /// Address space of the pointer
        /// </summary>
        public AddressSpace AddressSpace;
    }
    
    public class ArrayType : Type
    {
        /// <summary>
        /// Element type
        /// </summary>
        public Type ElementType;

        /// <summary>
        /// Number of elements
        /// </summary>
        public uint Count;
    }
    
    public class TextureType : Type
    {
        /// <summary>
        /// Actual sampled type
        /// </summary>
        public Type SampledType;

        /// <summary>
        /// Texture dimension
        /// </summary>
        public TextureDimension Dimension;

        /// <summary>
        /// Multisampled?
        /// </summary>
        public bool Multisampled;

        /// <summary>
        /// Attached sampler mode
        /// </summary>
        public ResourceSamplerMode SamplerMode;

        /// <summary>
        /// Given texel format
        /// </summary>
        public Format Format;
    }
    
    public class BufferType : Type
    {
        /// <summary>
        /// Attached sampler mode
        /// </summary>
        public ResourceSamplerMode SamplerMode;

        /// <summary>
        /// Texel format of this buffer
        /// </summary>
        public Format TexelType;

        /// <summary>
        /// Element type of this buffer
        /// </summary>
        public Type ElementType;
    }
    
    public class SamplerType : Type
    {
        
    }
    
    public class CBufferType : Type
    {
        
    }
    
    public class FunctionType : Type
    {
        /// <summary>
        /// Return type
        /// </summary>
        public Type ReturnType;

        /// <summary>
        /// Parameter types
        /// </summary>
        public Type[] ParameterTypes;
    }
    
    public class StructType : Type
    {
        /// <summary>
        /// Member types
        /// </summary>
        public Type[] MemberTypes;
    }
}
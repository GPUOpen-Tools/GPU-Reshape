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

using System;

namespace Studio.Models.Workspace.Objects
{
    public struct ShaderLocation
    {
        /// <summary>
        /// Represents an invalid file uid, shared with backend
        /// </summary>
        public static readonly uint InvalidFileUID = 0xFFFF;
        
        /// <summary>
        /// Originating shader UID
        /// </summary>
        public UInt64 SGUID { get; set; }
        
        /// <summary>
        /// Line offset
        /// </summary>
        public int Line { get; set; }
        
        /// <summary>
        /// File identifier
        /// </summary>
        public int FileUID { get; set; }

        /// <summary>
        /// Column offset
        /// </summary>
        public int Column { get; set; }
        
        /// <summary>
        /// Instruction basic block id
        /// </summary>
        public uint BasicBlockId { get; set; }
        
        /// <summary>
        /// Instruction linear index into the basic block
        /// </summary>
        public uint InstructionIndex { get; set; }
    }
}
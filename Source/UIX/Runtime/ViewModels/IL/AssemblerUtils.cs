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

using Studio.Models.IL;
using Studio.Models.Workspace.Objects;

namespace Runtime.ViewModels.IL
{
    public static class AssemblerUtils
    {
        /// <summary>
        /// Assemble a single line in a program
        /// </summary>
        /// <param name="program">source program</param>
        /// <param name="location">location, must point to program</param>
        /// <returns>null if failed</returns>
        public static string? AssembleSingle(Program program, ShaderLocation location)
        {
            // Try to get basic block
            if (!program.Lookup.TryGetValue(location.BasicBlockId, out object? valueObject) || valueObject is not BasicBlock block)
            {
                return null;
            }

            // Validate instruction
            if (block.Instructions.Length < location.InstructionIndex)
            {
                return null;
            }

            // Create assembler
            var assembler = new Assembler(program);

            // Assemble the instruction
            Instruction instr = block.Instructions[location.InstructionIndex];
            return assembler.AssembleInstruction(instr);
        }
    }
}
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
using ReactiveUI;
using Runtime.ViewModels.IL;
using Studio.Models.IL;
using Studio.Models.Workspace.Listeners;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.ResourceBounds.UIX.Workspace.Objects
{
    public class ScalarizationDetailViewModel : ReactiveObject, IValidationDetailViewModel
    {
        /// <summary>
        /// Source segment
        /// </summary>
        public ShaderSourceSegment? Segment
        {
            get => _segment;
            set
            {
                this.RaiseAndSetIfChanged(ref _segment, value);
                OnObjectChanged();
            }
        }

        /// <summary>
        /// Underlying object
        /// </summary>
        public ShaderViewModel? Object
        {
            get => _object;
            set
            {
                this.RaiseAndSetIfChanged(ref _object, value);
                OnObjectChanged();
            }
        }

        /// <summary>
        /// Final assembled instruction
        /// </summary>
        public string AssembledInstruction
        {
            get => _assembledInstruction;
            set => this.RaiseAndSetIfChanged(ref _assembledInstruction, value);
        }

        /// <summary>
        /// Final assembled instruction
        /// </summary>
        public string AssembledVaryingOperand
        {
            get => _assembledVaryingOperand;
            set => this.RaiseAndSetIfChanged(ref _assembledVaryingOperand, value);
        }

        /// <summary>
        /// Operand index that was deemed varying
        /// </summary>
        public uint VaryingOperandIndex { get; set; }

        /// <summary>
        /// Invoked when the detail object has changed
        /// </summary>
        private void OnObjectChanged()
        {
            Object?.WhenAnyValue(x => x.Program).WhereNotNull().Subscribe(Assemble);
        }

        /// <summary>
        /// Invoked on assembling requests
        /// </summary>
        private void Assemble(Program program)
        {
            // Validation
            if (_segment == null || _object == null)
            {
                return;
            }
            
            // Try to get basic block
            if (!program.Lookup.TryGetValue(_segment.Location.BasicBlockId, out object? valueObject) || valueObject is not BasicBlock block)
            {
                return;
            }

            // Validate instruction
            if (block.Instructions.Length < _segment.Location.InstructionIndex)
            {
                return;
            }

            // Create assembler
            var assembler = new Assembler(program);

            // Assemble the instruction
            Instruction instr = block.Instructions[_segment.Location.InstructionIndex];
            AssembledInstruction = assembler.AssembleInstruction(instr);

            // Assemble the operand
            switch (instr.OpCode)
            {
                case OpCode.Extract:
                {
                    var typed = (ExtractInstruction)instr;
                    AssembledVaryingOperand = assembler.AssembleInlineOperand(typed.Chains[VaryingOperandIndex]);
                    break;
                }
                case OpCode.AddressChain:
                {
                    var typed = (AddressChainInstruction)instr;
                    AssembledVaryingOperand = assembler.AssembleInlineOperand(typed.Chains[VaryingOperandIndex]);
                    break;
                }
            }
        }
        
        /// <summary>
        /// Internal object
        /// </summary>
        private ShaderViewModel? _object;

        /// <summary>
        /// Internal segment
        /// </summary>
        private ShaderSourceSegment? _segment;

        /// <summary>
        /// Internal assembling state
        /// </summary>
        private string _assembledInstruction = "Assembling...";

        /// <summary>
        /// Internal assembling state
        /// </summary>
        private string _assembledVaryingOperand = string.Empty;
    }
}
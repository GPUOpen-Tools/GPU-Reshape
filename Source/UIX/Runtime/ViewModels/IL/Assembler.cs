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
using System.Collections.Generic;
using System.Text;
using Studio.Models.IL;
using Type = Studio.Models.IL.Type;

namespace Runtime.ViewModels.IL
{
    public class Assembler
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="program">source program</param>
        public Assembler(Program program)
        {
            _program = program;
        }
        
        /// <summary>
        /// Assemble the program
        /// </summary>
        /// <returns>assembled textual form</returns>
        public string Assemble()
        {
            // Cleanup
            _lineOffset = 0;
            _assembledMappings.Clear();
            
            // Create builder
            StringBuilder builder = new();

            // Header
            AssembleHeader(builder);
            NewLine(builder);
            
            // Assemble all variables
            foreach (Variable variable in _program.Variables)
            {
                AssembleVariable(variable, builder);
            }
            
            // Assemble all functions
            foreach (Function function in _program.Functions)
            {
                NewLine(builder);
                AssembleFunction(function, builder);
            }

            // To string
            return builder.ToString();
        }

        /// <summary>
        /// Assemble a single instruction
        /// </summary>
        /// <returns>assembled textual form</returns>
        public string AssembleInstruction(Instruction instr)
        {
            // Cleanup
            _lineOffset = 0;
            _assembledMappings.Clear();
            
            // Assemble into builder
            StringBuilder builder = new();
            AssembleInstruction(instr, builder);
            
            // To string
            return builder.ToString();
        }

        /// <summary>
        /// Assemble an inline operand
        /// </summary>
        /// <returns>assembled textual form</returns>
        public string AssembleInlineOperand(uint operand)
        {
            // Cleanup
            _lineOffset = 0;
            _assembledMappings.Clear();
            
            // Assemble into builder
            StringBuilder builder = new();
            AssembleInlineOperand(operand, builder);
            
            // To string
            return builder.ToString();
        }

        /// <summary>
        /// Get an associated mapping
        /// </summary>
        public AssembledMapping GetMapping(uint basicBlockId, uint instructionIndex)
        {
            return _assembledMappings.GetValueOrDefault(GetMappingKey(basicBlockId, instructionIndex));
        }

        /// <summary>
        /// Create a new line
        /// All newlines must use this functionf or correct mappings
        /// </summary>
        private void NewLine(StringBuilder builder)
        {
            builder.Append('\n');
            _lineOffset++;
        }

        /// <summary>
        /// Assemble the header
        /// </summary>
        private void AssembleHeader(StringBuilder builder)
        {
            builder.AppendFormat("// Allocated Identifiers : {0}", _program.AllocatedIdentifiers);
            NewLine(builder);
            builder.AppendFormat("// Entry Point : %{0}", _program.EntryPoint);
            NewLine(builder);
            builder.AppendFormat("// GUID : {0}", _program.GUID);
            NewLine(builder);
        }

        /// <summary>
        /// Assemble a variable
        /// </summary>
        private void AssembleVariable(Variable variable, StringBuilder builder)
        {
            builder.AppendFormat("%{0} = global {1} ", variable.ID, variable.AddressSpace);
            AssembleInlineType(variable.Type, builder);
            NewLine(builder);
        }

        /// <summary>
        /// Assemble a function
        /// </summary>
        private void AssembleFunction(Function function, StringBuilder builder)
        {
            AssembleInlineType(function.Type.ReturnType, builder);
            builder.AppendFormat(" %{0}(", function.ID);

            // Assemble parameters
            for (int i = 0; i < function.Type.ParameterTypes.Length; i++)
            {
                if (i != 0)
                {
                    builder.Append(", ");
                }
                
                AssembleInlineType(function.Type.ParameterTypes[i], builder);
                builder.AppendFormat("%{0}", function.Parameters[i]);
            }
            
            // Open function
            builder.AppendFormat(") {{");
            NewLine(builder);

            // Assemble basic blocks
            for (int i = 0; i < function.BasicBlocks.Length; i++)
            {
                if (i != 0)
                {
                    NewLine(builder);
                }

                AssembleBasicBlock(function.BasicBlocks[i], builder);
            }
            
            // Close function
            builder.AppendFormat("}}");
            NewLine(builder);
        }

        /// <summary>
        /// Assemble a basic block
        /// </summary>
        private void AssembleBasicBlock(BasicBlock block, StringBuilder builder)
        {
            builder.AppendFormat("\t%{0} = BasicBlock", block.ID);
            NewLine(builder);
            
            // Assemble all instructions
            for (int i = 0; i < block.Instructions.Length; i++)
            {
                // Create a mapping for later
                _assembledMappings.Add(GetMappingKey(block.ID, (uint)i), new AssembledMapping()
                {
                    Line = _lineOffset
                });

                // Indentation
                builder.Append('\t');
                
                AssembleInstruction(block.Instructions[i], builder);
                NewLine(builder);
            }
        }

        /// <summary>
        /// Get a mapping key for lookups
        /// </summary>
        private UInt64 GetMappingKey(uint basicBlockId, uint instructionIndex)
        {
            return ((UInt64)basicBlockId) << 32 | instructionIndex;
        }

        /// <summary>
        /// Assemble an instruction
        /// </summary>
        private void AssembleInstruction(Instruction instruction, StringBuilder builder)
        {
            // Has result?
            if (instruction.ID != uint.MaxValue)
            {
                builder.AppendFormat("%{0} = ", instruction.ID);
            }

            // Op code
            builder.Append(instruction.OpCode.ToString().ToLower());
            builder.Append(' ');
            
            // Handle op code
            switch (instruction.OpCode)
            {
                case OpCode.None:
                {
                    break;
                }
                case OpCode.Unexposed:
                {
                    var typed = (UnexposedInstruction)instruction;
                    if (typed.Symbol != null)
                    {
                        builder.AppendFormat("'{0}'", typed.Symbol);
                    }
                    else
                    {
                        builder.Append(typed.UnexposedOp);
                    }
                    break;
                }
                case OpCode.Literal:
                {
                    var typed = (LiteralInstruction)instruction;
                    if (typed.LiteralType == LiteralType.Int)
                    {
                        builder.Append(typed.Integral);
                    }
                    else
                    {
                        builder.Append(typed.FP);
                    }
                    break;
                }
                case OpCode.Any:
                case OpCode.All:
                case OpCode.Trunc:
                case OpCode.IsInf:
                case OpCode.IsNaN:
                {
                    var typed = (UnaryInstruction)instruction;
                    AssembleInlineOperand(typed.Value, builder);
                    break;
                }
                case OpCode.WaveAnyTrue:
                case OpCode.WaveAllTrue:
                case OpCode.WaveBallot:
                case OpCode.WaveReadFirst:
                case OpCode.WaveAllEqual:
                case OpCode.WaveBitAnd:
                case OpCode.WaveBitOr:
                case OpCode.WaveBitXOr:
                case OpCode.WaveCountBits:
                case OpCode.WaveMax:
                case OpCode.WaveMin:
                case OpCode.WaveProduct:
                case OpCode.WaveSum:
                case OpCode.WavePrefixCountBits:
                case OpCode.WavePrefixProduct:
                case OpCode.WavePrefixSum:
                {
                    var typed = (UnaryInstruction)instruction;
                    AssembleInlineOperand(typed.Value, builder);
                    break;
                }
                case OpCode.WaveRead:
                {
                    var typed = (WaveReadInstruction)instruction;
                    AssembleInlineOperand(typed.Value, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Lane, builder);
                    break;
                }
                case OpCode.Add:
                case OpCode.Sub:
                case OpCode.Div:
                case OpCode.Mul:
                case OpCode.Rem:
                case OpCode.Or:
                case OpCode.And:
                case OpCode.Equal:
                case OpCode.NotEqual:
                case OpCode.LessThan:
                case OpCode.LessThanEqual:
                case OpCode.GreaterThan:
                case OpCode.GreaterThanEqual:
                case OpCode.BitOr:
                case OpCode.BitXOr:
                case OpCode.BitAnd:
                {
                    var typed = (BinaryInstruction)instruction;
                    AssembleInlineOperand(typed.LHS, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.RHS, builder);
                    break;
                }
                case OpCode.Select:
                {
                    var typed = (SelectInstruction)instruction;
                    AssembleInlineOperand(typed.Condition, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Pass, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Fail, builder);
                    break;
                }
                case OpCode.Branch:
                {
                    var typed = (BranchInstruction)instruction;
                    AssembleInlineOperand(typed.Branch, builder);
                    
                    if (typed.ControlFlow != null)
                    {
                        builder.AppendFormat(", merge %{0}", typed.ControlFlow.MergeBranch);

                        if (typed.ControlFlow.ContinueBranch != uint.MaxValue)
                        {
                            builder.AppendFormat(", continue %{0}", typed.ControlFlow.ContinueBranch);
                        }
                    }
                    break;
                }
                case OpCode.BranchConditional:
                {
                    var typed = (BranchConditionalInstruction)instruction;
                    AssembleInlineOperand(typed.Condition, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Pass, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Fail, builder);

                    if (typed.ControlFlow != null)
                    {
                        builder.AppendFormat(", merge %{0}", typed.ControlFlow.MergeBranch);

                        if (typed.ControlFlow.ContinueBranch != uint.MaxValue)
                        {
                            builder.AppendFormat(", continue %{0}", typed.ControlFlow.ContinueBranch);
                        }
                    }
                    break;
                }
                case OpCode.Switch:
                {
                    var typed = (SwitchInstruction)instruction;
                    AssembleInlineOperand(typed.Value, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Default, builder);

                    if (typed.ControlFlow != null)
                    {
                        builder.AppendFormat(", merge %{0}", typed.ControlFlow.MergeBranch);

                        if (typed.ControlFlow.ContinueBranch != uint.MaxValue)
                        {
                            builder.AppendFormat(", continue %{0}", typed.ControlFlow.ContinueBranch);
                        }
                    }

                    builder.Append(' ');
                    
                    for (int i = 0; i < typed.BranchValues.Length; i++)
                    {
                        if (i != 0)
                        {
                            builder.Append(", ");
                        }
                        
                        builder.AppendFormat("[%{0}, {1}]", typed.BranchValues[i].Item1, typed.BranchValues[i].Item2);
                    }
                    break;
                }
                case OpCode.Phi:
                {
                    var typed = (PhiInstruction)instruction;

                    for (int i = 0; i < typed.BranchValues.Length; i++)
                    {
                        if (i != 0)
                        {
                            builder.Append(", ");
                        }
                        
                        builder.AppendFormat("[ %{0}, %{1} ]", typed.BranchValues[i].Item1, typed.BranchValues[i].Item2);
                    }
                    break;
                }
                case OpCode.Return:
                {
                    var typed = (ReturnInstruction)instruction;

                    if (typed.Value.HasValue)
                    {
                        AssembleInlineOperand(typed.Value.Value, builder);
                    }
                    break;
                }
                case OpCode.Call:
                {
                    var typed = (CallInstruction)instruction;
                    
                    AssembleInlineOperand(typed.Target, builder);
                    builder.Append("(");
                    
                    for (int i = 0; i < typed.Arguments.Length; i++)
                    {
                        if (i != 0)
                        {
                            builder.Append(", ");
                        }
                        
                        builder.AppendFormat("%{0}", typed.Arguments[i]);
                    }
                    
                    builder.Append(" )");
                    break;
                }
                case OpCode.AtomicOr:
                case OpCode.AtomicXOr:
                case OpCode.AtomicAnd:
                case OpCode.AtomicAdd:
                case OpCode.AtomicMin:
                case OpCode.AtomicMax:
                case OpCode.AtomicExchange:
                {
                    var typed = (AtomicInstruction)instruction;
                    AssembleInlineOperand(typed.Address, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Value, builder);
                    break;
                }
                case OpCode.AtomicCompareExchange:
                {
                    var typed = (AtomicCompareInstruction)instruction;
                    AssembleInlineOperand(typed.Address, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Value, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Comparator, builder);
                    break;
                }
                case OpCode.BitShiftLeft:
                case OpCode.BitShiftRight:
                {
                    var typed = (BitShiftInstruction)instruction;
                    AssembleInlineOperand(typed.Value, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Shift, builder);
                    break;
                }
                case OpCode.AddressChain:
                {
                    var typed = (AddressChainInstruction)instruction;
                    AssembleInlineOperand(typed.Composite, builder);

                    builder.Append(" [ ");
                    
                    for (int i = 0; i < typed.Chains.Length; i++)
                    {
                        if (i != 0)
                        {
                            builder.Append(", ");
                        }
                        
                        AssembleInlineOperand(typed.Chains[i], builder);
                    }
                    
                    builder.Append(" ]");
                    break;
                }
                case OpCode.Extract:
                {
                    var typed = (ExtractInstruction)instruction;
                    AssembleInlineOperand(typed.Composite, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Index, builder);
                    break;
                }
                case OpCode.Insert:
                {
                    var typed = (InsertInstruction)instruction;
                    AssembleInlineOperand(typed.Composite, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Value, builder);
                    break;
                }
                case OpCode.FloatToInt:
                {
                    var typed = (FloatToIntInstruction)instruction;
                    AssembleInlineOperand(typed.Value, builder);
                    break;
                }
                case OpCode.IntToFloat:
                {
                    var typed = (IntToFloatInstruction)instruction;
                    AssembleInlineOperand(typed.Value, builder);
                    break;
                }
                case OpCode.BitCast:
                {
                    var typed = (BitCastInstruction)instruction;
                    AssembleInlineOperand(typed.Value, builder);
                    break;
                }
                case OpCode.Export:
                {
                    var typed = (ExportInstruction)instruction;
                    builder.AppendFormat("{0} ", typed.ExportID);
                    
                    builder.Append(" [ ");
                    
                    for (int i = 0; i < typed.Values.Length; i++)
                    {
                        if (i != 0)
                        {
                            builder.Append(", ");
                        }
                        
                        AssembleInlineOperand(typed.Values[i], builder);
                    }
                    
                    builder.Append(" ]");
                    break;
                }
                case OpCode.Alloca:
                {
                    var typed = (AllocaInstruction)instruction;
                    AssembleInlineType(typed.Type, builder);
                    break;
                }
                case OpCode.Load:
                {
                    var typed = (LoadInstruction)instruction;
                    AssembleInlineOperand(typed.Address, builder);
                    break;
                }
                case OpCode.Store:
                {
                    var typed = (StoreInstruction)instruction;
                    AssembleInlineOperand(typed.Address, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Value, builder);
                    break;
                }
                case OpCode.StoreOutput:
                {
                    var typed = (StoreOutputInstruction)instruction;
                    AssembleInlineOperand(typed.Index, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Row, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Column, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Value, builder);
                    break;
                }
                case OpCode.SampleTexture:
                {
                    var typed = (SampleTextureInstruction)instruction;
                    AssembleInlineOperand(typed.Texture, builder);
                    builder.AppendFormat(" {0} ", typed.SampleMode);

                    if (typed.Sampler.HasValue)
                    {
                        AssembleInlineOperand(typed.Sampler.Value, builder);
                        builder.Append(' ');
                    }

                    AssembleInlineOperand(typed.Coordinate, builder);

                    if (typed.Reference.HasValue)
                    {
                        builder.Append("ref ");
                        AssembleInlineOperand(typed.Reference.Value, builder);
                        builder.Append(' ');
                    }

                    if (typed.Lod.HasValue)
                    {
                        builder.Append("lod ");
                        AssembleInlineOperand(typed.Lod.Value, builder);
                        builder.Append(' ');
                    }

                    if (typed.Bias.HasValue)
                    {
                        builder.Append("bias ");
                        AssembleInlineOperand(typed.Bias.Value, builder);
                        builder.Append(' ');
                    }

                    if (typed.Offset.HasValue)
                    {
                        builder.Append("offset ");
                        AssembleInlineOperand(typed.Offset.Value, builder);
                        builder.Append(' ');
                    }

                    if (typed.DDx.HasValue)
                    {
                        builder.Append("ddx ");
                        AssembleInlineOperand(typed.DDx.Value, builder);
                        builder.Append(' ');
                    }

                    if (typed.DDy.HasValue)
                    {
                        builder.Append("ddy ");
                        AssembleInlineOperand(typed.DDy.Value, builder);
                        builder.Append(' ');
                    }
                    break;
                }
                case OpCode.StoreTexture:
                {
                    var typed = (StoreTextureInstruction)instruction;
                    AssembleInlineOperand(typed.Texture, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Index, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Texel, builder);
                    builder.AppendFormat(" mask {0}", typed.ComponentMask);
                    break;
                }
                case OpCode.LoadTexture:
                {
                    var typed = (LoadTextureInstruction)instruction;
                    AssembleInlineOperand(typed.Texture, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Index, builder);
                    builder.Append(' ');
                    
                    if (typed.Offset.HasValue)
                    {
                        builder.Append("offset ");
                        AssembleInlineOperand(typed.Offset.Value, builder);
                        builder.Append(' ');
                    }
                    
                    if (typed.Mip.HasValue)
                    {
                        builder.Append("mip ");
                        AssembleInlineOperand(typed.Mip.Value, builder);
                        builder.Append(' ');
                    }
                    break;
                }
                case OpCode.StoreBuffer:
                {
                    var typed = (StoreBufferInstruction)instruction;
                    AssembleInlineOperand(typed.Buffer, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Index, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Value, builder);
                    builder.AppendFormat(" mask {0}", typed.ComponentMask);
                    break;
                }
                case OpCode.LoadBuffer:
                {
                    var typed = (LoadBufferInstruction)instruction;
                    AssembleInlineOperand(typed.Buffer, builder);
                    builder.Append(' ');
                    AssembleInlineOperand(typed.Index, builder);
                    builder.Append(' ');
                    
                    if (typed.Offset.HasValue)
                    {
                        builder.Append("offset ");
                        AssembleInlineOperand(typed.Offset.Value, builder);
                        builder.Append(' ');
                    }
                    break;
                }
                case OpCode.ResourceToken:
                {
                    var typed = (ResourceTokenInstruction)instruction;
                    AssembleInlineOperand(typed.Resource, builder);
                    break;
                }
                case OpCode.ResourceSize:
                {
                    var typed = (ResourceSizeInstruction)instruction;
                    AssembleInlineOperand(typed.Resource, builder);
                    break;
                }
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        /// <summary>
        /// Assemble an inline operand
        ///
        /// Inline operands are shorthand representations for inline (on the same line) printing
        /// </summary>
        private void AssembleInlineOperand(uint operand, StringBuilder builder)
        {
            // Foreign? Just dump it
            if (!_program.Lookup.ContainsKey(operand))
            {
                builder.AppendFormat("%{0}", operand);
                return;
            }

            // Get the lookup
            object value = _program.Lookup[operand];

            // Type?
            if (value is Type type)
            {
                AssembleInlineType(type, builder, true);
                return;
            }

            // Constant?
            if (value is Constant constant)
            {
                AssembleInlineConstant(constant, builder);
                return;
            }

            // Instruction?
            if (value is Instruction instruction)
            {
                AssembleInlineInstruction(instruction, builder);
                return;
            }

            // Basic block?
            if (value is BasicBlock block)
            {
                AssembleInlineBasicBlock(block, builder);
                return;
            }

            // Variable?
            if (value is Variable variable)
            {
                AssembleInlineVariable(variable, builder);
                return;
            }

            // Function?
            if (value is Function function)
            {
                AssembleInlineFunction(function, builder);
                return;
            }
        }

        /// <summary>
        /// Assemble an inline basic block
        /// </summary>
        private void AssembleInlineBasicBlock(BasicBlock block, StringBuilder builder)
        {
            builder.AppendFormat("label %{0}", block.ID);
        }

        /// <summary>
        /// Assemble an inline variable
        /// </summary>
        private void AssembleInlineVariable(Variable variable, StringBuilder builder)
        {
            builder.AppendFormat("{0}* %{1}", variable.AddressSpace, variable.ID);
        }

        /// <summary>
        /// Assemble an inline function
        /// </summary>
        private void AssembleInlineFunction(Function function, StringBuilder builder)
        {
            builder.AppendFormat("func %{0}", function.ID);
        }

        /// <summary>
        /// Assemble an inline instruction
        /// </summary>
        private void AssembleInlineInstruction(Instruction instruction, StringBuilder builder)
        {
            // Prefix with short type
            if (instruction.Type != null)
            {
                AssembleInlineType(instruction.Type, builder, true);
                builder.Append(' ');
            }

            // ID
            builder.AppendFormat("%{0}", instruction.ID);
        }

        /// <summary>
        /// Assemble an inline constant, ideally we want to represent constants with the value on the same line
        /// </summary>
        private void AssembleInlineConstant(Constant constant, StringBuilder builder, bool emitType = true)
        {
            // Typed?
            if (emitType)
            {
                AssembleInlineType(constant.Type, builder, true);
                builder.Append(' ');
            }

            // Handle kind
            switch (constant.Kind)
            {
                case ConstantKind.None:
                {
                    builder.Append("<none>");
                    break;
                }
                case ConstantKind.Undef:
                {
                    builder.Append("undef");
                    break;
                }
                case ConstantKind.Bool:
                {
                    var typed = (BoolConstant)constant;
                    builder.Append(typed.Value);
                    break;
                }
                case ConstantKind.Int:
                {
                    var typed = (IntConstant)constant;
                    builder.Append(typed.Value);
                    break;
                }
                case ConstantKind.FP:
                {
                    var typed = (FPConstant)constant;
                    builder.Append(typed.Value);
                    break;
                }
                case ConstantKind.Array:
                {
                    var typed = (ArrayConstant)constant;
                    builder.Append("[ ");

                    for (int i = 0; i < typed.Elements.Length; i++)
                    {
                        if (i != 0)
                        {
                            builder.Append(", ");
                        }
                        
                        AssembleInlineConstant(typed.Elements[i], builder, false);
                    }
                    
                    builder.Append(" ]");
                    break;
                }
                case ConstantKind.Vector:
                {
                    var typed = (VectorConstant)constant;
                    builder.Append("< ");

                    for (int i = 0; i < typed.Elements.Length; i++)
                    {
                        if (i != 0)
                        {
                            builder.Append(", ");
                        }
                        
                        AssembleInlineConstant(typed.Elements[i], builder, false);
                    }
                    
                    builder.Append(" >");
                    break;
                }
                case ConstantKind.Struct:
                {
                    var typed = (StructConstant)constant;
                    builder.Append("{ ");

                    for (int i = 0; i < typed.Members.Length; i++)
                    {
                        if (i != 0)
                        {
                            builder.Append(", ");
                        }
                        
                        AssembleInlineConstant(typed.Members[i], builder, false);
                    }
                    
                    builder.Append(" }");
                    break;
                }
                case ConstantKind.Null:
                {
                    builder.Append("null");
                    break;
                }
                case ConstantKind.Unexposed:
                {
                    builder.AppendFormat("unexposed %{0}", constant.ID);
                    break;
                }
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        /// <summary>
        /// Assemble an inline type
        /// </summary>
        private void AssembleInlineType(Type type, StringBuilder builder, bool shortType = false)
        {
            switch (type.Kind)
            {
                case TypeKind.None:
                {
                    builder.Append("<None>");
                    break;
                }
                case TypeKind.Bool:
                {
                    builder.Append("bool");
                    break;
                }
                case TypeKind.Void:
                {
                    builder.Append("void");
                    break;
                }
                case TypeKind.Int:
                {
                    var intType = (IntType)type;

                    if (intType.Signedness)
                    {
                        builder.AppendFormat("int{0}", intType.BitWidth);
                    }
                    else
                    {
                        builder.AppendFormat("uint{0}", intType.BitWidth);
                    }
                    break;
                }
                case TypeKind.FP:
                {
                    var fpType = (FPType)type;

                    switch (fpType.BitWidth)
                    {
                        default:
                            builder.AppendFormat("fp{0}", fpType.BitWidth);
                            break;
                        case 16:
                            builder.Append("half");
                            break;
                        case 32:
                            builder.Append("float");
                            break;
                        case 64:
                            builder.Append("double");
                            break;
                    }
                    break;
                }
                case TypeKind.Vector:
                {
                    var vectorType = (VectorType)type;
                    builder.Append('<');
                    AssembleInlineType(vectorType.ContainedType, builder, shortType);
                    builder.AppendFormat(", {0}>", vectorType.Dimension);
                    break;
                }
                case TypeKind.Matrix:
                {
                    var matrixType = (MatrixType)type;
                    builder.Append('<');
                    AssembleInlineType(matrixType.ContainedType, builder, shortType);
                    builder.AppendFormat(", {0}, {1}>", matrixType.Rows, matrixType.Columns);
                    break;
                }
                case TypeKind.Pointer:
                {
                    var pointerType = (PointerType)type;

                    // May be a forward reference
                    if (pointerType.Pointee != null)
                    {
                        AssembleInlineType(pointerType.Pointee, builder, shortType);
                    }
                    else
                    {
                        builder.Append("forward");
                    }
                    
                    builder.Append('*');

                    if (pointerType.AddressSpace != AddressSpace.Function)
                    {
                        builder.Append(pointerType.AddressSpace);
                    }
                    break;
                }
                case TypeKind.Array:
                {
                    var arrayType = (ArrayType)type;
                    builder.Append('[');
                    AssembleInlineType(arrayType.ElementType, builder, shortType);
                    builder.AppendFormat(", {0}]", arrayType.Count);
                    break;
                }
                case TypeKind.Texture:
                {
                    var textureType = (TextureType)type;
                    builder.Append(textureType.Dimension);

                    if (textureType.Multisampled)
                    {
                        builder.Append("MS");
                    }

                    if (!shortType)
                    {
                        AssembleInlineType(textureType.SampledType, builder, shortType);
                        builder.AppendFormat(" {0} {1}", textureType.Format, textureType.SamplerMode);
                    }
                    break;
                }
                case TypeKind.Buffer:
                {
                    var bufferType = (BufferType)type;
                    builder.Append("Buffer<");
                    AssembleInlineType(bufferType.ElementType, builder, shortType);
                    builder.Append('>');
                    if (!shortType)
                    {
                        builder.AppendFormat(" {0} {1}", bufferType.TexelType, bufferType.SamplerMode);
                    }

                    break;
                }
                case TypeKind.Sampler:
                {
                    var samplerType = (SamplerType)type;
                    builder.Append("Sampler");
                    break;
                }
                case TypeKind.CBuffer:
                {
                    var samplerType = (SamplerType)type;
                    builder.Append("CBuffer");
                    break;
                }
                case TypeKind.Function:
                {
                    var functionType = (FunctionType)type;
                    builder.Append("Function ");
                    AssembleInlineType(functionType.ReturnType, builder, shortType);

                    if (!shortType)
                    {
                        for (int i = 0; i < functionType.ParameterTypes.Length; i++)
                        {
                            if (i != 0)
                            {
                                builder.Append(", ");
                            }
                        
                            AssembleInlineType(functionType.ParameterTypes[i], builder, shortType);
                        }
                    }
                    break;
                }
                case TypeKind.Struct:
                {
                    var structType = (StructType)type;

                    builder.Append("{ ");
                    for (int i = 0; i < structType.MemberTypes.Length; i++)
                    {
                        if (i != 0)
                        {
                            builder.Append(", ");
                        }
                        
                        AssembleInlineType(structType.MemberTypes[i], builder, shortType);
                    }
                    builder.Append(" }");
                    break;
                }
                case TypeKind.Unexposed:
                {
                    builder.Append("Unexposed");
                    break;
                }
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }
        
        /// <summary>
        /// Current program
        /// </summary>
        private Program _program;

        /// <summary>
        /// All assembled mappings
        /// </summary>
        private Dictionary<UInt64, AssembledMapping> _assembledMappings = new();
            
        /// <summary>
        /// Current line offset
        /// </summary>
        private uint _lineOffset;
    }
}

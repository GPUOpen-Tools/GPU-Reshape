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
using System.Diagnostics;
using Newtonsoft.Json;

namespace Studio.Models.IL
{
    public class Parser
    {
        /// <summary>
        /// Parse a program
        /// </summary>
        /// <param name="json"></param>
        /// <returns>program</returns>
        public Program Parse(string json)
        {
            Program program = new();
            ParseInternal(json, program);
            return program;
        }

        /// <summary>
        /// Parse helper
        /// </summary>
        /// <param name="json">source json</param>
        /// <param name="program">destination program</param>
        private void ParseInternal(string json, Program program)
        {
            // Attempt to deserialize
            dynamic? node;
            try
            {
                node = JsonConvert.DeserializeObject(json);
                if (node == null)
                {
                    return;
                }
            }
            catch
            {
                if (Debugger.IsAttached)
                {
                    throw;
                }

                // Deserialization failed
                return;
            }

            // Parse root as program
            ParseProgram(node, program);
        }

        /// <summary>
        /// Parse the program
        /// </summary>
        /// <param name="node">root node</param>
        /// <param name="program">destination program</param>
        private void ParseProgram(dynamic node, Program program)
        {
            // Set header
            program.AllocatedIdentifiers = node.AllocatedIdentifiers;
            program.EntryPoint = node.EntryPoint;
            program.GUID = node.GUID;

            // Preallocate targets
            program.Types = new Type[node.Types.Count];
            program.Constants = new Constant[node.Constants.Count];
            program.Variables = new Variable[node.Variables.Count];
            program.Functions = new Function[node.Functions.Count];

            // Parse all types
            for (int i = 0; i < program.Types.Length; i++)
            {
                program.Types[i] = ParseType(node.Types[i], program);
                program.Lookup.Add(program.Types[i].ID, program.Types[i]);
            }

            // Parse all type references
            for (int i = 0; i < program.Types.Length; i++)
            {
                ParseTypeReferences(program.Types[i], node.Types[i], program);
            }

            // Parse all constants
            for (int i = 0; i < program.Constants.Length; i++)
            {
                program.Constants[i] = ParseConstant(node.Constants[i], program);
                program.Lookup.Add(program.Constants[i].ID, program.Constants[i]);
            }

            // Parse all variables
            for (int i = 0; i < program.Variables.Length; i++)
            {
                program.Variables[i] = ParseVariable(node.Variables[i], program);
                program.Lookup.Add(program.Variables[i].ID, program.Variables[i]);
            }

            // Parse all functions
            for (int i = 0; i < program.Functions.Length; i++)
            {
                program.Functions[i] = ParseFunction(node.Functions[i], program);
                program.Lookup.Add(program.Functions[i].ID, program.Functions[i]);
            }
        }

        /// <summary>
        /// Parse a type
        /// </summary>
        private Type ParseType(dynamic node, Program program)
        {
            // Handle type
            switch ((TypeKind)node.Kind)
            {
                case TypeKind.None:
                    return new Type();
                case TypeKind.Bool:
                {
                    return new BoolType()
                    {
                        Kind = TypeKind.Bool,
                        ID = node.ID
                    };
                }
                case TypeKind.Void:
                {
                    return new VoidType()
                    {
                        Kind = TypeKind.Void,
                        ID = node.ID
                    };
                }
                case TypeKind.Int:
                {
                    return new IntType()
                    {
                        Kind = TypeKind.Int,
                        ID = node.ID,
                        BitWidth = node.Width,
                        Signedness = node.Signed
                    };
                }
                case TypeKind.FP:
                {
                    return new FPType()
                    {
                        Kind = TypeKind.FP,
                        ID = node.ID,
                        BitWidth = node.Width
                    };
                }
                case TypeKind.Vector:
                {
                    return new VectorType()
                    {
                        Kind = TypeKind.Vector,
                        ID = node.ID,
                        Dimension = node.Dim
                    };
                }
                case TypeKind.Matrix:
                {
                    return new MatrixType()
                    {
                        Kind = TypeKind.Matrix,
                        ID = node.ID,
                        Rows = node.Rows,
                        Columns = node.Columns
                    };
                }
                case TypeKind.Pointer:
                {
                    return new PointerType()
                    {
                        Kind = TypeKind.Pointer,
                        ID = node.ID,
                        AddressSpace = (AddressSpace)node.AddressSpace
                    };
                }
                case TypeKind.Array:
                {
                    return new ArrayType()
                    {
                        Kind = TypeKind.Array,
                        ID = node.ID,
                        Count = node.Count
                    };
                }
                case TypeKind.Texture:
                {
                    return new TextureType()
                    {
                        Kind = TypeKind.Texture,
                        ID = node.ID,
                        Dimension = node.Dimension,
                        Format = (Format)node.Format,
                        SamplerMode = (ResourceSamplerMode)node.SamplerMode,
                        Multisampled = node.MultiSampled,
                    };
                }
                case TypeKind.Buffer:
                {
                    return new BufferType()
                    {
                        Kind = TypeKind.Buffer,
                        ID = node.ID,
                        TexelType = (Format)node.TexelType,
                        SamplerMode = (ResourceSamplerMode)node.SamplerMode
                    };
                }
                case TypeKind.Sampler:
                {
                    return new SamplerType()
                    {
                        Kind = TypeKind.Sampler,
                        ID = node.ID
                    };
                }
                case TypeKind.CBuffer:
                {
                    return new CBufferType()
                    {
                        Kind = TypeKind.CBuffer,
                        ID = node.ID
                    };
                }
                case TypeKind.Function:
                {
                    return new FunctionType()
                    {
                        Kind = TypeKind.Function,
                        ID = node.ID
                    };
                }
                case TypeKind.Struct:
                {
                    return new StructType()
                    {
                        Kind = TypeKind.Struct,
                        ID = node.ID
                    };
                }
                case TypeKind.Unexposed:
                {
                    return new UnexposedType()
                    {
                        Kind = TypeKind.Unexposed,
                        ID = node.ID
                    };
                }
                default:
                {
                    throw new ArgumentOutOfRangeException();
                }
            }
        }

        /// <summary>
        /// Parse a type
        /// </summary>
        private void ParseTypeReferences(Type type, dynamic node, Program program)
        {
            // Handle type
            switch (type.Kind)
            {
                default:
                    break;
                case TypeKind.Vector:
                {
                    var typed = (VectorType)type;
                    typed.ContainedType = (Type)program.Lookup[(uint)node.Contained];
                    break;
                }
                case TypeKind.Matrix:
                {
                    var typed = (MatrixType)type;
                    typed.ContainedType = (Type)program.Lookup[(uint)node.Contained];
                    break;
                }
                case TypeKind.Pointer:
                {
                    var typed = (PointerType)type;
                    typed.Pointee = (Type)program.Lookup[(uint)node.Pointee];
                    break;
                }
                case TypeKind.Array:
                {
                    var typed = (ArrayType)type;
                    typed.ElementType = (Type)program.Lookup[(uint)node.Contained];
                    break;
                }
                case TypeKind.Texture:
                {
                    var typed = (TextureType)type;
                    typed.SampledType = (Type)program.Lookup[(uint)node.SampledType];
                    break;
                }
                case TypeKind.Buffer:
                {
                    var typed = (BufferType)type;
                    typed.ElementType = (Type)program.Lookup[(uint)node.ElementType];
                    break;
                }
                case TypeKind.Function:
                {
                    var typed = (FunctionType)type;
                    typed.ReturnType = (Type)program.Lookup[(uint)node.ReturnType];

                    typed.ParameterTypes = new Type[node.ParameterTypes.Count];
                    for (int i = 0; i < node.ParameterTypes.Count; i++)
                    {
                        typed.ParameterTypes[i] = (Type)program.Lookup[(uint)node.ParameterTypes[i]];
                    }

                    break;
                }
                case TypeKind.Struct:
                {
                    var typed = (StructType)type;

                    typed.MemberTypes = new Type[node.Members.Count];
                    for (int i = 0; i < typed.MemberTypes.Length; i++)
                    {
                        typed.MemberTypes[i] = (Type)program.Lookup[(uint)node.Members[i]];
                    }

                    break;
                }
            }
        }

        /// <summary>
        /// Parse a constant
        /// </summary>
        private Constant ParseConstant(dynamic node, Program program)
        {
            Constant constant;

            // Assume kind
            var kind = (ConstantKind)node.Kind;
            
            // Handle kind
            switch (kind)
            {
                case ConstantKind.None:
                {
                    constant = new Constant();
                    break;
                }
                case ConstantKind.Undef:
                {
                    constant = new UndefConstant();
                    break;
                }
                case ConstantKind.Bool:
                {
                    constant = new BoolConstant()
                    {
                        Value = (bool)node.Value
                    };
                    break;
                }
                case ConstantKind.Int:
                {
                    constant = new IntConstant()
                    {
                        Value = (Int64)node.Value
                    };
                    break;
                }
                case ConstantKind.FP:
                {
                    constant = new FPConstant()
                    {
                        Value = (double)node.Value
                    };
                    break;
                }
                case ConstantKind.Struct:
                {
                    StructConstant _struct = new();

                    _struct.Members = new Constant[node.Members.Count];

                    for (int i = 0; i < _struct.Members.Length; i++)
                    {
                        _struct.Members[i] = (Constant)program.Lookup[(uint)node.Members[i]];
                    }

                    constant = _struct;
                    break;
                }
                case ConstantKind.Null:
                {
                    constant = new NullConstant();
                    break;
                }
                case ConstantKind.Unexposed:
                {
                    constant = new UnexposedConstant();
                    break;
                }
                default:
                    throw new ArgumentOutOfRangeException();
            }

            // Set commons
            constant.ID = node.ID;
            constant.Kind = node.Kind;
            constant.Type = (Type)program.Lookup[(uint)node.Type];

            // OK
            return constant;
        }

        /// <summary>
        /// Parse a variable
        /// </summary>
        private Variable ParseVariable(dynamic node, Program program)
        {
            return new Variable()
            {
                ID = node.ID,
                AddressSpace = (AddressSpace)node.AddressSpace,
                Type = (Type)program.Lookup[(uint)node.Type]
            };
        }

        /// <summary>
        /// Parse a function
        /// </summary>
        private Function ParseFunction(dynamic node, Program program)
        {
            Function function = new();
            
            // Set commons
            function.ID = node.ID;
            function.Type = (FunctionType)program.Lookup[(uint)node.Type];
            
            // Preallocate targets
            function.Parameters = new uint[node.Parameters.Count];
            function.BasicBlocks = new BasicBlock[node.BasicBlocks.Count];

            // Parse parameters
            for (int i = 0; i < function.Parameters.Length; i++)
            {
                function.Parameters[i] = node.Parameters[i];
            }

            // Parse basic blocks
            for (int i = 0; i < function.BasicBlocks.Length; i++)
            {
                function.BasicBlocks[i] = ParseBasicBlock(node.BasicBlocks[i], program);
                program.Lookup.Add(function.BasicBlocks[i].ID, function.BasicBlocks[i]);
            }
            
            // OK
            return function;
        }

        /// <summary>
        /// Parse a basic block
        /// </summary>
        private BasicBlock ParseBasicBlock(dynamic node, Program program)
        {
            BasicBlock block = new();
            
            // Set commons
            block.ID = node.ID;
            
            // Preallocate targets
            block.Instructions = new Instruction[node.Instructions.Count];

            // Parse all instructions
            for (int i = 0; i < block.Instructions.Length; i++)
            {
                block.Instructions[i] = ParseInstruction(node.Instructions[i], program);

                // Lookup only if a valid value
                if (block.Instructions[i].ID != uint.MaxValue)
                {
                    program.Lookup.Add(block.Instructions[i].ID, block.Instructions[i]);
                }
            }
            
            // OK
            return block;
        }

        /// <summary>
        /// Parse an instruction
        /// </summary>
        private Instruction ParseInstruction(dynamic node, Program program)
        {
            // Get op code
            var opCode = (OpCode)node.OpCode;

            // Target instruction
            Instruction instruction;

            // Handle op code
            switch (opCode)
            {
                case OpCode.None:
                {
                    instruction = new Instruction();
                    break;
                }
                case OpCode.Unexposed:
                {
                    instruction = new UnexposedInstruction()
                    {
                        Symbol = node.Symbol,
                        UnexposedOp = node.UnexposedOp
                    };
                    break;
                }
                case OpCode.Literal:
                {
                    instruction = new LiteralInstruction()
                    {
                        LiteralType = (LiteralType)node.Type,
                        Integral = (Int64)node.Value,
                        FP = (double)node.Value
                    };
                    break;
                }
                case OpCode.Any:
                case OpCode.All:
                case OpCode.Trunc:
                case OpCode.IsInf:
                case OpCode.IsNaN:
                {
                    instruction = new UnaryInstruction()
                    {
                        Value = node.Value
                    };
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
                    instruction = new BinaryInstruction()
                    {
                        LHS = node.LHS,
                        RHS = node.RHS
                    };
                    break;
                }
                case OpCode.Select:
                {
                    instruction = new SelectInstruction()
                    {
                        Condition = node.Condition,
                        Pass = node.Pass,
                        Fail = node.Fail
                    };
                    break;
                }
                case OpCode.Branch:
                {
                    BranchInstruction branch = new()
                    {
                        Branch = node.Branch
                    };

                    if (node.Merge != null)
                    {
                        branch.ControlFlow = new ControlFlow()
                        {
                            MergeBranch = node.Merge
                        };

                        if (node.Continue != null)
                        {
                            branch.ControlFlow.ContinueBranch = node.Continue;
                        }
                    }
                    
                    instruction = branch;
                    break;
                }
                case OpCode.BranchConditional:
                {
                    BranchConditionalInstruction branch = new()
                    {
                        Condition = node.Condition,
                        Pass = node.Pass,
                        Fail = node.Fail
                    };

                    if (node.Merge != null)
                    {
                        branch.ControlFlow = new ControlFlow()
                        {
                            MergeBranch = node.Merge
                        };

                        if (node.Continue != null)
                        {
                            branch.ControlFlow.ContinueBranch = node.Continue;
                        }
                    }
                    
                    instruction = branch;
                    break;
                }
                case OpCode.Switch:
                {
                    SwitchInstruction _switch = new()
                    {
                        Value = node.Value,
                        Default = node.Default
                    };

                    _switch.BranchValues = new Tuple<uint, uint>[node.Cases.Count];
                    for (int i = 0; i < _switch.BranchValues.Length; i++)
                    {
                        _switch.BranchValues[i] = Tuple.Create((uint)node.Cases[i].Branch, (uint)node.Cases[i].Literal);
                    }

                    if (node.Merge != null)
                    {
                        _switch.ControlFlow = new ControlFlow()
                        {
                            MergeBranch = node.Merge
                        };

                        if (node.Continue != null)
                        {
                            _switch.ControlFlow.ContinueBranch = node.Continue;
                        }
                    }

                    instruction = _switch;
                    break;
                }
                case OpCode.Phi:
                {
                    PhiInstruction phi = new();

                    phi.BranchValues = new Tuple<uint, uint>[node.Values.Count];
                    for (int i = 0; i < phi.BranchValues.Length; i++)
                    {
                        phi.BranchValues[i] = Tuple.Create((uint)node.Values[i].Branch, (uint)node.Values[i].Value);
                    }

                    instruction = phi;
                    break;
                }
                case OpCode.Return:
                {
                    instruction = new ReturnInstruction()
                    {
                        Value = node.Value
                    };
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
                    instruction = new AtomicInstruction()
                    {
                        Address = node.Address,
                        Value = node.Value
                    };
                    break;
                }
                case OpCode.AtomicCompareExchange:
                {
                    instruction = new AtomicCompareInstruction()
                    {
                        Address = node.Address,
                        Value = node.Value,
                        Comparator = node.Comparator
                    };
                    break;
                }
                case OpCode.BitShiftLeft:
                case OpCode.BitShiftRight:
                {
                    instruction = new BitShiftInstruction()
                    {
                        Value = node.Value,
                        Shift = node.Shift
                    };
                    break;
                }
                case OpCode.AddressChain:
                {
                    AddressChainInstruction chain = new()
                    {
                        Composite = node.Composite
                    };

                    chain.Chains = new uint[node.Chains.Count];
                    for (int i = 0; i < chain.Chains.Length; i++)
                    {
                        chain.Chains[i] = node.Chains[i];
                    }

                    instruction = chain;
                    break;
                }
                case OpCode.Extract:
                {
                    instruction = new ExtractInstruction()
                    {
                        Composite = node.Composite,
                        Index = node.Index
                    };
                    break;
                }
                case OpCode.Insert:
                {
                    instruction = new InsertInstruction()
                    {
                        Composite = node.Composite,
                        Value = node.Value
                    };
                    break;
                }
                case OpCode.FloatToInt:
                {
                    instruction = new FloatToIntInstruction()
                    {
                        Value = node.Value
                    };
                    break;
                }
                case OpCode.IntToFloat:
                {
                    instruction = new IntToFloatInstruction()
                    {
                        Value = node.Value
                    };
                    break;
                }
                case OpCode.BitCast:
                {
                    instruction = new BitCastInstruction()
                    {
                        Value = node.Value
                    };
                    break;
                }
                case OpCode.Export:
                {
                    ExportInstruction export = new()
                    {
                        ExportID = node.ExportID
                    };

                    export.Values = new uint[node.Values.Count];
                    for (int i = 0; i < export.Values.Length; i++)
                    {
                        export.Values[i] = node.Chains[i];
                    }

                    instruction = export;
                    break;
                }
                case OpCode.Alloca:
                {
                    instruction = new AllocaInstruction();
                    break;
                }
                case OpCode.Load:
                {
                    instruction = new LoadInstruction()
                    {
                        Address = node.Address
                    };
                    break;
                }
                case OpCode.Store:
                {
                    instruction = new StoreInstruction()
                    {
                        Address = node.Address,
                        Value = node.Value
                    };
                    break;
                }
                case OpCode.StoreOutput:
                {
                    instruction = new StoreOutputInstruction()
                    {
                        Index = node.Index,
                        Row = node.Row,
                        Column = node.Column,
                        Value = node.Value
                    };
                    break;
                }
                case OpCode.SampleTexture:
                {
                    instruction = new SampleTextureInstruction()
                    {
                        SampleMode = (TextureSampleMode)node.SampleMode,
                        Texture = node.Texture,
                        Sampler = node.Sampler,
                        Coordinate = node.Coordinate,
                        Reference = node.Reference,
                        Lod = node.Lod,
                        Bias  = node.Bias,
                        DDx = node.DDx,
                        DDy = node.DDy
                    };
                    break;
                }
                case OpCode.StoreTexture:
                {
                    instruction = new StoreTextureInstruction()
                    {
                        Texture = node.Texture,
                        Index = node.Index,
                        Texel = node.Texel,
                        ComponentMask = node.ComponentMask
                    };
                    break;
                }
                case OpCode.LoadTexture:
                {
                    instruction = new LoadTextureInstruction()
                    {
                        Texture = node.Texture,
                        Index = node.Index,
                        Offset = node.Offset,
                        Mip = node.Mip
                    };
                    break;
                }
                case OpCode.StoreBuffer:
                {
                    instruction = new StoreBufferInstruction()
                    {
                        Buffer = node.Buffer,
                        Index = node.Index,
                        Value = node.Value,
                        ComponentMask = node.ComponentMask
                    };
                    break;
                }
                case OpCode.LoadBuffer:
                {
                    instruction = new LoadBufferInstruction()
                    {
                        Buffer = node.Buffer,
                        Index = node.Index,
                        Offset = node.Offset
                    };
                    break;
                }
                case OpCode.ResourceToken:
                {
                    instruction = new ResourceTokenInstruction()
                    {
                        Resource = node.Resource
                    };
                    break;
                }
                case OpCode.ResourceSize:
                {
                    instruction = new ResourceSizeInstruction()
                    {
                        Resource = node.Resource
                    };
                    break;
                }
                default:
                    throw new ArgumentOutOfRangeException();
            }

            // Set commons
            instruction.OpCode = opCode;
            
            // Assign type if present
            if (node.Type != null)
            {
                instruction.Type = (Type)program.Lookup[(uint)node.Type];
            }

            // Assign id if present
            if (node.ID != null)
            {
                instruction.ID = node.ID;
            }

            // OK
            return instruction;
        }
    }
}
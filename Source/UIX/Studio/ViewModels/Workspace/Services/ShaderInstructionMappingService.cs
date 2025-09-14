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
using System.Linq;
using Avalonia.Threading;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Services
{
    public class ShaderInstructionMappingService : ReactiveObject, IPropertyService, IShaderInstructionMappingService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _connectionViewModel, value);
                OnConnectionChanged();
            }
        }

        /// <summary>
        /// Get or create source associations
        /// </summary>
        public ShaderInstructionSourceAssociationViewModel GetOrCreateSourceAssociation(UInt64 shaderGUID, AssembledInstructionMapping location)
        {
            var key = Tuple.Create(shaderGUID, location);
            
            // Sync cached check
            ShaderInstructionSourceAssociationViewModel? association;
            lock (this)
            {
                if (_shaderSourceAssociations.TryGetValue(key, out association))
                {
                    return association;
                }
                
                association = new ShaderInstructionSourceAssociationViewModel();
                _shaderSourceAssociations.Add(key, association);
            }

            // Request from backend
            if (_connectionViewModel?.GetSharedBus() is {} bus)
            {
                var request = bus.Add<GetShaderSourceInstructionMappingMessage>();
                request.shaderGUID = shaderGUID;
                request.basicBlockId = location.BasicBlockId;
                request.instructionIndex = location.InstructionIndex;
                request.codeOffset = location.CodeOffset;
            }
                
            return association;
        }

        /// <summary>
        /// Get or create instruction association
        /// </summary>
        public ShaderInstructionAssociationViewModel GetOrCreateInstructionAssociation(ShaderShaderInstructionAssociationLocation location)
        {
            // Sync cached check
            ShaderInstructionAssociationViewModel? association;
            lock (this)
            {
                if (_shaderInstructionAssociations.TryGetValue(location, out association))
                {
                    return association;
                }
                
                association = new ShaderInstructionAssociationViewModel();
                _shaderInstructionAssociations.Add(location, association);
            }

            // Request from backend
            if (_connectionViewModel?.GetSharedBus() is {} bus)
            {
                var request = bus.Add<GetShaderInstructionMappingMessage>();
                request.shaderGUID = location.SGUID;
                request.fileUID = (uint)location.FileUID;
                request.line = (uint)location.Line;
            }
                
            return association;
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            if (_connectionViewModel == null)
            {
                return;
            }
            
            // Register listeners
            _connectionViewModel.Bridge?.Register(this);

            // Submit request
            var request = _connectionViewModel.GetSharedBus().Add<GetFeaturesMessage>();
            request.featureBitSet = UInt64.MaxValue;
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsOrdered())
            {
                return;
            }

            // Create view
            var view = new OrderedMessageView(streams);

            // Consume all messages
            lock (this)
            {
                foreach (OrderedMessage message in view)
                {
                    switch (message.ID)
                    {
                        case ShaderInstructionMappingSetMessage.ID:
                        {
                            var typed = message.Get<ShaderInstructionMappingSetMessage>();
                            if (typed.found == 0)
                            {
                                Studio.Logging.Warning("Source to instruction multi-association failed");
                            }

                            // To loocation for lookups
                            ShaderShaderInstructionAssociationLocation location = new()
                            {
                                SGUID = typed.shaderGUID,
                                FileUID = (int)typed.fileUID,
                                Line = (int)typed.line
                            };

                            // Get the per-shader associations
                            ShaderInstructionAssociationViewModel associations = _shaderInstructionAssociations.GetOrAddDefault(location);
                            
                            // Flatten on message thread
                            ShaderInstructionMappingMessage.FlatInfo[] mappings = new StaticMessageView<ShaderInstructionMappingMessage>(typed.mappings.Stream).Select(x => x.Flat).ToArray();

                            // Assign on ui thread
                            Dispatcher.UIThread.InvokeAsync(() =>
                            {
                                foreach (ShaderInstructionMappingMessage.FlatInfo mapping in mappings)
                                {
                                    associations.Mappings.Add(new AssembledInstructionMapping()
                                    {
                                        BasicBlockId = mapping.basicBlockId,
                                        InstructionIndex = mapping.instructionIndex,
                                        CodeOffset = mapping.codeOffset
                                    });
                                }

                                associations.Populated = true;
                            });
                            break;
                        }
                        case ShaderSourceInstructionMappingMessage.ID:
                        {
                            var typed = message.Get<ShaderSourceInstructionMappingMessage>();
                            if (typed.found == 0)
                            {
                                Studio.Logging.Warning("Instruction to source single-association failed");
                                break;
                            }

                            // Flatten the message
                            ShaderSourceInstructionMappingMessage.FlatInfo flat = typed.Flat;

                            // Get the per-shader association
                            ShaderInstructionSourceAssociationViewModel association = _shaderSourceAssociations.GetOrAddDefault(Tuple.Create(
                                flat.shaderGUID,
                                new AssembledInstructionMapping()
                                {
                                    BasicBlockId = flat.basicBlockId,
                                    InstructionIndex = flat.instructionIndex,
                                    CodeOffset = flat.codeOffset
                                }
                            ));
                            
                            // Assign on ui thread
                            Dispatcher.UIThread.InvokeAsync(() =>
                            {
                                association.Location = new ShaderLocation()
                                {
                                    BasicBlockId = flat.basicBlockId,
                                    InstructionIndex = flat.instructionIndex,
                                    FileUID = (int)flat.fileUID,
                                    Line = (int)flat.line
                                };
                            });
                            break;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// All mapped instruction associations
        /// </summary>
        private Dictionary<ShaderShaderInstructionAssociationLocation, ShaderInstructionAssociationViewModel> _shaderInstructionAssociations = new();

        /// <summary>
        /// All mapped source associations
        /// </summary>
        private Dictionary<Tuple<UInt64, AssembledInstructionMapping>, ShaderInstructionSourceAssociationViewModel> _shaderSourceAssociations = new();

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}
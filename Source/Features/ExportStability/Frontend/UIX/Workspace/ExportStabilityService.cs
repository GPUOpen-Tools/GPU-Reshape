﻿// 
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
using System.Threading.Tasks;
using Avalonia.Threading;
using Studio.ViewModels.Workspace;
using Message.CLR;
using Bridge.CLR;
using GRS.Features.ResourceBounds.UIX.Workspace.Properties.Instrumentation;
using ReactiveUI;
using Runtime.Threading;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.IL;
using Studio.Models.Instrumentation;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.ResourceBounds.UIX.Workspace
{
    public class ExportStabilityService : IInstrumentationPropertyService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Feature name
        /// </summary>
        public string Name => "Export Stability";
        
        /// <summary>
        /// Feature category
        /// </summary>
        public string Category => string.Empty;

        /// <summary>
        /// Feature flags
        /// </summary>
        public InstrumentationFlag Flags => InstrumentationFlag.Standard;
        
        /// <summary>
        /// Assigned workspace
        /// </summary>
        public IWorkspaceViewModel ViewModel { get; }
        
        public ExportStabilityService(IWorkspaceViewModel viewModel)
        {
            ViewModel = viewModel;
            
            // Add listener to bridge
            viewModel.Connection?.Bridge?.Register(UnstableExportMessage.ID, this);
            
            // Get properties
            _messageCollectionViewModel = viewModel.PropertyCollection.GetProperty<IMessageCollectionViewModel>();
            
            // Get services
            _shaderMappingService = viewModel.PropertyCollection.GetService<IShaderMappingService>();
            _versioningService = ViewModel.PropertyCollection.GetService<IVersioningService>();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            ViewModel.Connection?.Bridge?.Deregister(UnstableExportMessage.ID, this);
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsChunked(UnstableExportMessage.ID)) 
                return;

            var view = new ChunkedMessageView<UnstableExportMessage>(streams);

            // Latent update set
            var enqueued = new Dictionary<uint, uint>();

            // Preallocate initial latents
            foreach (var kv in _reducedMessages)
            {
                enqueued.Add(kv.Key, 0);
            }

            foreach (UnstableExportMessage message in view)
            {
                // Add to latent set
                if (enqueued.TryGetValue(message.Key, out uint enqueuedCount))
                {
                    enqueued[message.Key] = enqueuedCount + 1u;
                }
                else
                {
                    // Create object
                    var validationObject = new ValidationObject()
                    {
                        Traits = _traits,
                        Severity = ValidationSeverity.Info,
                        Content = $"Exporting {(message.Flat.isNaN == 1 ? "NaN" : "Inf")}",
                        Count = 1u
                    };
                    
                    // Register with latent
                    enqueued.Add(message.Key, 1u);

                    // Shader view model injection
                    validationObject.WhenAnyValue(x => x.Segment).WhereNotNull().Subscribe(x =>
                    {
                        // Try to get shader collection
                        var shaderCollectionViewModel = ViewModel.PropertyCollection.GetProperty<IShaderCollectionViewModel>();
                        if (shaderCollectionViewModel != null)
                        {
                            // Get the respective shader
                            ShaderViewModel shaderViewModel = shaderCollectionViewModel.GetOrAddShader(x.Location.SGUID);

                            // Append validation object to target shader
                            shaderViewModel.ValidationObjects.Add(validationObject);
                        }
                    });

                    // Enqueue segment binding
                    _shaderMappingService?.EnqueueMessage(validationObject, message.sguid);

                    // Insert lookup
                    _reducedMessages.Add(message.Key, validationObject);

                    // Add to UI visible collection
                    Dispatcher.UIThread.InvokeAsync(() => { _messageCollectionViewModel?.ValidationObjects.Add(validationObject); });
                }

                // Detailed?
                if (message.HasChunk(UnstableExportMessage.Chunk.Detail))
                {
                    UnstableExportMessage.DetailChunk detailChunk = message.GetDetailChunk();

                    // To token
                    var token = new ResourceToken()
                    {
                        Token = detailChunk.token
                    };

                    // Get detailed view model
                    if (!_reducedDetails.TryGetValue(message.Key, out ResourceValidationDetailViewModel? detailViewModel))
                    {
                        // Not found, find the object
                        if (!_reducedMessages.TryGetValue(message.Key, out ValidationObject? validationObject))
                        {
                            continue;
                        }

                        // Create the missing detail view model
                        detailViewModel = new ResourceValidationDetailViewModel();
                        
                        // Assign on UI thread
                        Dispatcher.UIThread.InvokeAsync(() =>
                        {
                            validationObject.DetailViewModel = detailViewModel;
                        });
                        
                        // Add lookup
                        _reducedDetails.Add(message.Key, detailViewModel);
                    }

                    // Try to find resource
                    Resource resource = _versioningService?.GetResource(token.PUID, streams.VersionID) ?? new Resource()
                    {
                        PUID = token.PUID,
                        Version = streams.VersionID,
                        Name = $"#{token.PUID}",
                        IsUnknown = true
                    };
                    
                    // Get resource
                    ResourceValidationObject resourceValidationObject = detailViewModel.FindOrAddResource(resource);

                    // Compose detailed message
                    resourceValidationObject.AddUniqueInstance(_reducedMessages[message.Key].Content);
                }
            }
            
            // Update counts on main thread
            foreach (var kv in enqueued)
            {
                if (_reducedMessages.TryGetValue(kv.Key, out ValidationObject? value))
                {
                    if (kv.Value > 0)
                    {
                        ValidationMergePumpBus.Increment(value, kv.Value);
                    }
                }
            }
        }

        /// <summary>
        /// Check if a target may be instrumented
        /// </summary>
        public bool IsInstrumentationValidFor(IInstrumentableObject instrumentable)
        {
            return instrumentable
                .GetWorkspaceCollection()?
                .GetProperty<IFeatureCollectionViewModel>()?
                .HasFeature("Export Stability") ?? false;
        }

        /// <summary>
        /// Create an instrumentation property
        /// </summary>
        /// <param name="target"></param>
        /// <param name="replication"></param>
        /// <returns></returns>
        public async Task<IPropertyViewModel?> CreateInstrumentationObjectProperty(IPropertyViewModel target, bool replication)
        {
            // Get feature info on target
            FeatureInfo? featureInfo = (target as IInstrumentableObject)?
                .GetWorkspaceCollection()?
                .GetProperty<IFeatureCollectionViewModel>()?
                .GetFeature("Export Stability");

            // Invalid or already exists?
            if (featureInfo == null || target.HasProperty<ExportStabilityPropertyViewModel>())
            {
                return null;
            }
                
            // Create property
            return new ExportStabilityPropertyViewModel()
            {
                Parent = target,
                ConnectionViewModel = target.ConnectionViewModel,
                FeatureInfo = featureInfo.Value
            };
        }
        
        /// <summary>
        /// All reduced resource messages
        /// </summary>
        private Dictionary<uint, ValidationObject> _reducedMessages = new();

        /// <summary>
        /// All reduced resource messages
        /// </summary>
        private Dictionary<uint, ResourceValidationDetailViewModel> _reducedDetails = new();
        
        /// <summary>
        /// Shared validation traits
        /// </summary>
        private ValidationInstructionExcludeTrait _traits = new ()
        {
            ExcludedOps = new OpCode[]
            {
                OpCode.StoreOutput
            }
        };

        /// <summary>
        /// Segment mapping
        /// </summary>
        private IShaderMappingService? _shaderMappingService;

        /// <summary>
        /// Validation container
        /// </summary>
        private IMessageCollectionViewModel? _messageCollectionViewModel;

        /// <summary>
        /// Versioning service
        /// </summary>
        private IVersioningService? _versioningService;
    }
}
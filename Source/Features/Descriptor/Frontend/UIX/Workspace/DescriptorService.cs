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
using Studio.Models.Instrumentation;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Descriptor.UIX.Workspace
{
    public class DescriptorService : IInstrumentationPropertyService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Feature name
        /// </summary>
        public string Name => "Descriptor";
        
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
        
        public DescriptorService(IWorkspaceViewModel viewModel)
        {
            ViewModel = viewModel;
            
            // Add listener to bridge
            viewModel.Connection?.Bridge?.Register(DescriptorMismatchMessage.ID, this);
            
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
            ViewModel.Connection?.Bridge?.Deregister(DescriptorMismatchMessage.ID, this);
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsChunked(DescriptorMismatchMessage.ID)) 
                return;

            var view = new ChunkedMessageView<DescriptorMismatchMessage>(streams);

            // Latent update set
            var enqueued = new Dictionary<uint, uint>();

            // Preallocate initial latents
            foreach (var kv in _reducedMessages)
            {
                enqueued.Add(kv.Key, 0);
            }

            // Type lookup
            string[] typeLookup = new[] { "Texture", "Buffer", "CBuffer", "Sampler" };

            foreach (DescriptorMismatchMessage message in view)
            {
                // Add to latent set
                if (enqueued.TryGetValue(message.Key, out uint enqueuedCount))
                {
                    enqueued[message.Key] = enqueuedCount + 1u;
                }
                else
                {
                    // Composed message
                    string contents;
                    if (message.isUndefined != 0)
                    {
                        contents = $"Descriptor is undefined, shader expected {typeLookup[message.Flat.compileType]}";
                    }
                    else if (message.isOutOfBounds != 0)
                    {
                        contents = $"Descriptor indexing out of bounds, shader expected {typeLookup[message.Flat.compileType]}";
                    }
                    else if (message.isTableNotBound != 0)
                    {
                        contents = $"Descriptor table not bound, shader expected {typeLookup[message.Flat.compileType]}";
                    }
                    else
                    {
                        contents = $"Descriptor mismatch detected, shader expected {typeLookup[message.Flat.compileType]} but received {typeLookup[message.Flat.runtimeType]}";
                    }

                    // Create object
                    var validationObject = new ValidationObject()
                    {
                        Content = contents,
                        Severity = ValidationSeverity.Error,
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
                if (message.HasChunk(DescriptorMismatchMessage.Chunk.Detail))
                {
                    DescriptorMismatchMessage.DetailChunk detailChunk = message.GetDetailChunk();

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
                .HasFeature("Descriptor") ?? false;
        }

        /// <summary>
        /// Create an instrumentation property
        /// </summary>
        /// <param name="target"></param>
        /// <param name="replication"></param>
        /// <returns></returns>
        public async Task<IPropertyViewModel?> CreateInstrumentationObjectProperty(IPropertyViewModel target, bool replication)
        {
            // Get feature in parent
            FeatureInfo? featureInfo = (target as IInstrumentableObject)?
                .GetWorkspaceCollection()?
                .GetProperty<IFeatureCollectionViewModel>()?
                .GetFeature("Descriptor");

            // Invalid or already exists?
            if (featureInfo == null || target.HasProperty<DescriptorPropertyViewModel>())
            {
                return null;
            }
                
            // Create property
            return new DescriptorPropertyViewModel()
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
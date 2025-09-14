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
using System.Threading.Tasks;
using Avalonia.Threading;
using Studio.ViewModels.Workspace;
using Message.CLR;
using GRS.Features.ResourceBounds.UIX.Workspace.Properties.Instrumentation;
using ReactiveUI;
using Runtime.Threading;
using Runtime.Utils.Workspace;
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
    public class ResourceBoundsService : IInstrumentationPropertyService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Feature name
        /// </summary>
        public string Name => "Resource Bounds";
        
        /// <summary>
        /// Feature category
        /// </summary>
        public string Category => string.Empty;

        /// <summary>
        /// Feature flags
        /// </summary>
        public InstrumentationFlag Flags => InstrumentationFlag.Standard;
        
        /// <summary>
        /// Parent view model
        /// </summary>
        public IWorkspaceViewModel ViewModel { get; }

        public ResourceBoundsService(IWorkspaceViewModel viewModel)
        {
            ViewModel = viewModel;

            // Add listener to bridge
            viewModel.Connection?.Bridge?.Register(ResourceIndexOutOfBoundsMessage.ID, this);

            // Get properties
            _messageCollectionViewModel = viewModel.PropertyCollection.GetProperty<IMessageCollectionViewModel>();

            // Get services
            _shaderMappingService = viewModel.PropertyCollection.GetService<IShaderSourceMappingService>();
            _versioningService = ViewModel.PropertyCollection.GetService<IVersioningService>();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            ViewModel.Connection?.Bridge?.Deregister(ResourceIndexOutOfBoundsMessage.ID, this);
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsChunked(ResourceIndexOutOfBoundsMessage.ID))
                return;

            var view = new ChunkedMessageView<ResourceIndexOutOfBoundsMessage>(streams);
            
            // Latent update set
            var enqueued = new Dictionary<uint, uint>();
            
            // Preallocate initial latents
            foreach (var kv in _reducedMessages)
            {
                enqueued.Add(kv.Key, 0);
            }

            foreach (ResourceIndexOutOfBoundsMessage message in view)
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
                        Severity = SourceObjectSeverity.Warning,
                        Content = $"{(message.Flat.isTexture == 1 ? "Texture" : "Buffer")} {(message.Flat.isWrite == 1 ? "write" : "read")} out of bounds",
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

                // Formatted?
                // TODO: Optimize the hell out of this, current version is not good enough
                if (message.IsChunked())
                {
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

                    // Formatted message
                    StringBuilder builder = new();
                    builder.Append("Out of bounds");
                    
                    // Destination resource
                    Resource resource;

                    // Handle detail
                    if (message.HasChunk(ResourceIndexOutOfBoundsMessage.Chunk.Detail))
                    {
                        ResourceIndexOutOfBoundsMessage.DetailChunk detailChunk = message.GetDetailChunk();
                        
                        // Unpack token
                        ResourceToken token = new()
                        {
                            Token = detailChunk.token
                        };
                        
                        // Try to find resource
                        resource = _versioningService?.GetResource(token.PUID, streams.VersionID) ?? new Resource()
                        {
                            PUID = token.PUID,
                            Version = streams.VersionID,
                            Name = $"#{token.PUID}",
                            IsUnknown = true
                        };
                        
                        // Append coordinates
                        uint[] coordinate = detailChunk.coordinate;
                        builder.Append($" at coordinates [{coordinate[0]}, {coordinate[1]}, {coordinate[2]}]");
                    }
                    else
                    {
                        resource = new Resource()
                        {
                            Name = "Unknown",
                            IsUnknown = true
                        };
                    }

                    // Handle traceback
                    if (message.HasChunk(ResourceIndexOutOfBoundsMessage.Chunk.Traceback))
                    {
                        ResourceIndexOutOfBoundsMessage.TracebackChunk tracebackChunk = message.GetTracebackChunk();
                        builder.Append($" at {TracebackUtils.Format(ViewModel, tracebackChunk.GetModel())}");
                    }

                    // Append message on resource
                    ResourceValidationObject resourceValidationObject = detailViewModel.FindOrAddResource(resource);
                    resourceValidationObject.AddUniqueInstance(builder.ToString());
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
                .HasFeature("Resource Bounds") ?? false;
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
                .GetFeature("Resource Bounds");

            // Invalid or already exists?
            if (featureInfo == null || target.HasProperty<ResourceBoundsPropertyViewModel>())
            {
                return null;
            }

            // Create the property
            return new ResourceBoundsPropertyViewModel()
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
        /// Shader segment mapper
        /// </summary>
        private IShaderSourceMappingService? _shaderMappingService;

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
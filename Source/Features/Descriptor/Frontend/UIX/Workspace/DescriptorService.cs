using System;
using System.Collections.Generic;
using Avalonia.Threading;
using Studio.ViewModels.Workspace;
using Message.CLR;
using Bridge.CLR;
using GRS.Features.ResourceBounds.UIX.Workspace.Properties.Instrumentation;
using ReactiveUI;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Listeners;
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
                    else
                    {
                        contents = $"Descriptor mismatch detected, shader expected {typeLookup[message.Flat.compileType]} but received {typeLookup[message.Flat.runtimeType]}";
                    }

                    // Create object
                    var validationObject = new ValidationObject()
                    {
                        Content = contents,
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
                        Dispatcher.UIThread.InvokeAsync(() => { value.Count += kv.Value; });
                    }
                }
            }
        }

        /// <summary>
        /// Create an instrumentation property
        /// </summary>
        /// <param name="target"></param>
        /// <returns></returns>
        public IPropertyViewModel? CreateInstrumentationObjectProperty(IPropertyViewModel target)
        {
            // Get feature in parent
            FeatureInfo? featureInfo = (target as IInstrumentableObject)?
                .GetWorkspace()?
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
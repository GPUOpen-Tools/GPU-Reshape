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
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Listeners;
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
            _shaderMappingService = viewModel.PropertyCollection.GetService<IShaderMappingService>();
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
            var lookup = new Dictionary<uint, ResourceIndexOutOfBoundsMessage>();
            var enqueued = new Dictionary<uint, uint>();

            // Consume all messages
            foreach (ResourceIndexOutOfBoundsMessage message in view)
            {
                if (enqueued.TryGetValue(message.Key, out uint enqueuedCount))
                {
                    enqueued[message.Key] = enqueuedCount + 1;
                }
                else
                {
                    lookup.Add(message.Key, message);
                    enqueued.Add(message.Key, 1);
                }
            }

            foreach (var kv in enqueued)
            {
                // Add to reduced set
                if (_reducedMessages.ContainsKey(kv.Key))
                {
                    Dispatcher.UIThread.InvokeAsync(() => { _reducedMessages[kv.Key].Count += kv.Value; });
                }
                else
                {
                    // Get from key
                    var message = lookup[kv.Key];

                    // Create object
                    var validationObject = new ValidationObject()
                    {
                        Content = $"{(message.Flat.isTexture == 1 ? "Texture" : "Buffer")} {(message.Flat.isWrite == 1 ? "write" : "read")} out of bounds",
                        Count = kv.Value
                    };

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
                    _reducedMessages.Add(kv.Key, validationObject);

                    // Add to UI visible collection
                    Dispatcher.UIThread.InvokeAsync(() => { _messageCollectionViewModel?.ValidationObjects.Add(validationObject); });
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
            // Get feature info on target
            FeatureInfo? featureInfo = (target as IInstrumentableObject)?
                .GetWorkspace()?
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
        /// Shader segment mapper
        /// </summary>
        private IShaderMappingService? _shaderMappingService;

        /// <summary>
        /// Validation container
        /// </summary>
        private IMessageCollectionViewModel? _messageCollectionViewModel;
    }
}
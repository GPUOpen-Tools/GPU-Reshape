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

namespace GRS.Features.Descriptor.UIX.Workspace
{
    public class DescriptorService : IInstrumentationPropertyService, Bridge.CLR.IBridgeListener
    {
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
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsStatic(DescriptorMismatchMessage.ID)) 
                return;

            var view = new StaticMessageView<DescriptorMismatchMessage>(streams);

            // Latent update set
            var lookup = new Dictionary<uint, DescriptorMismatchMessage>();
            var enqueued = new Dictionary<uint, uint>();

            // Consume all messages
            foreach (DescriptorMismatchMessage message in view)
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
            
            // Type lookup
            string[] typeLookup = new[] { "Texture", "Buffer", "CBuffer", "Sampler" };

            foreach (var kv in enqueued)
            {
                // Add to reduced set
                if (_reducedMessages.ContainsKey(kv.Key))
                {
                    Dispatcher.UIThread.InvokeAsync(() =>
                    {
                        _reducedMessages[kv.Key].Count += kv.Value;
                    });
                }
                else
                {
                    // Get from key
                    var message = lookup[kv.Key];

                    // Create object
                    var validationObject = new ValidationObject()
                    {
                        Content = $"Descriptor mismatch detected, shader expected {typeLookup[message.Flat.compileType]} but received {typeLookup[message.Flat.runtimeType]}",
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
        /// Segment mapping
        /// </summary>
        private IShaderMappingService? _shaderMappingService;

        /// <summary>
        /// Validation container
        /// </summary>
        private IMessageCollectionViewModel? _messageCollectionViewModel;
    }
}
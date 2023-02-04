using System;
using System.Collections.Generic;
using Avalonia.Threading;
using Studio.ViewModels.Workspace;
using Message.CLR;
using Bridge.CLR;
using ReactiveUI;
using Studio.ViewModels.Workspace.Listeners;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.ResourceBounds.UIX.Workspace
{
    public class ExportStabilityService : IPropertyService, Bridge.CLR.IBridgeListener
    {
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
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsStatic(UnstableExportMessage.ID)) 
                return;

            var view = new StaticMessageView<UnstableExportMessage>(streams);

            // Latent update set
            var lookup = new Dictionary<uint, UnstableExportMessage>();
            var enqueued = new Dictionary<uint, uint>();

            // Consume all messages
            foreach (UnstableExportMessage message in view)
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
                    _reducedMessages[kv.Key].Count += kv.Value;
                }
                else
                {
                    // Get from key
                    var message = lookup[kv.Key];

                    // Create object
                    var validationObject = new ValidationObject()
                    {
                        Content = $"Exporting {(message.Flat.isNaN == 1 ? "NaN" : "Inf")}",
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
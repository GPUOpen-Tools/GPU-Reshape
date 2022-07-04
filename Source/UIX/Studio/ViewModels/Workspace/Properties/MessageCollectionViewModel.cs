using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace.Listeners;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Properties
{
    public class MessageCollectionViewModel : ReactiveObject, IPropertyViewModel, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name { get; } = "Messages";

        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceOverview;

        /// <summary>
        /// Parent property
        /// </summary>
        public IPropertyViewModel? Parent { get; set; }

        /// <summary>
        /// Child properties
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; set; } = new SourceList<IPropertyViewModel>();

        /// <summary>
        /// All condensed messages
        /// </summary>
        public ObservableCollection<Objects.ValidationObject> ValidationObjects { get; } = new();

        /// <summary>
        /// Opens a given shader document from view model
        /// </summary>
        public ICommand OpenShaderDocument;

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

        public MessageCollectionViewModel()
        {
            OpenShaderDocument = ReactiveCommand.Create<object>(OnOpenShaderDocument);
        }

        /// <summary>
        /// On shader document open
        /// </summary>
        /// <param name="viewModel">must be ValidationObject</param>
        private void OnOpenShaderDocument(object viewModel)
        {
            var validationObject = (ValidationObject)viewModel;

            // May not be ready yet
            if (validationObject.Segment == null)
            {
                return;
            }

            // Create document
            Interactions.DocumentInteractions.OpenDocument.OnNext(new Documents.ShaderViewModel()
            {
                Id = $"Shader{validationObject.Segment.Location.SGUID}",
                Title = $"Shader",
                PropertyCollection = Parent,
                GUID = validationObject.Segment.Location.SGUID
            });
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Set connection
            _shaderMappingListener.ConnectionViewModel = ConnectionViewModel;
            
            // Register internal listeners
            _connectionViewModel?.Bridge?.Register(ShaderSourceMappingMessage.ID, _shaderMappingListener);

            // HARDCODED LISTENER, WILL BE MOVED TO MODULAR CODE
            _connectionViewModel?.Bridge?.Register(ResourceIndexOutOfBoundsMessage.ID, this);

            // Create all properties
            CreateProperties();
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();

            if (_connectionViewModel == null)
            {
                return;
            }
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            // HARDCODED READER, WILL BE MOVED TO MODULAR CODE
            if (streams.GetSchema().IsStatic(ResourceIndexOutOfBoundsMessage.ID))
            {
                var view = new StaticMessageView<ResourceIndexOutOfBoundsMessage>(streams);

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
                        _reducedMessages[kv.Key].Count += kv.Value;
                    }
                    else
                    {
                        // Get from key
                        var message = lookup[kv.Key];

                        // Create object
                        var validationObject = new Objects.ValidationObject()
                        {
                            Content = $"{(message.Flat.isTexture == 1 ? "Texture" : "Buffer")} {(message.Flat.isWrite == 1 ? "write" : "read")} out of bounds"
                        };

                        // Shader view model injection
                        validationObject.WhenAnyValue(x => x.Segment).WhereNotNull().Subscribe(x =>
                        {
                            // Try to get shader collection
                            var shaderCollectionViewModel = Parent?.GetProperty<ShaderCollectionViewModel>();
                            if (shaderCollectionViewModel != null)
                            {
                                // Get the respective shader
                                Objects.ShaderViewModel shaderViewModel = shaderCollectionViewModel.GetOrAddShader(x.Location.SGUID);

                                // Append validation object to target shader
                                shaderViewModel.ValidationObjects.Add(validationObject);
                            }
                        });

                        // Enqueue segment binding
                        _shaderMappingListener.EnqueueMessage(validationObject, message.sguid);

                        // Insert lookup
                        _reducedMessages.Add(kv.Key, validationObject);

                        // Add to UI visible collection
                        Dispatcher.UIThread.InvokeAsync(() => { ValidationObjects.Add(validationObject); });
                    }
                }
            }
        }

        /// <summary>
        /// All reduced resource messages
        /// </summary>
        private Dictionary<uint, Objects.ValidationObject> _reducedMessages = new();

        /// <summary>
        /// Shader mapping bridge listener
        /// </summary>
        private ShaderMappingListener _shaderMappingListener = new();

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}
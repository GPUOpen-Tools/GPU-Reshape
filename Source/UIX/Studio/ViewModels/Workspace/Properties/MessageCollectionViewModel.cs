using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.ViewModels.Workspace.Listeners;

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
        /// Child properties
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; set; } = new SourceList<IPropertyViewModel>();

        /// <summary>
        /// All condensed messages
        /// </summary>
        public ObservableCollection<CondensedMessage> CondensedMessages { get; } = new();

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
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
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
                var enqueued = new HashSet<uint>();
                
                // Consume all messages
                foreach (ResourceIndexOutOfBoundsMessage message in view)
                {
                    uint key = message.Key;

                    // Enqueue for update
                    enqueued.Add(key);

                    // Add to reduced set
                    if (_reducedMessages.ContainsKey(key))
                    {
                        // Update the model, not view model, for performance reasons
                        _reducedMessages[key].Model.Count++;
                    }
                    else
                    {
                        // Create message
                        var condensed = new CondensedMessage()
                        {
                            Model = new Models.Workspace.Properties.CondensedMessage()
                            {
                                Count = 1,
                                Extract = _shaderMappingListener.GetSegment(message.sguid)?.Extract ?? "[Lookup Failed]",
                                Content = $"{(message.Flat.isTexture == 1 ? "Texture" : "Buffer")} {(message.Flat.isWrite == 1 ? "write" : "read")} out of bounds"
                            }
                        };
                        
                        // Insert lookup
                        _reducedMessages.Add(key, condensed);

                        // Add to UI visible collection
                        Dispatcher.UIThread.InvokeAsync(() =>
                        {
                            CondensedMessages.Add(condensed);
                        });
                    }
                }
                
                // Update all view models of enqueued keys
                // TODO: Throttle!
                foreach (uint key in enqueued)
                {
                    _reducedMessages[key].RaisePropertyChanged("Count");
                }
            }
        }

        /// <summary>
        /// All reduced resource messages
        /// </summary>
        private Dictionary<uint, CondensedMessage> _reducedMessages = new();

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
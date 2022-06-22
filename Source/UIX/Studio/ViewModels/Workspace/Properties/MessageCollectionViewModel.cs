using System;
using System.Collections.Generic;
using DynamicData;
using Message.CLR;
using ReactiveUI;

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
        public PropertyVisibility Visibility => PropertyVisibility.Default;

        /// <summary>
        /// Child properties
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; set; } = new SourceList<IPropertyViewModel>();
        
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
                
                // Consume all messages
                foreach (ResourceIndexOutOfBoundsMessage message in view)
                {
                    uint key = message.Key;

                    // Add to reduced set
                    if (_reducedMessages.ContainsKey(key))
                    {
                        _reducedMessages[key].Count++;
                    }
                    else
                    {
                        _reducedMessages.Add(key, new BatchedMessage()
                        {
                            Count = 1,
                            Message = message.Flat
                        });
                    }
                }
            }
        }

        // Counted message
        public class BatchedMessage
        {
            // Number of messages
            public uint Count;
            
            // Flat message
            public ResourceIndexOutOfBoundsMessage.FlatInfo Message;
        }

        /// <summary>
        /// All reduced resource messages
        /// </summary>
        private Dictionary<uint, BatchedMessage> _reducedMessages = new();

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}
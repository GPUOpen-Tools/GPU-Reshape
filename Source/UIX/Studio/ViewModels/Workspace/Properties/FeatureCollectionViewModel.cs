using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace.Listeners;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Properties
{
    public class FeatureCollectionViewModel : ReactiveObject, IFeatureCollectionViewModel, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name { get; } = "Features";

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
        /// All services
        /// </summary>
        public ISourceList<IPropertyService> Services { get; set; } = new SourceList<IPropertyService>();

        /// <summary>
        /// All pooled features
        /// </summary>
        public ObservableCollection<FeatureInfo> Features { get; } = new();

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

        public FeatureCollectionViewModel()
        {
            
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsOrdered())
            {
                return;
            }
            
            // Create view
            var view = new OrderedMessageView(streams);

            // Consume all messages
            foreach (OrderedMessage message in view)
            {
                switch (message.ID)
                {
                    case FeatureListMessage.ID:
                    {
                        OnMessage(message.Get<FeatureListMessage>());
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Invoked on feature descriptor list message
        /// </summary>
        /// <param name="message"></param>
        /// <exception cref="NotImplementedException"></exception>
        private void OnMessage(FeatureListMessage message)
        {
            List<FeatureInfo> features = new();

            // Convert descriptors
            foreach (OrderedMessage ordered in new OrderedMessageView(message.descriptors.Stream))
            {
                switch (ordered.ID)
                {
                    case FeatureDescriptorMessage.ID:
                        var descriptor = ordered.Get<FeatureDescriptorMessage>();
                        features.Add(new FeatureInfo()
                        {
                            Name = descriptor.name.String ?? string.Empty,
                            Description = descriptor.description.String ?? string.Empty,
                            FeatureBit = descriptor.featureBit
                        });
                        break;
                }
            }

            // Set on main thread
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                Features.Clear();
                Features.AddRange(features);
            });
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            if (_connectionViewModel == null)
            {
                return;
            }
            
            // Register listeners
            _connectionViewModel.Bridge?.Register(this);

            // Submit request
            var request = _connectionViewModel.GetSharedBus().Add<GetFeaturesMessage>();
            request.featureBitSet = UInt64.MaxValue;
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}
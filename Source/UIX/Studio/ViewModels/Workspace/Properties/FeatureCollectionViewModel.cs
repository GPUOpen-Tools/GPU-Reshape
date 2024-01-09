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
using Studio.ViewModels.Workspace.Services;
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
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceOverview | PropertyVisibility.HiddenByDefault;

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
        public ISourceList<IPropertyService> Services { get; } = new SourceList<IPropertyService>();

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
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Deregister listeners
            _connectionViewModel?.Bridge?.Deregister(this);
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}
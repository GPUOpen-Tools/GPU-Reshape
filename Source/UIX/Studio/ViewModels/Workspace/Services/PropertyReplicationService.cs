// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Query;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace Studio.ViewModels.Workspace.Services
{
    public class PropertyReplicationService : ReactiveObject, IPropertyReplicationService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Target connection
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
        /// Target property
        /// </summary>
        public IPropertyViewModel Property
        {
            set
            {
                _root = value.GetRoot();
                _shaderCollection = _root.GetProperty<IShaderCollectionViewModel>();
                _pipelineCollection = _root.GetProperty<IPipelineCollectionViewModel>();
            }
        }

        /// <summary>
        /// Invoked on connection changes
        /// </summary>
        private void OnConnectionChanged()
        {
            // Register listener
            _connectionViewModel?.Bridge?.Register(this);

            // Initial sync
            Sync();
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
        /// Replicate all properties
        /// </summary>
        public void Sync()
        {
            // Ensure the feature requests are submitted by now
            _connectionViewModel?.Commit();
            
            // Submit state requests
            if (_connectionViewModel?.GetSharedBus() is not { } bus)
            {
                return;
            }

            // Request all relevant states
            RequestState(bus, SetGlobalInstrumentationMessage.ID);
            RequestState(bus, SetShaderInstrumentationMessage.ID);
            RequestState(bus, SetPipelineInstrumentationMessage.ID);
            RequestState(bus, SetOrAddFilteredPipelineInstrumentationMessage.ID);
        }

        /// <summary>
        /// Unique state request
        /// </summary>
        private void RequestState(OrderedMessageView<ReadWriteMessageStream> bus, uint uuid)
        {
            var message = bus.Add<GetStateMessage>();
            message.uuid = uuid;
        }

        /// <summary>
        /// Invoked on message streams
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            foreach (OrderedMessage message in new OrderedMessageView(streams))
            {
                switch (message.ID)
                {
                    case SetGlobalInstrumentationMessage.ID:
                    {
                        // Ensure base is set
                        if (_root == null)
                        {
                            break;
                        }

                        // Defer request with flat message
                        var flat = message.Get<SetGlobalInstrumentationMessage>().Flat;
                        Dispatcher.UIThread.InvokeAsync(() => Handle(flat));
                        break;
                    }
                    case SetShaderInstrumentationMessage.ID:
                    {
                        // Ensure base is set
                        if (_shaderCollection == null)
                        {
                            break;
                        }
                        
                        // Defer request with flat message
                        var flat = message.Get<SetShaderInstrumentationMessage>().Flat;
                        Dispatcher.UIThread.InvokeAsync(() => Handle(flat));
                        break;
                    }
                    case SetPipelineInstrumentationMessage.ID:
                    {
                        // Ensure base is set
                        if (_pipelineCollection == null)
                        {
                            break;
                        }
                        
                        // Defer request with flat message
                        var flat = message.Get<SetPipelineInstrumentationMessage>().Flat;
                        Dispatcher.UIThread.InvokeAsync(() => Handle(flat));
                        break;
                    }
                    case SetOrAddFilteredPipelineInstrumentationMessage.ID:
                    {
                        // Ensure base is set
                        if (_pipelineCollection == null)
                        {
                            break;
                        }
                        
                        // Defer request with flat message
                        var flat = message.Get<SetOrAddFilteredPipelineInstrumentationMessage>().Flat;
                        Dispatcher.UIThread.InvokeAsync(() => Handle(flat));
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Global handler
        /// </summary>
        private void Handle(SetGlobalInstrumentationMessage.FlatInfo msg)
        {
            // Get global instrumentation
            var globalViewModel = _root.GetProperty<GlobalViewModel>();
            if (globalViewModel == null)
            {
                _root.Properties.Add(globalViewModel = new GlobalViewModel()
                {
                    Parent = _root,
                    ConnectionViewModel = ConnectionViewModel
                });
            }

            // Transfer all states to property
            TransferInstrumentationState(globalViewModel, msg.featureBitSet);
        }

        /// <summary>
        /// Shader handler
        /// </summary>
        private void Handle(SetShaderInstrumentationMessage.FlatInfo msg)
        {
            // Find or create property
            var shaderViewModel = _shaderCollection.GetPropertyWhere<ShaderViewModel>(x => x.Shader.GUID == msg.shaderUID);
            if (shaderViewModel == null)
            {
                _shaderCollection.Properties.Add(shaderViewModel = new ShaderViewModel()
                {
                    Parent = _shaderCollection,
                    ConnectionViewModel = _shaderCollection.ConnectionViewModel,
                    Shader = new ShaderIdentifier()
                    {
                        GUID = msg.shaderUID,
                        Descriptor = $"Shader {msg.shaderUID}"
                    }
                });
            }

            // Transfer all states to property
            TransferInstrumentationState(shaderViewModel, msg.featureBitSet);
        }

        /// <summary>
        /// Pipeline handler
        /// </summary>
        private void Handle(SetPipelineInstrumentationMessage.FlatInfo msg)
        {
            // Find or create property
            var pipelineViewModel = _pipelineCollection.GetPropertyWhere<PipelineViewModel>(x => x.Pipeline.GUID == msg.pipelineUID);
            if (pipelineViewModel == null)
            {
                _pipelineCollection.Properties.Add(pipelineViewModel = new PipelineViewModel()
                {
                    Parent = _pipelineCollection,
                    ConnectionViewModel = _pipelineCollection.ConnectionViewModel,
                    Pipeline = new PipelineIdentifier()
                    {
                        GUID = msg.pipelineUID,
                        Descriptor = $"Pipeline {msg.pipelineUID}"
                    }
                });
            }

            // Transfer all states to property
            TransferInstrumentationState(pipelineViewModel, msg.featureBitSet);
        }

        /// <summary>
        /// Pipeline filter handler
        /// </summary>
        private void Handle(SetOrAddFilteredPipelineInstrumentationMessage.FlatInfo msg)
        {
            // Convert type
            var type = (PipelineType)msg.type;
            
            // Standard naming decoration
            string name = $"type:{type} {msg.name}";
            
            // Create filter
            Workspace.Properties.Instrumentation.PipelineFilterViewModel pipelineViewModel = new()
            {
                Parent = _pipelineCollection,
                ConnectionViewModel = _pipelineCollection.ConnectionViewModel,
                Name = name,
                GUID = Guid.Parse(msg.guid),
                Filter = new PipelineFilterQueryViewModel()
                {
                    Type = type,
                    Name = name,
                    Decorator = name
                }
            };

            // Add property
            _pipelineCollection.Properties.Add(pipelineViewModel);

            // Transfer all states to property
            TransferInstrumentationState(pipelineViewModel, msg.featureBitSet);
        }

        /// <summary>
        /// Create feature map for property bit-set population
        /// </summary>
        private void CreateFeatureMap()
        {
            // Must have feature collection
            if (_root?.GetProperty<IFeatureCollectionViewModel>() is not { } featureCollection)
            {
                return;
            }

            // Local named lookup
            Dictionary<string, IInstrumentationPropertyService> services = new();

            // Get all instrumentation services
            foreach (IInstrumentationPropertyService instrumentationPropertyService in _root.GetServices<IInstrumentationPropertyService>())
            {
                services[instrumentationPropertyService.Name] = instrumentationPropertyService;
            }

            // Create feature bit to service lookup
            foreach (FeatureInfo featureInfo in featureCollection.Features)
            {
                _propertyServices[featureInfo.FeatureBit] = services[featureInfo.Name];
            }
        }

        /// <summary>
        /// Transfer all instrumentation states
        /// </summary>
        /// <param name="propertyViewModel">destination property</param>
        /// <param name="featureBitSet">source bit-set</param>
        private void TransferInstrumentationState(IPropertyViewModel propertyViewModel, ulong featureBitSet)
        {
            // Lazy load feature map
            if (_propertyServices.Count == 0)
            {
                CreateFeatureMap();
            }
            
            // TODO: Not the "cleanest", but it'll suffice for now
            for (int i = 0; i < 64; i++)
            {
                // Current mask
                ulong mask = 1ul << i;
                
                // Current bit set?
                if ((featureBitSet & mask) == 0x0)
                {
                    continue;
                }
                
                // Get the service for specified mask
                IPropertyViewModel? featureProperty = _propertyServices.GetValueOrDefault(mask, null)?.CreateInstrumentationObjectProperty(propertyViewModel);
                if (featureProperty == null)
                {
                    Studio.Logging.Error($"Failed state replication on feature bit {i} for {propertyViewModel.Name}");
                    return;
                }
                
                // Succeeded, add as property
                propertyViewModel.Properties.Add(featureProperty);
            }
        }

        private Dictionary<ulong, IInstrumentationPropertyService> _propertyServices = new();

        /// <summary>
        /// Internal connection state
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Workspace root property
        /// </summary>
        private IPropertyViewModel? _root;

        /// <summary>
        /// Shader collection property
        /// </summary>
        private IShaderCollectionViewModel? _shaderCollection;

        /// <summary>
        /// Pipeline collection property
        /// </summary>
        private IPipelineCollectionViewModel? _pipelineCollection;
    }
}
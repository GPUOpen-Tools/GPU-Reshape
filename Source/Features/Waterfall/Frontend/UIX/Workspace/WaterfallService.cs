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
using System.Threading.Tasks;
using Avalonia.Threading;
using GRS.Features.ResourceBounds.UIX.Workspace.Objects;
using Studio.ViewModels.Workspace;
using Message.CLR;
using GRS.Features.ResourceBounds.UIX.Workspace.Properties.Instrumentation;
using ReactiveUI;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Instrumentation;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Waterfall.UIX.Workspace
{
    public class WaterfallService : IInstrumentationPropertyService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Feature name
        /// </summary>
        public string Name => "Waterfall";

        /// <summary>
        /// Feature category
        /// </summary>
        public string Category => "AMD";

        /// <summary>
        /// Feature flags
        /// </summary>
        public InstrumentationFlag Flags => InstrumentationFlag.None;

        /// <summary>
        /// Assigned workspace
        /// </summary>
        public IWorkspaceViewModel ViewModel { get; }

        public WaterfallService(IWorkspaceViewModel viewModel)
        {
            ViewModel = viewModel;

            // Add listener to bridge
            viewModel.Connection?.Bridge?.Register(WaterfallingConditionMessage.ID, this);
            viewModel.Connection?.Bridge?.Register(DivergentResourceIndexingMessage.ID, this);

            // Get properties
            _messageCollectionViewModel = viewModel.PropertyCollection.GetProperty<IMessageCollectionViewModel>();

            // Get services
            _shaderMappingService = viewModel.PropertyCollection.GetService<IShaderMappingService>();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            ViewModel.Connection?.Bridge?.Deregister(WaterfallingConditionMessage.ID, this);
            ViewModel.Connection?.Bridge?.Deregister(DivergentResourceIndexingMessage.ID, this);
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (streams.GetSchema().type != MessageSchemaType.Static)
                return;

            switch (streams.GetSchema().id)
            {
                case WaterfallingConditionMessage.ID:
                    Handle(new StaticMessageView<WaterfallingConditionMessage>(streams));
                    break;
                case DivergentResourceIndexingMessage.ID:
                    Handle(new StaticMessageView<DivergentResourceIndexingMessage>(streams));
                    break;
            }
        }

        /// <summary>
        /// Waterfalling handler
        /// </summary>
        private void Handle(StaticMessageView<WaterfallingConditionMessage> view)
        {
            // Latent update set
            var lookup = new Dictionary<uint, WaterfallingConditionMessage>();
            var enqueued = new Dictionary<uint, uint>();

            // Consume all messages
            foreach (WaterfallingConditionMessage message in view)
            {
                if (enqueued.TryGetValue(message.sguid, out uint enqueuedCount))
                {
                    enqueued[message.sguid] = enqueuedCount + 1;
                }
                else
                {
                    lookup.Add(message.sguid, message);
                    enqueued.Add(message.sguid, 1);
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

                    // Create details
                    var detailViewModel = new ScalarizationDetailViewModel()
                    {
                        VaryingOperandIndex = message.varyingOperandIndex
                    };
                    
                    // Create object
                    var validationObject = new ValidationObject()
                    {
                        Content = $"Addressing requires scalarization",
                        Count = kv.Value,
                        Severity = ValidationSeverity.Warning,
                        DetailViewModel = detailViewModel
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

                            // Populate details
                            detailViewModel.Segment = x;
                            detailViewModel.Object = shaderViewModel;
                            
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
        /// Divergent resource index handler
        /// </summary>
        private void Handle(StaticMessageView<DivergentResourceIndexingMessage> view)
        {
            // Latent update set
            var lookup = new Dictionary<uint, DivergentResourceIndexingMessage>();
            var enqueued = new Dictionary<uint, uint>();

            // Consume all messages
            foreach (DivergentResourceIndexingMessage message in view)
            {
                if (enqueued.TryGetValue(message.sguid, out uint enqueuedCount))
                {
                    enqueued[message.sguid] = enqueuedCount + 1;
                }
                else
                {
                    lookup.Add(message.sguid, message);
                    enqueued.Add(message.sguid, 1);
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
                        Content = $"Divergent resource addressing",
                        Count = kv.Value,
                        Severity = ValidationSeverity.Error,
                        DetailViewModel = _divergentResourceAddressingDetailViewModel
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
        /// Check if a target may be instrumented
        /// </summary>
        public bool IsInstrumentationValidFor(IInstrumentableObject instrumentable)
        {
            return instrumentable
                .GetWorkspaceCollection()?
                .GetProperty<IFeatureCollectionViewModel>()?
                .HasFeature("Waterfall") ?? false;
        }

        /// <summary>
        /// Create an instrumentation property
        /// </summary>
        /// <param name="target"></param>
        /// <param name="replication"></param>
        /// <returns></returns>
        public async Task<IPropertyViewModel?> CreateInstrumentationObjectProperty(IPropertyViewModel target, bool replication)
        {
            // Find feature data
            FeatureInfo? featureInfo = (target as IInstrumentableObject)?
                .GetWorkspaceCollection()?
                .GetProperty<IFeatureCollectionViewModel>()?
                .GetFeature("Waterfall");

            // Invalid or already exists?
            if (featureInfo == null || target.HasProperty<WaterfallPropertyViewModel>())
            {
                return null;
            }

            // Create against target
            return new WaterfallPropertyViewModel()
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

        /// <summary>
        /// Shared divergence detail view model
        /// </summary>
        private DivergentResourceAddressingDetailViewModel _divergentResourceAddressingDetailViewModel = new();
    }
}
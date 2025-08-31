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
using Studio.ViewModels.Workspace;
using Message.CLR;
using GRS.Features.DeviceCommands.UIX.Workspace.Properties.Instrumentation;
using Runtime.Threading;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.IL;
using Studio.Models.Instrumentation;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.DeviceCommands.UIX.Workspace
{
    public class DeviceCommandsService : IInstrumentationPropertyService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Feature name
        /// </summary>
        public string Name => "Device Commands";
        
        /// <summary>
        /// Feature category
        /// </summary>
        public string Category => string.Empty;

        /// <summary>
        /// Feature flags
        /// </summary>
        public InstrumentationFlag Flags => InstrumentationFlag.Standard;
        
        /// <summary>
        /// Parent view model
        /// </summary>
        public IWorkspaceViewModel ViewModel { get; }

        public DeviceCommandsService(IWorkspaceViewModel viewModel)
        {
            ViewModel = viewModel;

            // Add listener to bridge
            viewModel.Connection?.Bridge?.Register(DeviceCommandInvalidArgumentMessage.ID, this);

            // Get properties
            _messageCollectionViewModel = viewModel.PropertyCollection.GetProperty<IMessageCollectionViewModel>();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            ViewModel.Connection?.Bridge?.Deregister(DeviceCommandInvalidArgumentMessage.ID, this);
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsChunked(DeviceCommandInvalidArgumentMessage.ID))
                return;

            var view = new ChunkedMessageView<DeviceCommandInvalidArgumentMessage>(streams);
            
            // Latent update set
            var enqueued = new Dictionary<uint, uint>();
            
            // Preallocate initial latents
            foreach (var kv in _reducedMessages)
            {
                enqueued.Add(kv.Key, 0);
            }

            foreach (DeviceCommandInvalidArgumentMessage message in view)
            {
                // Add to latent set
                if (enqueued.TryGetValue(message.Key, out uint enqueuedCount))
                {
                    enqueued[message.Key] = enqueuedCount + 1u;
                }
                else
                {
                    // Create object
                    var validationObject = new ValidationObject()
                    {
                        Traits = _traits,
                        Severity = SourceObjectSeverity.Error,
                        Content = "Device command generated invalid payload",
                        Count = 1u
                    };
                    
                    // Register with latent
                    enqueued.Add(message.Key, 1u);

                    // Insert lookup
                    _reducedMessages.Add(message.Key, validationObject);

                    // Add to UI visible collection
                    Dispatcher.UIThread.InvokeAsync(() => { _messageCollectionViewModel?.ValidationObjects.Add(validationObject); });
                }

                // Detailed?
                if (message.HasChunk(DeviceCommandInvalidArgumentMessage.Chunk.Detail))
                {
                    DeviceCommandInvalidArgumentMessage.DetailChunk detailChunk = message.GetDetailChunk();
                }
            }
            
            // Update counts on main thread
            foreach (var kv in enqueued)
            {
                if (_reducedMessages.TryGetValue(kv.Key, out ValidationObject? value))
                {
                    if (kv.Value > 0)
                    {
                        ValidationMergePumpBus.Increment(value, kv.Value);
                    }
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
                .HasFeature("Device Commands") ?? false;
        }

        /// <summary>
        /// Create an instrumentation property
        /// </summary>
        /// <param name="target"></param>
        /// <param name="replication"></param>
        /// <returns></returns>
        public async Task<IPropertyViewModel?> CreateInstrumentationObjectProperty(IPropertyViewModel target, bool replication)
        {
            // Get feature info on target
            FeatureInfo? featureInfo = (target as IInstrumentableObject)?
                .GetWorkspaceCollection()?
                .GetProperty<IFeatureCollectionViewModel>()?
                .GetFeature("Device Commands");

            // Invalid or already exists?
            if (featureInfo == null || target.HasProperty<DeviceCommandsPropertyViewModel>())
            {
                return null;
            }

            // Create the property
            return new DeviceCommandsPropertyViewModel()
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
        /// Shared validation traits
        /// </summary>
        private ValidationInstructionExcludeTrait _traits = new ()
        {
            ExcludedOps = new OpCode[]
            {
                
            }
        };

        /// <summary>
        /// Validation container
        /// </summary>
        private IMessageCollectionViewModel? _messageCollectionViewModel;
    }
}
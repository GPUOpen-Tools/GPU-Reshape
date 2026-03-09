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
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using Avalonia.Threading;
using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.Settings;
using GRS.Features.Debug.UIX.ViewModels;
using Studio.ViewModels.Workspace;
using Message.CLR;
using Runtime.ViewModels.Workspace.Properties;
using Studio;
using Studio.Models.IL;
using Studio.Models.IL.Tiny;
using Studio.Models.Instrumentation;
using Studio.Services;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;
using ArrayType = Studio.Models.IL.ArrayType;
using MatrixType = Studio.Models.IL.MatrixType;
using StructType = Studio.Models.IL.StructType;
using VectorType = Studio.Models.IL.VectorType;
using Type = Studio.Models.IL.Type;

namespace GRS.Features.Debug.UIX.Workspace
{
    public class DebugService : IInstrumentationPropertyService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Feature name
        /// </summary>
        public string Name => "Debug";
        
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

        public DebugService(IWorkspaceViewModel viewModel)
        {
            ViewModel = viewModel;
            
            // Add listener to bridge
            viewModel.Connection?.Bridge?.Register(DebugWatchpointMetadataMessage.ID, this);
            viewModel.Connection?.Bridge?.Register(DebugWatchpointStreamMessage.ID, this);

            // Get the watchpoint registry for the workspace
            _watchpointRegistryService = ViewModel.PropertyCollection.GetService<WatchpointRegistryService>();

            // Get the settings
            _debugSettingViewModel = ServiceRegistry.Get<ISettingsService>()?.Get<DebugSettingViewModel>();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            ViewModel.Connection?.Bridge?.Deregister(DebugWatchpointStreamMessage.ID, this);
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            switch (streams.Schema.id)
            {
                case DebugWatchpointStreamMessage.ID:
                    HandleStream(new DynamicMessageView<DebugWatchpointStreamMessage>(streams));
                    break;
                case DebugWatchpointMetadataMessage.ID:
                    HandleMetadata(new DynamicMessageView<DebugWatchpointMetadataMessage>(streams));
                    break;
            }
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        private void HandleStream(DynamicMessageView<DebugWatchpointStreamMessage> view)
        {
            foreach (DebugWatchpointStreamMessage message in view)
            {
                // Get the watchpoint
                if (_watchpointRegistryService?.GetWatchpoint(message.uid) is not {} watchpointViewModel)
                {
                    continue;
                }

                // Skip if paused
                if (watchpointViewModel.Paused)
                {
                    continue;
                }
                
                // Do we have a processor?
                if (watchpointViewModel.GetOrCreateProcessor(message, out IDisposable? processorCommit) is not { } processorViewModel)
                {
                    // Always commit if needed
                    Dispatcher.UIThread.InvokeAsync(() => processorCommit?.Dispose());
                    continue;
                }
                
                // May have changed capture mode
                if ((WatchpointCaptureMode)message.captureMode != watchpointViewModel.CaptureMode)
                {
                    continue;
                }

                // Update tiny type if needed
                if (message.dataTinyType.Count != 0)
                {
                    UpdateTinyType(watchpointViewModel, message);
                }

                // Process it on the messaging thread, let the heavy weight stuff leave the UI thread be
                object? payload = processorViewModel.Process(watchpointViewModel, message);
                
                // Total number of streamed data
                uint byteCount = (uint)message.data.Count;
                
                // Flatten the data for UI thread
                DebugWatchpointStreamMessage.FlatInfo flat = message.Flat;

                // Update all watchpoint stats
                IDisposable statsCommit = UpdateStats(watchpointViewModel, message);
                
                // The rest needs to happen on the UI thread
                Dispatcher.UIThread.InvokeAsync(() =>
                {
                    // If there's a commit, do it now
                    processorCommit?.Dispose();
                    statsCommit.Dispose();
                    
                    // Any processed payloads?
                    if (payload != null)
                    {
                        // Finally, install the payload
                        if (watchpointViewModel.DisplayViewModel != null)
                        {
                            processorViewModel.Install(watchpointViewModel.DisplayViewModel, payload);
                        }
                    }

                    // Internal stream handling
                    ProcessStreamRequest(watchpointViewModel, flat, byteCount);
                });
            }
        }

        /// <summary>
        /// Recreate the value structure
        /// </summary>
        private void CreateValueStructure(Type type, UInt64 variableHash, WatchpointDebugValue value, IEnumerator<DebugWatchpointValueMetadataMessage> enumerator)
        {
            enumerator.MoveNext();
            var valueMessage = enumerator.Current;
            
            // Set info
            value.Type = type;
            value.Name = valueMessage.name.String;
            value.VariableHash = variableHash;
            value.ValueId = valueMessage.valueId;
            value.HasReconstruction = valueMessage.hasReconstruction == 1;
            
            // Handle structure
            switch (type.Kind)
            {
                case TypeKind.Struct:
                {
                    var typed = (StructType)type;
                    value.Values = new WatchpointDebugValue[typed.MemberTypes.Length];
                    
                    for (int i = 0; i < typed.MemberTypes.Length; i++)
                    {
                        CreateValueStructure(typed.MemberTypes[i], variableHash, value.Values[i] = new WatchpointDebugValue(), enumerator);
                    }
                    break;
                }
                case TypeKind.Array:
                {
                    var typed = (ArrayType)type;
                    value.Values = new WatchpointDebugValue[typed.Count];
                    
                    for (int i = 0; i < typed.Count; i++)
                    {
                        CreateValueStructure(typed.ElementType, variableHash, value.Values[i] = new WatchpointDebugValue(), enumerator);
                    }
                    break;
                }
                case TypeKind.Vector:
                {
                    var typed = (VectorType)type;
                    value.Values = new WatchpointDebugValue[typed.Dimension];
                    
                    for (int i = 0; i < typed.Dimension; i++)
                    {
                        CreateValueStructure(typed.ContainedType, variableHash, value.Values[i] = new WatchpointDebugValue(), enumerator);
                    }
                    break;
                }
                case TypeKind.Matrix:
                {
                    var typed = (MatrixType)type;
                    value.Values = new WatchpointDebugValue[typed.Columns * typed.Rows];
                    
                    for (int i = 0; i < typed.Columns * typed.Rows; i++)
                    {
                        CreateValueStructure(typed.ContainedType, variableHash, value.Values[i] = new WatchpointDebugValue(), enumerator);
                    }
                    break;
                }
            }
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        private void HandleMetadata(DynamicMessageView<DebugWatchpointMetadataMessage> view)
        {
            foreach (DebugWatchpointMetadataMessage md in view)
            {
                // Get the watchpoint
                if (_watchpointRegistryService?.GetWatchpoint(md.uid) is not {} watchpointViewModel)
                {
                    continue;
                }

                List<WatchpointDebugVariable> remoteVariables = new();

                // Parse all variables
                foreach (DebugWatchpointVariableMetadataMessage variable in
                         new DynamicMessageView<DebugWatchpointVariableMetadataMessage>(md.variables.Stream))
                {
                    // Unpack the type
                    Type type = TinyTypePacking.UnpackTinyType(
                        variable.dataTinyType,
                        new TinyTypePacking.TinyTypeResolver()
                    );

                    // Create variable
                    var debugVariable = new WatchpointDebugVariable()
                    {
                        Name = variable.name.String,
                        Type = type,
                        VariableHash = variable.variableHash
                    };

                    // Parse all variables
                    CreateValueStructure(
                        type,
                        debugVariable.VariableHash,
                        debugVariable.Value,
                        new DynamicMessageView<DebugWatchpointValueMetadataMessage>(variable.values.Stream).GetEnumerator()
                    );
                    
                    remoteVariables.Add(debugVariable);
                }
                
                // The rest needs to happen on the UI thread
                Dispatcher.UIThread.InvokeAsync(() =>
                {
                    // TODO: This is not correct, it's a multi-subscriber situation, again
                    foreach (WatchpointDebugVariable variable in remoteVariables)
                    {
                        if (watchpointViewModel.DebugVariables.All(x => x.VariableHash != variable.VariableHash))
                        {
                            watchpointViewModel.DebugVariables.Add(variable);
                        }
                    }
                    
                    // Default select first
                    if (watchpointViewModel.SelectedDebugValue == null && watchpointViewModel.DebugVariables.Count > 0)
                    {
                        watchpointViewModel.SelectedDebugValue = watchpointViewModel.DebugVariables[0].Value;
                    }
                });
            }
        }

        /// <summary>
        /// Update an underlying tiny type
        /// </summary>
        private void UpdateTinyType(WatchpointViewModel watchpointViewModel, DebugWatchpointStreamMessage message)
        {
            // Check if we need to parse it again
            if (watchpointViewModel.TinyType is { ID: var id } && id == message.dataTypeId)
            {
                return;
            }
            
            // Unpack it
            watchpointViewModel.TinyType = TinyTypePacking.UnpackTinyType(
                message.dataTinyType,
                new TinyTypePacking.TinyTypeResolver()
            );

            // Switch over from tiny type id to real one
            watchpointViewModel.TinyType.ID = message.dataTypeId;
        }

        /// <summary>
        /// Update all watchpoint statistics
        /// </summary>
        private IDisposable UpdateStats(WatchpointViewModel watchpointViewModel, DebugWatchpointStreamMessage message)
        {
            // Calculate average frametime
            long  now = Stopwatch.GetTimestamp();
            long  delta = now - watchpointViewModel.ProcessThreadLastTimeStamp;
            float seconds = delta / (float)Stopwatch.Frequency;
            float frameRate = 1.0f / seconds;
            float weight = 0.95f;
            
            // Update the thread specific stamp
            watchpointViewModel.ProcessThreadLastTimeStamp = now;

            // Get the optional status message
            string statusMessage = GetStatusMessage(message);
            
            // Update timing
            return new ActionDisposable(() =>
            {
                watchpointViewModel.StatusMessage = statusMessage;
                watchpointViewModel.HasStatusMessage = !string.IsNullOrEmpty(statusMessage);
                watchpointViewModel.FrameRate = weight * watchpointViewModel.FrameRate + (1.0f - weight) * frameRate;
            });
        }
        
        /// <summary>
        /// Check watchpoint status
        /// </summary>
        private string GetStatusMessage(DebugWatchpointStreamMessage message)
        {
            // Get the requested byte size
            uint dynamicRequestedByteSize = 0;
            switch ((WatchpointDataOrder)message.dataOrder)
            {
                case WatchpointDataOrder.None:
                case WatchpointDataOrder.Static:
                    break;
                case WatchpointDataOrder.Dynamic:
                    dynamicRequestedByteSize = message.dataDynamicCounter * sizeof(int) * (DynamicWatchpointHeader.DWordCount + message.dataDWordStride);
                    break;
                case WatchpointDataOrder.Loose:
                    dynamicRequestedByteSize = message.dataDynamicCounter * sizeof(int) * (LooseWatchpointHeader.DWordCount + message.dataDWordStride);
                    break;
                default:
                    Logging.Error("Failed to decode watchpoint stream");
                    break;
            }

            // Out of memory?
            if (dynamicRequestedByteSize > message.data.Count)
            {
                return $"Out of memory, requested {(int)(dynamicRequestedByteSize / 1e6)}mb, max {_debugSettingViewModel?.MaxWatchpointMemoryMb ?? 32}mb";
            }

            // Nothing of importance
            return string.Empty;
        }

        /// <summary>
        /// Invoked on stream requests
        /// </summary>
        private void ProcessStreamRequest(WatchpointViewModel watchpointViewModel, DebugWatchpointStreamMessage.FlatInfo flat, uint byteCount)
        {
            uint request = flat.request;

            // Notice the streamer that this request was handled
            if (ViewModel.Connection?.GetSharedBus() is { } sharedBus)
            {
                var limit = sharedBus.Add<DebugWatchpointStreamHandledMessage>();
                limit.request = request;
            }

            // Did we export more than we streamed?
            // If so, try to grow the backing memory
            if (byteCount < flat.dataRequestStreamSize)
            {
                ReallocateWatchpoint(watchpointViewModel, flat);
            }
        }

        /// <summary>
        /// Grow the backing memory of a watchpoint
        /// </summary>
        private void ReallocateWatchpoint(WatchpointViewModel watchpointViewModel, DebugWatchpointStreamMessage.FlatInfo flat)
        {
            // Determine the new size
            uint limit             = (_debugSettingViewModel?.MaxWatchpointMemoryMb ?? 32) * 1000000;
            uint optimalStreamSize = Math.Min((uint)(flat.dataRequestStreamSize * 1.1), limit);

            // May be capped by limits
            if (watchpointViewModel.StreamSize == optimalStreamSize)
            {
                return;
            }
            
            // Let the backend reallocate it
            watchpointViewModel.StreamSize = optimalStreamSize;
            _watchpointRegistryService?.Reallocate(watchpointViewModel);
        }

        /// <summary>
        /// Check if a target may be instrumented
        /// </summary>
        public bool IsInstrumentationValidFor(IInstrumentableObject instrumentable)
        {
            return instrumentable
                .GetWorkspaceCollection()?
                .GetProperty<IFeatureCollectionViewModel>()?
                .HasFeature("Debug") ?? false;
        }

        /// <summary>
        /// Create an instrumentation property
        /// </summary>
        public async Task<IPropertyViewModel?> CreateInstrumentationObjectProperty(IPropertyViewModel target, bool replication)
        {
            // Debugging doesn't have any "implicit" properties
            return null;
        }

        /// <summary>
        /// Internal registry
        /// </summary>
        private readonly WatchpointRegistryService? _watchpointRegistryService;

        /// <summary>
        /// Internal settings
        /// </summary>
        private readonly DebugSettingViewModel? _debugSettingViewModel;
    }
}
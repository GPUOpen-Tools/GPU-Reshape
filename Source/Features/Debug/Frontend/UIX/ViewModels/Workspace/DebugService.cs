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
using Studio.Models.IL.Tiny;
using Studio.Models.Instrumentation;
using Studio.Services;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;
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
            viewModel.Connection?.Bridge?.Register(DebugBreakpointMetadataMessage.ID, this);
            viewModel.Connection?.Bridge?.Register(DebugBreakpointStreamMessage.ID, this);

            // Get the breakpoint registry for the workspace
            _breakpointRegistryService = ViewModel.PropertyCollection.GetService<BreakpointRegistryService>();

            // Get the settings
            _debugSettingViewModel = ServiceRegistry.Get<ISettingsService>()?.Get<DebugSettingViewModel>();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            ViewModel.Connection?.Bridge?.Deregister(DebugBreakpointStreamMessage.ID, this);
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            switch (streams.Schema.id)
            {
                case DebugBreakpointStreamMessage.ID:
                    HandleStream(new DynamicMessageView<DebugBreakpointStreamMessage>(streams));
                    break;
                case DebugBreakpointMetadataMessage.ID:
                    HandleMetadata(new DynamicMessageView<DebugBreakpointMetadataMessage>(streams));
                    break;
            }
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        private void HandleStream(DynamicMessageView<DebugBreakpointStreamMessage> view)
        {
            foreach (DebugBreakpointStreamMessage message in view)
            {
                // Get the breakpoint
                if (_breakpointRegistryService?.GetBreakpoint(message.uid) is not {} breakpointViewModel)
                {
                    continue;
                }

                // Skip if paused
                if (breakpointViewModel.Paused)
                {
                    continue;
                }
                
                // Do we have a processor?
                if (breakpointViewModel.GetOrCreateProcessor(message, out IDisposable? processorCommit) is not { } processorViewModel)
                {
                    // Always commit if needed
                    Dispatcher.UIThread.InvokeAsync(() => processorCommit?.Dispose());
                    continue;
                }
                
                // May have changed capture mode
                if ((BreakpointCaptureMode)message.captureMode != breakpointViewModel.CaptureMode)
                {
                    continue;
                }

                // Update tiny type if needed
                if (message.dataTinyType.Count != 0)
                {
                    UpdateTinyType(breakpointViewModel, message);
                }

                // Process it on the messaging thread, let the heavy weight stuff leave the UI thread be
                object? payload = processorViewModel.Process(breakpointViewModel, message);
                
                // Total number of streamed data
                uint byteCount = (uint)message.data.Count;
                
                // Flatten the data for UI thread
                DebugBreakpointStreamMessage.FlatInfo flat = message.Flat;

                // Update all breakpoint stats
                IDisposable statsCommit = UpdateStats(breakpointViewModel, message);
                
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
                        if (breakpointViewModel.DisplayViewModel != null)
                        {
                            processorViewModel.Install(breakpointViewModel.DisplayViewModel, payload);
                        }
                    }

                    // Internal stream handling
                    ProcessStreamRequest(breakpointViewModel, flat, byteCount);
                });
            }
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        private void HandleMetadata(DynamicMessageView<DebugBreakpointMetadataMessage> view)
        {
            foreach (DebugBreakpointMetadataMessage md in view)
            {
                // Get the breakpoint
                if (_breakpointRegistryService?.GetBreakpoint(md.uid) is not {} breakpointViewModel)
                {
                    continue;
                }

                List<BreakpointDebugVariable> remoteVariables = new();

                // Parse all variables
                foreach (DebugBreakpointVariableMetadataMessage variable in
                         new DynamicMessageView<DebugBreakpointVariableMetadataMessage>(md.variables.Stream))
                {
                    // Unpack the type
                    Type type = TinyTypePacking.UnpackTinyType(
                        variable.dataTinyType,
                        new TinyTypePacking.TinyTypeResolver()
                    );
                    
                    remoteVariables.Add(new BreakpointDebugVariable()
                    {
                        Name = variable.name.String,
                        Type = type,
                        Handle = variable.handle
                    });
                }
                
                // The rest needs to happen on the UI thread
                Dispatcher.UIThread.InvokeAsync(() =>
                {
                    // TODO: This is not correct, it's a multi-subscriber situation, again
                    foreach (BreakpointDebugVariable variable in remoteVariables)
                    {
                        if (!breakpointViewModel.DebugVariables.Any(x => x.Handle == variable.Handle))
                        {
                            breakpointViewModel.DebugVariables.Add(variable);
                        }
                    }
                    
                    // Default select first
                    if (breakpointViewModel.SelectedDebugVariable == null && breakpointViewModel.DebugVariables.Count > 0)
                    {
                        breakpointViewModel.SelectedDebugVariable = breakpointViewModel.DebugVariables[0];
                    }
                });
            }
        }

        /// <summary>
        /// Update an underlying tiny type
        /// </summary>
        private void UpdateTinyType(BreakpointViewModel breakpointViewModel, DebugBreakpointStreamMessage message)
        {
            // Check if we need to parse it again
            if (breakpointViewModel.TinyType is { ID: var id } && id == message.dataTypeId)
            {
                return;
            }
            
            // Unpack it
            breakpointViewModel.TinyType = TinyTypePacking.UnpackTinyType(
                message.dataTinyType,
                new TinyTypePacking.TinyTypeResolver()
            );

            // Switch over from tiny type id to real one
            breakpointViewModel.TinyType.ID = message.dataTypeId;
        }

        /// <summary>
        /// Update all breakpoint statistics
        /// </summary>
        private IDisposable UpdateStats(BreakpointViewModel breakpointViewModel, DebugBreakpointStreamMessage message)
        {
            // Calculate average frametime
            long  now = Stopwatch.GetTimestamp();
            long  delta = now - breakpointViewModel.ProcessThreadLastTimeStamp;
            float seconds = delta / (float)Stopwatch.Frequency;
            float frameRate = 1.0f / seconds;
            float weight = 0.95f;
            
            // Update the thread specific stamp
            breakpointViewModel.ProcessThreadLastTimeStamp = now;

            // Get the optional status message
            string statusMessage = GetStatusMessage(message);
            
            // Update timing
            return new ActionDisposable(() =>
            {
                breakpointViewModel.StatusMessage = statusMessage;
                breakpointViewModel.HasStatusMessage = !string.IsNullOrEmpty(statusMessage);
                breakpointViewModel.FrameRate = weight * breakpointViewModel.FrameRate + (1.0f - weight) * frameRate;
            });
        }
        
        /// <summary>
        /// Check breakpoint status
        /// </summary>
        private string GetStatusMessage(DebugBreakpointStreamMessage message)
        {
            // Get the requested byte size
            uint dynamicRequestedByteSize = 0;
            switch ((BreakpointDataOrder)message.dataOrder)
            {
                case BreakpointDataOrder.None:
                case BreakpointDataOrder.Static:
                    break;
                case BreakpointDataOrder.Dynamic:
                    dynamicRequestedByteSize = message.dataDynamicCounter * sizeof(int) * (DynamicBreakpointHeader.DWordCount + message.dataDWordStride);
                    break;
                case BreakpointDataOrder.Loose:
                    dynamicRequestedByteSize = message.dataDynamicCounter * sizeof(int) * (LooseBreakpointHeader.DWordCount + message.dataDWordStride);
                    break;
                default:
                    Logging.Error("Failed to decode breakpoint stream");
                    break;
            }

            // Out of memory?
            if (dynamicRequestedByteSize > message.data.Count)
            {
                return $"Out of memory, requested {(int)(dynamicRequestedByteSize / 1e6)}mb, max {_debugSettingViewModel?.MaxBreakpointMemoryMb ?? 32}mb";
            }

            // Nothing of importance
            return string.Empty;
        }

        /// <summary>
        /// Invoked on stream requests
        /// </summary>
        private void ProcessStreamRequest(BreakpointViewModel breakpointViewModel, DebugBreakpointStreamMessage.FlatInfo flat, uint byteCount)
        {
            uint request = flat.request;

            // Notice the streamer that this request was handled
            if (ViewModel.Connection?.GetSharedBus() is { } sharedBus)
            {
                var limit = sharedBus.Add<DebugBreakpointStreamHandledMessage>();
                limit.request = request;
            }

            // Did we export more than we streamed?
            // If so, try to grow the backing memory
            if (byteCount < flat.dataRequestStreamSize)
            {
                ReallocateBreakpoint(breakpointViewModel, flat);
            }
        }

        /// <summary>
        /// Grow the backing memory of a breakpoint
        /// </summary>
        private void ReallocateBreakpoint(BreakpointViewModel breakpointViewModel, DebugBreakpointStreamMessage.FlatInfo flat)
        {
            // Determine the new size
            uint limit             = (_debugSettingViewModel?.MaxBreakpointMemoryMb ?? 32) * 1000000;
            uint optimalStreamSize = Math.Min((uint)(flat.dataRequestStreamSize * 1.1), limit);

            // May be capped by limits
            if (breakpointViewModel.StreamSize == optimalStreamSize)
            {
                return;
            }
            
            // Let the backend reallocate it
            breakpointViewModel.StreamSize = optimalStreamSize;
            _breakpointRegistryService?.Reallocate(breakpointViewModel);
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
        private readonly BreakpointRegistryService? _breakpointRegistryService;

        /// <summary>
        /// Internal settings
        /// </summary>
        private readonly DebugSettingViewModel? _debugSettingViewModel;
    }
}
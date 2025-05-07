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
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Threading;
using GRS.Features.Debug.UIX.Settings;
using GRS.Features.Debug.UIX.ViewModels;
using Studio.ViewModels.Workspace;
using Message.CLR;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Instrumentation;
using Studio.Services;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;
using UIX.Views;

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
            var view = new DynamicMessageView<DebugBreakpointStreamMessage>(streams);
            
            foreach (DebugBreakpointStreamMessage message in view)
            {
                // TODO[dbg]: Temporary code for selecting the display mode
                BreakpointDisplayMode mode;
                if ((BreakpointDataOrder)message.dataOrder == BreakpointDataOrder.Static &&
                    (BreakpointCompression)message.dataCompression == BreakpointCompression.FPUNorm8888)
                {
                    mode = BreakpointDisplayMode.Image;
                }
                else
                {
                    mode = BreakpointDisplayMode.Structural;
                }
                
                // Deserialize on the message thread
                object? payload = null;
                switch (mode)
                {
                    case BreakpointDisplayMode.Image:
                        payload = DeserializeImagePayload(message, mode);
                        break;
                    case BreakpointDisplayMode.Structural:
                        break;
                }

                // Total number of streamed data
                uint byteCount = (uint)message.data.Count;
                
                // Flatten the data for UI thread
                DebugBreakpointStreamMessage.FlatInfo flat = message.Flat;
                
                Dispatcher.UIThread.InvokeAsync(() =>
                {
                    ProcessStreamRequest(flat, mode, payload, byteCount);
                });
            }
        }

        /// <summary>
        /// Invoked on stream requests
        /// </summary>
        private void ProcessStreamRequest(DebugBreakpointStreamMessage.FlatInfo flat, BreakpointDisplayMode mode, object? payload, uint byteCount)
        {
            uint request = flat.request;

            // Get the breakpoint
            if (!(_breakpointRegistryService?.Lookup.TryGetValue(flat.uid, out BreakpointViewModel? breakpointViewModel) ?? false))
            {
                return;
            }

            // Install the payload
            switch (mode)
            {
                case BreakpointDisplayMode.Image:
                    InstallImagePayload(breakpointViewModel, (Bitmap)payload!);
                    break;
                case BreakpointDisplayMode.Structural:
                    break;
            }

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
        /// Deserialize an incoming image payload
        /// </summary>
        private unsafe object DeserializeImagePayload(DebugBreakpointStreamMessage message, BreakpointDisplayMode mode)
        {
            switch (mode)
            {
                default:
                {
                    throw new InvalidOperationException();
                }
                case BreakpointDisplayMode.Image:
                {
                    return new WriteableBitmap(
                        PixelFormat.Rgba8888, AlphaFormat.Opaque,
                        new IntPtr(message.data.GetDataStart()), new PixelSize((int)message.dataStaticWidth, (int)message.dataStaticHeight),
                        new Vector(96, 96), (int)(message.dataStaticWidth * 4)
                    );
                }
                case BreakpointDisplayMode.Structural:
                {
                    using var stream = new UnmanagedMemoryStream(message.data.GetDataStart(), message.data.Count);
                    return new Bitmap(stream);
                }
            }
        }

        /// <summary>
        /// Install an image playload on the UI thread
        /// </summary>
        private void InstallImagePayload(BreakpointViewModel breakpointViewModel, Bitmap payload)
        {
            // Assign view model, change if needed
            if (breakpointViewModel.DisplayViewModel is not ImageBreakpointDisplayViewModel displayViewModel)
            {
                breakpointViewModel.DisplayViewModel = displayViewModel = new ImageBreakpointDisplayViewModel();
                
                // TODO[dbg]: Temporary window for feature development
                BreakpointDisplayView displayView = new();
                displayView.DataContext = displayViewModel;
                displayView.Width = 1920;
                displayView.Height = 1080;
                displayView.Show();
            }

            // Assign new image
            displayViewModel.Image = payload;

            // Calculate average frametime
            long  now = Stopwatch.GetTimestamp();
            long  delta = now - displayViewModel.LastTimeStamp;
            float seconds = delta / (float)Stopwatch.Frequency;
            float frameRate = 1.0f / seconds;
            float weight = 0.95f;
            
            // Update timing
            displayViewModel.FrameRate = weight * displayViewModel.FrameRate + (1.0f - weight) * frameRate;
            displayViewModel.LastTimeStamp = now;
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
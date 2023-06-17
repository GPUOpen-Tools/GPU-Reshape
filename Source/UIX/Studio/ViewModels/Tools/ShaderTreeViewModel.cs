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
using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Avalonia.Threading;
using Bridge.CLR;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using DynamicData.Binding;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Runtime.ViewModels.Objects;
using Runtime.ViewModels.Tools;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Tools
{
    public class ShaderTreeViewModel : ToolViewModel, IBridgeListener
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public override StreamGeometry? Icon => ResourceLocator.GetIcon("ToolShaderTree");
        
        /// <summary>
        /// All identifiers
        /// </summary>
        public ObservableCollection<ShaderIdentifierViewModel> ShaderIdentifiers { get; } = new();

        /// <summary>
        /// Is the help message visible?
        /// </summary>
        public bool IsHelpVisible
        {
            get => ShaderIdentifiers.Count == 0;
        }

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IWorkspaceViewModel? WorkspaceViewModel
        {
            get => _workspaceViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _workspaceViewModel, value);

                OnConnectionChanged();
            }
        }

        /// <summary>
        /// Refresh all items
        /// </summary>
        public ICommand Refresh { get; }
        
        /// <summary>
        /// Opens a given shader document from view model
        /// </summary>
        public ICommand OpenShaderDocument;

        public ShaderTreeViewModel()
        {
            Refresh = ReactiveCommand.Create(OnRefresh);
            OpenShaderDocument = ReactiveCommand.Create<ShaderIdentifierViewModel>(OnOpenShaderDocument);
            
            // Bind visibility
            ShaderIdentifiers.ToObservableChangeSet(x => x)
                .OnItemAdded(_ => this.RaisePropertyChanged(nameof(IsHelpVisible)))
                .Subscribe();
            
            // Bind selected workspace
            App.Locator.GetService<IWorkspaceService>()?
                .WhenAnyValue(x => x.SelectedWorkspace)
                .Subscribe(x => WorkspaceViewModel = x);
        }

        /// <summary>
        /// Invoked on document requests
        /// </summary>
        /// <param name="shaderIdentifierViewModel"></param>
        private void OnOpenShaderDocument(ShaderIdentifierViewModel shaderIdentifierViewModel)
        {
            if (_workspaceViewModel == null)
                return;
            
            if (App.Locator.GetService<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.OpenDocument?.Execute( new ShaderDescriptor()
                {
                    PropertyCollection = _workspaceViewModel?.PropertyCollection,
                    GUID = shaderIdentifierViewModel.GUID
                });
            }
        }

        /// <summary>
        /// Populate a given range
        /// </summary>
        /// <param name="start">starting index</param>
        /// <param name="end">end index</param>
        public void PopulateRange(int start, int end)
        {
            if (_workspaceViewModel is not { Connection: { } })
                return;
            
            // Submit range request
            var range = _workspaceViewModel.Connection.GetSharedBus().Add<GetShaderUIDRangeMessage>();
            range.start = (uint)start;
            range.limit = (uint)(end - start);
        }

        /// <summary>
        /// Populate a single identifier
        /// </summary>
        /// <param name="shaderIdentifierViewModel"></param>
        public void PopulateSingle(ShaderIdentifierViewModel shaderIdentifierViewModel)
        {
            if (_workspaceViewModel is not { Connection: { } })
                return;
            
            // Submit single request
            var range = _workspaceViewModel.Connection.GetSharedBus().Add<GetShaderUIDRangeMessage>();
            range.start = (uint)ShaderIdentifiers.IndexOf(shaderIdentifierViewModel);
            range.limit = 1;
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Clear states
            ShaderIdentifiers.Clear();
            _lookup.Clear();
            
            if (_workspaceViewModel is not { Connection: { } })
                return;
            
            // Subscribe
            _workspaceViewModel.Connection.Bridge?.Register(this);

            // Start pooling timer
            _poolTimer = new DispatcherTimer(TimeSpan.FromSeconds(1), DispatcherPriority.Background, OnPoolEvent);
            _poolTimer.Start();
        }

        /// <summary>
        /// Invoked on timed pooling
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnPoolEvent(object? sender, EventArgs e)
        {
            // Submit request for all objects
            _workspaceViewModel?.Connection?.GetSharedBus().Add<GetObjectStatesMessage>();
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsOrdered())
            {
                return;
            }

            // Enumerate all messages
            foreach (OrderedMessage message in new OrderedMessageView(streams))
            {
                switch (message.ID)
                {
                    case ObjectStatesMessage.ID:
                        Handle(message.Get<ObjectStatesMessage>());
                        break;
                    case ShaderUIDRangeMessage.ID:
                        Handle(message.Get<ShaderUIDRangeMessage>());
                        break;
                    case ShaderCodeMessage.ID:
                        Handle(message.Get<ShaderCodeMessage>());
                        break;
                    case ShaderCodeFileMessage.ID:
                        Handle(message.Get<ShaderCodeFileMessage>());
                        break;
                }
            }
        }

        /// <summary>
        /// Message handler
        /// </summary>
        /// <param name="message"></param>
        private void Handle(ShaderCodeFileMessage message)
        {
            // Only care about first file
            if (message.fileUID != 0)
            {
                return;
            }
            
            // Flatten dynamics
            UInt64 uid = message.shaderUID;
            string filename = message.filename.String;

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (!_lookup.ContainsKey(uid))
                    return;

                ShaderIdentifierViewModel shaderIdentifierViewModel = _lookup[uid];
                shaderIdentifierViewModel.Descriptor = $"Shader {uid} - {System.IO.Path.GetFileName(filename)}";
            });
        }

        /// <summary>
        /// Message handler
        /// </summary>
        /// <param name="message"></param>
        private void Handle(ShaderCodeMessage message)
        {
            // Flatten
            ShaderCodeMessage.FlatInfo flat = message.Flat;

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (!_lookup.ContainsKey(flat.shaderUID))
                    return;

                ShaderIdentifierViewModel shaderIdentifierViewModel = _lookup[flat.shaderUID];
                
                // Mark shaders with no symbol data
                if (flat.native != 0)
                {
                    shaderIdentifierViewModel.Descriptor = $"Shader {flat.shaderUID} - No Symbols";
                }
            });
        }

        /// <summary>
        /// Message handler
        /// </summary>
        /// <param name="message"></param>
        private void Handle(ObjectStatesMessage message)
        {
            // Flatten message
            ObjectStatesMessage.FlatInfo flat = message.Flat;

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                // Prune dead models
                for (uint i = flat.shaderCount; i < ShaderIdentifiers.Count; i++)
                {
                    ShaderIdentifierViewModel shaderIdentifierViewModel = ShaderIdentifiers[(int)i];

                    // Remove GUID association if needed
                    if (shaderIdentifierViewModel.GUID != 0)
                    {
                        _lookup.Remove(shaderIdentifierViewModel.GUID);
                    }
                    
                    ShaderIdentifiers.RemoveAt((int)i);
                }

                // Add new models
                for (int i = ShaderIdentifiers.Count; i < flat.shaderCount; i++)
                {
                    ShaderIdentifiers.Add(new()
                    {
                        Workspace = WorkspaceViewModel,
                        GUID = 0,
                        Descriptor = "Loading..."
                    });
                }
            });
        }

        /// <summary>
        /// Message handler
        /// </summary>
        /// <param name="message"></param>
        private void Handle(ShaderUIDRangeMessage message)
        {
            uint start = message.start;
            
            var guids = new UInt64[message.shaderUID.Count];

            // Copy guids
            for (int i = 0; i < message.shaderUID.Count; i++)
            {
                guids[i] = message.shaderUID[i];
            }

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                var bus = _workspaceViewModel?.Connection?.GetSharedBus();
                
                for (int i = 0; i < guids.Length; i++)
                {
                    if (_lookup.ContainsKey(guids[i]))
                    {
                        // OK
                    }
                    else if (start + i < ShaderIdentifiers.Count)
                    {
                        ShaderIdentifierViewModel shaderIdentifierViewModel = ShaderIdentifiers[(int)start + i];

                        // Update identifier
                        shaderIdentifierViewModel.GUID = guids[i];
                        shaderIdentifierViewModel.Descriptor = $"Shader {guids[i]} - ...";

                        // Submit request for filenames
                        if (bus != null)
                        {
                            var request = bus.Add<GetShaderCodeMessage>();
                            request.poolCode = 0;
                            request.shaderUID = guids[i];
                        }

                        // Add association
                        _lookup.Add(guids[i], shaderIdentifierViewModel);
                    }
                }
            });
        }

        /// <summary>
        /// Invoked on refresh requests
        /// </summary>
        private void OnRefresh()
        {
            // Clear states
            ShaderIdentifiers.Clear();
            _lookup.Clear();
            
            // Immediate repool
            _workspaceViewModel?.Connection?.GetSharedBus().Add<GetObjectStatesMessage>();
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private IWorkspaceViewModel? _workspaceViewModel;

        /// <summary>
        /// Internal GUID association
        /// </summary>
        private Dictionary<UInt64, ShaderIdentifierViewModel> _lookup = new();

        /// <summary>
        /// Internal pooling timer
        /// </summary>
        private DispatcherTimer? _poolTimer;
    }
}
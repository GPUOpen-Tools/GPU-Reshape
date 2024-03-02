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
using System.Windows.Input;
using DynamicData;
using ReactiveUI;
using Runtime.Models.Objects;
using Runtime.ViewModels.Traits;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Properties.Config;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace Studio.ViewModels.Workspace.Properties
{
    public class WorkspaceCollectionViewModel : ReactiveObject, IPropertyViewModel, IWorkspaceAdapter, IInstrumentableObject, IClosableObject, IDescriptorObject
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name => ConnectionViewModel?.Application?.DecoratedName ?? "Invalid";

        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceTool;

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
        /// Document descriptor
        /// </summary>
        public IDescriptor? Descriptor { get; set; }

        /// <summary>
        /// Close command
        /// </summary>
        public ICommand? CloseCommand { get; set; }

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
        /// The represented workspace
        /// </summary>
        public IWorkspaceViewModel? WorkspaceViewModel { get; set; }

        /// <summary>
        /// Get the targetable instrumentation property
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetOrCreateInstrumentationProperty()
        {
            // Get global instrumentation
            var globalViewModel = this.GetProperty<GlobalViewModel>();
            if (globalViewModel == null)
            {
                Properties.Add(globalViewModel = new GlobalViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = ConnectionViewModel
                });
            }

            // OK
            return globalViewModel;
        }

        /// <summary>
        /// Instrumentation handler
        /// </summary>
        public InstrumentationState InstrumentationState
        {
            get
            {
                return this.GetProperty<GlobalViewModel>()?.InstrumentationState ?? new InstrumentationState();
            }
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            CreateProperties();
            CreateServices();
        }

        /// <summary>
        /// Create all services
        /// </summary>
        private void CreateServices()
        {
            // Create pulse
            var pulseService = new PulseService()
            {
                ConnectionViewModel = ConnectionViewModel
            };

            // Bind to local time
            pulseService.WhenAnyValue(x => x.LastPulseTime).Subscribe(x => ConnectionViewModel!.LocalTime = x);
            
            // Register pulse service
            Services.Add(pulseService);
            
            // Register bus service
            Services.Add(new BusPropertyService()
            {
                ConnectionViewModel = ConnectionViewModel
            });
            
            // Register versioning service
            Services.Add(new VersioningService()
            {
                ConnectionViewModel = ConnectionViewModel
            });
            
            // Register property replication service
            Services.Add(new PropertyReplicationService()
            {
                Property = this,
                ConnectionViewModel = ConnectionViewModel
            });
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
            
            // Toolable properties
            Properties.AddRange(new IPropertyViewModel[]
            {
                new LogViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                },
                
                new FeatureCollectionViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                },
                
                new MessageCollectionViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                },
                
                new ShaderCollectionViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                },

                new PipelineCollectionViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                }
            });
            
            // Configuration properties
            Properties.AddRange(new IPropertyViewModel[]
            {
                new ApplicationInstrumentationConfigViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                }
            });
        }

        /// <summary>
        /// Get the workspace
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel GetWorkspaceCollection()
        {
            return this;
        }

        /// <summary>
        /// Get the workspace represented by this adapter
        /// </summary>
        public IWorkspaceViewModel GetWorkspace()
        {
            return WorkspaceViewModel!;
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}
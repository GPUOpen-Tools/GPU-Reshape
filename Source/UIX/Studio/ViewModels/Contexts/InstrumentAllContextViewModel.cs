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
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive;
using System.Windows.Input;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Extensions;
using Studio.Models.Instrumentation;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Contexts
{
    public class InstrumentAllContextViewModel : ReactiveObject, IInstrumentAllContextViewModel
    {
        /// <summary>
        /// Target view models of the context
        /// </summary>
        public object[]? TargetViewModels
        {
            get => _targetViewModels;
            set
            {
                this.RaiseAndSetIfChanged(ref _targetViewModels, value);
                IsVisible = _targetViewModels?.Any(x => x is IInstrumentableObject) ?? false;
            }
        }

        /// <summary>
        /// Display header of this context model
        /// </summary>
        public string Header { get; set; } = "All Standard";
        
        /// <summary>
        /// All items within this context model
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Target command
        /// </summary>
        public ICommand Command { get; }

        /// <summary>
        /// Is this context enabled?
        /// </summary>
        public bool IsVisible
        {
            get => _isEnabled;
            set => this.RaiseAndSetIfChanged(ref _isEnabled, value);
        }

        public InstrumentAllContextViewModel()
        {
            Command = ReactiveCommand.Create(OnInvoked);
        }

        /// <summary>
        /// Command implementation
        /// </summary>
        private async void OnInvoked()
        {
            if (_targetViewModels?.PromoteOrNull<IInstrumentableObject>() is not {} instrumentableObjects)
            {
                return;
            }
            
            // Some features may have user input, wait until everything is done
            // All objects share the same workspace
            using var scope = new BusScope(instrumentableObjects[0].GetWorkspaceCollection()?.GetService<IBusPropertyService>(), BusMode.RecordAndCommit);

            // Instrument all objects
            foreach (IInstrumentableObject instrumentable in instrumentableObjects)
            {
                if (instrumentable.GetOrCreateInstrumentationProperty() is not { } propertyViewModel)
                {
                    continue;
                }
            
                // Create all instrumentation properties
                foreach (IInstrumentationPropertyService service in instrumentable.GetWorkspaceCollection()?.GetServices<IInstrumentationPropertyService>() ?? Enumerable.Empty<IInstrumentationPropertyService>())
                {
                    // Ignore non-standard
                    if (!service.Flags.HasFlag(InstrumentationFlag.Standard))
                    {
                        continue;
                    }
                
                    // Create feature
                    if (await service.CreateInstrumentationObjectProperty(propertyViewModel, false) is { } instrumentationObjectProperty)
                    {
                        propertyViewModel.Properties.Add(instrumentationObjectProperty);
                    }
                }
            }
        }

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = false;

        /// <summary>
        /// Internal target view models
        /// </summary>
        private object[]? _targetViewModels;
    }
}
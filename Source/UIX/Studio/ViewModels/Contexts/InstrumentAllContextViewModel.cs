// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Contexts
{
    public class InstrumentAllContextViewModel : ReactiveObject, IInstrumentAllContextViewModel
    {
        /// <summary>
        /// Target view model of the context
        /// </summary>
        public object? TargetViewModel
        {
            get => _targetViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _targetViewModel, value);
                IsVisible = _targetViewModel is IInstrumentableObject;
            }
        }

        /// <summary>
        /// Display header of this context model
        /// </summary>
        public string Header { get; set; } = "All";
        
        /// <summary>
        /// All items within this context model
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Target command
        /// </summary>
        public ICommand Command { get; }

        /// <summary>
        /// All ignored features
        /// </summary>
        public ISourceList<string> IgnoredFeatures { get; } = new SourceList<string>();

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
            if (_targetViewModel is not IInstrumentableObject instrumentable ||
                instrumentable.GetOrCreateInstrumentationProperty() is not { } propertyViewModel)
            {
                return;
            }
            
            // Some features may have user input, wait until everything is done
            using var scope = new BusScope(instrumentable.GetWorkspace()?.GetService<IBusPropertyService>(), BusMode.RecordAndCommit);

            // Create all instrumentation properties
            foreach (IInstrumentationPropertyService service in instrumentable.GetWorkspace()?.GetServices<IInstrumentationPropertyService>() ?? Enumerable.Empty<IInstrumentationPropertyService>())
            {
                // Ignored?
                if (IgnoredFeatures.Items.Contains(service.Name))
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

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = false;

        /// <summary>
        /// Internal target view model
        /// </summary>
        private object? _targetViewModel;
    }
}
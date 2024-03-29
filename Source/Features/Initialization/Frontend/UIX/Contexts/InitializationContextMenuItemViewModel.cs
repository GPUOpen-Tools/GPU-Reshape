﻿// 
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

using System.Collections.ObjectModel;
using System.Threading.Tasks;
using System.Windows.Input;
using Avalonia;
using DynamicData;
using GRS.Features.ResourceBounds.UIX.Workspace.Properties.Instrumentation;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Runtime.ViewModels;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
using Studio.Services;
using Studio.ViewModels;
using Studio.ViewModels.Contexts;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Adapter;
using Studio.ViewModels.Workspace.Properties;
using UIX.Resources;

namespace GRS.Features.Initialization.UIX.Contexts
{
    public class InitializationContextMenuItemViewModel : ReactiveObject, IContextMenuItemViewModel
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
                
                // Enabled if resource bounds is available
                _featureInfo = (_targetViewModel as IInstrumentableObject)?
                    .GetWorkspace()?
                    .GetProperty<IFeatureCollectionViewModel>()?
                    .GetFeature("Initialization");
                
                IsVisible = _featureInfo.HasValue;
            }
        }

        /// <summary>
        /// Display header of this context model
        /// </summary>
        public string Header { get; set; } = "Initialization";
        
        /// <summary>
        /// All items within this context model
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Context command
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

        public InitializationContextMenuItemViewModel()
        {
            Command = ReactiveCommand.Create(OnInvoked);
        }

        /// <summary>
        /// Command implementation
        /// </summary>
        private async void OnInvoked()
        {
            if (_targetViewModel is not IInstrumentableObject instrumentable ||
               instrumentable.GetOrCreateInstrumentationProperty() is not { } propertyViewModel ||
                _featureInfo == null)
            {
                return;
            }
            
            // Already instrumented?
            if (propertyViewModel.HasProperty<InitializationPropertyViewModel>())
            {
                return;
            }

            // Initialization instrumentation is a fickle game, if this is not a virtual adapter, i.e. launch from, warn the user about it.
            if (instrumentable.GetWorkspace()?.ConnectionViewModel is not IVirtualConnectionViewModel &&  (_data != null && await _data.ConditionalWarning()))
            {
                return;
            }
            
            // Add property
            propertyViewModel.Properties.Add(new InitializationPropertyViewModel()
            {
                Parent = propertyViewModel,
                ConnectionViewModel = propertyViewModel.ConnectionViewModel,
                FeatureInfo = _featureInfo.Value
            });
        }

        /// <summary>
        /// Queried feature information
        /// </summary>
        private FeatureInfo? _featureInfo;

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = false;

        /// <summary>
        /// Internal target view model
        /// </summary>
        private object? _targetViewModel;

        /// <summary>
        /// Plugin data
        /// </summary>
        private Data? _data = AvaloniaLocator.Current.GetService<ISettingsService>()?.Get<Data>();
    }
}
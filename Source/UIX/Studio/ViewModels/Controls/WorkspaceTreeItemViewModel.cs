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
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using DynamicData;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Controls
{
    public class WorkspaceTreeItemViewModel : ReactiveObject, IObservableTreeItem, IContainedViewModel
    {
        /// <summary>
        /// Display text of this item
        /// </summary>
        public string Text
        {
            get { return _text; }
            set { this.RaiseAndSetIfChanged(ref _text, value); }
        }

        /// <summary>
        /// Expansion state
        /// </summary>
        public bool IsExpanded
        {
            get { return _isExpanded; }
            set { this.RaiseAndSetIfChanged(ref _isExpanded, value); }
        }

        /// <summary>
        /// Foreground color of the item
        /// </summary>
        public ISolidColorBrush? StatusColor
        {
            get => _statusColor;
            set => this.RaiseAndSetIfChanged(ref _statusColor, value);
        }
        
        /// <summary>
        /// Open the settings window
        /// </summary>
        public ICommand OpenDocument { get; }
        
        /// <summary>
        /// Hosted view model
        /// </summary>
        /// <exception cref="NotSupportedException"></exception>
        public object? ViewModel
        {
            get => _propertyViewModel;
            set
            {
                var workspace = value as IPropertyViewModel ?? throw new NotSupportedException("Invalid view model");

                this.RaiseAndSetIfChanged(ref _propertyViewModel, workspace);

                OnPropertyChanged();
            }
        }

        /// <summary>
        /// Object which owns this tree item
        /// </summary>
        public object? OwningContext { get; set; }

        /// <summary>
        /// All child items
        /// </summary>
        public ObservableCollection<IObservableTreeItem> Items { get; } = new();

        /// <summary>
        /// Constructor
        /// </summary>
        public WorkspaceTreeItemViewModel()
        {
            OpenDocument = ReactiveCommand.Create(OnOpenDocument);
        }

        /// <summary>
        /// Invoked on document open
        /// </summary>
        private void OnOpenDocument()
        {
            if (ViewModel is not IDescriptorObject {} descriptorObject)
            {
                return;
            }
            
            if (App.Locator.GetService<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.DocumentLayout?.OpenDocument(descriptorObject.Descriptor);
            }
        }

        /// <summary>
        /// Invoked when the base property has changed
        /// </summary>
        private void OnPropertyChanged()
        {
            if (_propertyViewModel == null)
            {
                return;
            }
            
            // Set text
            Text = _propertyViewModel.Name;
            
            // Cleanup
            Items.Clear();

            // Bind connection status to color
            _propertyViewModel.GetService<IPulseService>()?
                .WhenAnyValue(x => x.MissedPulse)
                .Subscribe(x => StatusColor = x ? ResourceLocator.GetResource<SolidColorBrush>("ErrorBrush") : Brushes.White);

            // TODO: Unsubscribe?
            _propertyViewModel.Properties.Connect()
                .OnItemAdded(OnPropertyAdded)
                .OnItemRemoved(OnPropertyRemoved)
                .Subscribe();
        }

        /// <summary>
        /// Invoked when a property has been added
        /// </summary>
        /// <param name="propertyViewModel">added property</param>
        private void OnPropertyAdded(IPropertyViewModel propertyViewModel)
        {
            if (propertyViewModel.Visibility.HasFlag(PropertyVisibility.WorkspaceTool))
            {
                Items.Add(new WorkspaceTreeItemViewModel()
                {
                    OwningContext = propertyViewModel,
                    ViewModel = propertyViewModel
                });
            }
        }

        /// <summary>
        /// Invoked when a property has been removed
        /// </summary>
        /// <param name="propertyViewModel">removed property</param>
        private void OnPropertyRemoved(IPropertyViewModel propertyViewModel)
        {
            // Find root item
            IObservableTreeItem? item = Items.FirstOrDefault(x => x.ViewModel == propertyViewModel);

            // Found?
            if (item != null)
            {
                Items.Remove(item);
            }
        }

        /// <summary>
        /// Internal connection state
        /// </summary>
        private IPropertyViewModel? _propertyViewModel;

        /// <summary>
        /// Internal text state
        /// </summary>
        private string _text = "ObservableTreeItem";

        /// <summary>
        /// Internal status color
        /// </summary>
        private ISolidColorBrush? _statusColor = Brushes.White;

        /// <summary>
        /// Internal expansion state
        /// </summary>
        private bool _isExpanded = true;
    }
}
using System;
using System.Collections.ObjectModel;
using Avalonia.Media;
using ReactiveUI;
using Runtime.ViewModels.Shader;

namespace Studio.ViewModels.Controls
{
    public class FileTreeItemViewModel : ReactiveObject, IObservableTreeItem
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
        /// Hosted view model
        /// </summary>
        /// <exception cref="NotSupportedException"></exception>
        public object? ViewModel
        {
            get => _shaderFileViewModel;
            set => this.RaiseAndSetIfChanged(ref _shaderFileViewModel, value as ShaderFileViewModel);
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
        /// All child items
        /// </summary>
        public ObservableCollection<IObservableTreeItem> Items { get; } = new();

        /// <summary>
        /// Internal text state
        /// </summary>
        private string _text = "FileTreeItemViewModel";

        /// <summary>
        /// Internal status color
        /// </summary>
        private ISolidColorBrush? _statusColor = Brushes.White;

        /// <summary>
        /// Internal file
        /// </summary>
        private ShaderFileViewModel? _shaderFileViewModel;
        
        /// <summary>
        /// Internal expansion state
        /// </summary>
        private bool _isExpanded = true;
    }
}
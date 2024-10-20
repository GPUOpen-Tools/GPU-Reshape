﻿using System.Collections.ObjectModel;
using System.Windows.Input;

namespace Studio.ViewModels.Contexts
{
    public class CategoryContextMenuItemViewModel : IContextMenuItemViewModel
    {
        /// <summary>
        /// Given header
        /// </summary>
        public string Header { get; set; } = string.Empty;

        /// <summary>
        /// All hosted items
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Command on invoke
        /// </summary>
        public ICommand? Command { get; } = null;

        /// <summary>
        /// If this context menu is enabled
        /// </summary>
        public bool IsVisible { get; set; } = true;
        
        /// <summary>
        /// Underlying target view models for the context menu
        /// </summary>
        public object[]? TargetViewModels { get; set; }
    }
}
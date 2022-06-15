using System.Collections.ObjectModel;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;

namespace Studio.ViewModels.Controls
{
    public interface IObservableTreeItem
    {
        /// <summary>
        /// Display text of this item
        /// </summary>
        public string Text { get; set; }

        /// <summary>
        /// Associated view model contained within this tree item
        /// </summary>
        public object? ViewModel { get; set; }

        /// <summary>
        /// All child items
        /// </summary>
        public ObservableCollection<IObservableTreeItem> Items { get; }
    }
}
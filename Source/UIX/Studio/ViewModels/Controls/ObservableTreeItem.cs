using System.Collections.ObjectModel;
using DynamicData;
using ReactiveUI;

namespace Studio.ViewModels.Controls
{
    public class ObservableTreeItem : ReactiveObject
    {
        /// <summary>
        /// Is this item expanded?
        /// </summary>
        public bool IsExpanded
        {
            get { return _isExpanded; }
            set { this.RaiseAndSetIfChanged(ref _isExpanded, value); }
        }
       
        /// <summary>
        /// Is this item selected?
        /// </summary>
        public bool IsSelected
        {
            get { return _isSelected; }
            set { this.RaiseAndSetIfChanged(ref _isSelected, value); }
        }
        
        /// <summary>
        /// Display text of this item
        /// </summary>
        public string Text
        {
            get { return _text; }
            set { this.RaiseAndSetIfChanged(ref _text, value); }
        }

        /// <summary>
        /// All child items
        /// </summary>
        public IObservableList<ObservableTreeItem> Items { get; } = new SourceList<ObservableTreeItem>();

        /// <summary>
        /// Internal expanded state
        /// </summary>
        bool _isExpanded = false;
        
        /// <summary>
        /// Internal selected state
        /// </summary>
        bool _isSelected = false;

        /// <summary>
        /// Internal text state
        /// </summary>
        private string _text = "ObservableTreeItem";
    }
}
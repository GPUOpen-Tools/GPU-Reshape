using System;
using System.Collections.ObjectModel;
using System.Linq;
using DynamicData;
using ReactiveUI;
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
        /// All child items
        /// </summary>
        public ObservableCollection<IObservableTreeItem> Items { get; } = new();
        
        /// <summary>
        /// Invoked when the base property has changed
        /// </summary>
        private void OnPropertyChanged()
        {
            Text = _propertyViewModel.Name;

            Items.Clear();

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
            Items.Add(new WorkspaceTreeItemViewModel()
            {
                ViewModel = propertyViewModel
            });
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
        private IPropertyViewModel _propertyViewModel;

        /// <summary>
        /// Internal text state
        /// </summary>
        private string _text = "ObservableTreeItem";
    }
}
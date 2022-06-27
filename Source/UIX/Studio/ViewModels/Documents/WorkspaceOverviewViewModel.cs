using System;
using System.Collections.ObjectModel;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using ReactiveUI;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Documents
{
    public class WorkspaceOverviewViewModel : Document
    {
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public Workspace.WorkspaceViewModel? Workspace
        {
            get => _workspaceViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _workspaceViewModel, value);
                OnWorkspaceChanged();
            }
        }

        /// <summary>
        /// All properties of this view
        /// </summary>
        public ObservableCollection<IPropertyViewModel> Properties { get; } = new();

        /// <summary>
        /// Invoked when the workspace has changed
        /// </summary>
        public void OnWorkspaceChanged()
        {
            // Filter the owning collection properties
            _workspaceViewModel?.PropertyCollection.Properties.Connect()
                .OnItemAdded(x =>
                {
                    if (x.Visibility.HasFlag(PropertyVisibility.WorkspaceOverview))
                    {
                        Properties.Add(x);
                    }
                })
                .OnItemRemoved(x =>
                {
                    Properties.Remove(x);
                })
                .Subscribe();
        }

        /// <summary>
        /// Underlying view model
        /// </summary>
        private Workspace.WorkspaceViewModel? _workspaceViewModel;
    }
}
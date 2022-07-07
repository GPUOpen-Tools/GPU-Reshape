using System;
using System.Collections.ObjectModel;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using ReactiveUI;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Documents
{
    public class WorkspaceOverviewViewModel : Document, IDocumentViewModel
    {
        /// <summary>
        /// Document icon
        /// </summary>
        public StreamGeometry? Icon
        {
            get => _icon;
            set => this.RaiseAndSetIfChanged(ref _icon, value);
        }

        /// <summary>
        /// Icon color
        /// </summary>
        public Color? IconForeground
        {
            get => _iconForeground;
            set => this.RaiseAndSetIfChanged(ref _iconForeground, value);
        }

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
        
        /// <summary>
        /// Internal icon
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("StreamOn");

        /// <summary>
        /// Internal icon color
        /// </summary>
        private Color? _iconForeground = ResourceLocator.GetResource<Color>("ThemeForegroundColor");
    }
}
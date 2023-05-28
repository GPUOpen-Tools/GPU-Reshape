using Avalonia;
using DynamicData;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Services
{
    public class WorkspaceService : ReactiveObject, IWorkspaceService
    {
        /// <summary>
        /// Active workspaces
        /// </summary>
        public IObservableList<ViewModels.Workspace.IWorkspaceViewModel> Workspaces => _workspaces;

        /// <summary>
        /// All extensions
        /// </summary>
        public ISourceList<IWorkspaceExtension> Extensions { get; } = new SourceList<IWorkspaceExtension>();

        /// <summary>
        /// Current workspace
        /// </summary>
        public IWorkspaceViewModel? SelectedWorkspace
        {
            get => _selectedWorkspace;
            set => this.RaiseAndSetIfChanged(ref _selectedWorkspace, value);
        }

        /// <summary>
        /// Current selected property
        /// </summary>
        public IPropertyViewModel? SelectedProperty
        {
            get => _selectedProperty;
            set => this.RaiseAndSetIfChanged(ref _selectedProperty, value);
        }

        /// <summary>
        /// Current selected property
        /// </summary>
        public ShaderNavigationViewModel? SelectedShader
        {
            get => _selectedShader;
            set => this.RaiseAndSetIfChanged(ref _selectedShader, value);
        }

        public WorkspaceService()
        {
            
        }
        
        /// <summary>
        /// Add a new workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        public void Add(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel)
        {
            _workspaces.Add(workspaceViewModel);
            
            // Submit document
            if (App.Locator.GetService<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.OpenDocument?.Execute(new WorkspaceOverviewDescriptor()
                {
                    Workspace = workspaceViewModel 
                });
            }
            
            // Diagnostic
            Logging.Info($"Workspace created for {workspaceViewModel.Connection?.Application?.Name} {{{workspaceViewModel.Connection?.Application?.Guid}}}");
            
            // Assign as selected
            SelectedWorkspace = workspaceViewModel;
        }

        /// <summary>
        /// Install a workspace
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        public void Install(IWorkspaceViewModel workspaceViewModel)
        {
            foreach (IWorkspaceExtension workspaceExtension in Extensions.Items)
            {
                workspaceExtension.Install(workspaceViewModel);
            }
        }

        /// <summary>
        /// Remove an existing workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        /// <returns>true if successfully removed</returns>
        public bool Remove(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel)
        {
            // Diagnostic
            Logging.Info($"Closed workspace for {workspaceViewModel.Connection?.Application?.Name}");
            
            // Is selected?
            if (SelectedWorkspace == workspaceViewModel)
            {
                SelectedWorkspace = null;
            }
            
            // Try to remove
            return _workspaces.Remove(workspaceViewModel);
        }

        /// <summary>
        /// Clear all workspaces
        /// </summary>
        public void Clear()
        {
            if (_workspaces.Count > 0)
            {
                // Diagnostic
                Logging.Info($"Closing all workspaces");

                // Remove selected
                SelectedWorkspace = null;
            
                // Remove all
                _workspaces.Clear();
            }
        }

        /// <summary>
        /// Internal active workspaces
        /// </summary>
        private SourceList<ViewModels.Workspace.IWorkspaceViewModel> _workspaces = new();

        /// <summary>
        /// Internal active workspace
        /// </summary>
        private IWorkspaceViewModel? _selectedWorkspace;

        /// <summary>
        /// Internal selected property
        /// </summary>
        private IPropertyViewModel? _selectedProperty;

        /// <summary>
        /// Internal selected shader
        /// </summary>
        private ShaderNavigationViewModel? _selectedShader;
    }
}
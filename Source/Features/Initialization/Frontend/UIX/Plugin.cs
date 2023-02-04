using System;
using Avalonia;
using DynamicData;
using GRS.Features.Initialization.UIX.Contexts;
using GRS.Features.Initialization.UIX.Workspace;
using Studio.Plugin;
using Studio.Services;
using Studio.ViewModels.Contexts;
using Studio.ViewModels.Workspace;

namespace GRS.Features.Initialization.UIX
{
    public class Plugin : IPlugin
    {
        /// <summary>
        /// Plugin info
        /// </summary>
        public PluginInfo Info { get; } = new()
        {
            Name = "InitializationService",
            Description = "Instrumentation and validation of resource initialization prior to reads",
            Dependencies = new string[]{ }
        };
        
        /// <summary>
        /// Install this plugin
        /// </summary>
        /// <returns></returns>
        public bool Install()
        {
            // Add to context menus
            AvaloniaLocator.Current.GetService<IContextMenuService>()?.ViewModel
                .GetItem<IInstrumentContextViewModel>()?
                .Items.Add(new InitializationContextMenuItemViewModel());
            
            // Connect to workspaces
            AvaloniaLocator.Current.GetService<IWorkspaceService>()?.Workspaces.Connect()
                .OnItemAdded(OnWorkspaceAdded)
                .Subscribe();

            // OK
            return true;
        }

        /// <summary>
        /// Uninstall this plugin
        /// </summary>
        public void Uninstall()
        {
            
        }

        /// <summary>
        /// Invoked when a workspace has been added
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        private void OnWorkspaceAdded(IWorkspaceViewModel workspaceViewModel)
        {
            // Create service
            workspaceViewModel.PropertyCollection.Services.Add(new InitializationService(workspaceViewModel));
        }
    }
}

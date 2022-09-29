using System;
using Avalonia;
using DynamicData;
using Features.ResourceBounds.UIX.Extensions;
using Studio.Plugin;
using Studio.Services;
using Studio.ViewModels.Workspace;

namespace Features.ResourceBounds.UIX
{
    public class Plugin : IPlugin
    {
        public PluginInfo Info { get; } = new()
        {
            Name = "ResourceBounds",
            Description = "Provides instrumentation around resource data indexing operations",
            Dependencies = new string[]{ }
        };
        
        /// <summary>
        /// Install this plugin
        /// </summary>
        /// <returns></returns>
        public bool Install()
        {
            // Get workspace service
            var workspaceService = AvaloniaLocator.Current.GetService<IWorkspaceService>();
            if (workspaceService == null)
                return false;

            // Connect to workspaces
            workspaceService.Workspaces.Connect()
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
            workspaceViewModel.PropertyCollection.Services.Add(new ResourceBoundsService(workspaceViewModel));
        }
    }
}

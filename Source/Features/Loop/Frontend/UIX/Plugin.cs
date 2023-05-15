using System;
using Avalonia;
using DynamicData;
using GRS.Features.Loop.UIX.Contexts;
using GRS.Features.Loop.UIX.Workspace;
using Studio.Plugin;
using Studio.Services;
using Studio.ViewModels.Contexts;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;

namespace GRS.Features.Loop.UIX
{
    public class Plugin : IPlugin, IWorkspaceExtension
    {
        /// <summary>
        /// Plugin info
        /// </summary>
        public PluginInfo Info { get; } = new()
        {
            Name = "Loop",
            Description = "Instrumentation and validation of infinite loops",
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
                .Items.Add(new LoopContextMenuItemViewModel());
            
            // Add workspace extension
            AvaloniaLocator.Current.GetService<IWorkspaceService>()?.Extensions.Add(this);

            // OK
            return true;
        }

        /// <summary>
        /// Uninstall this plugin
        /// </summary>
        public void Uninstall()
        {
            // Remove workspace extension
            AvaloniaLocator.Current.GetService<IWorkspaceService>()?.Extensions.Remove(this);
        }

        /// <summary>
        /// Install an extension
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        public void Install(IWorkspaceViewModel workspaceViewModel)
        {
            // Create service
            workspaceViewModel.PropertyCollection.Services.Add(new LoopService(workspaceViewModel));
        }
    }
}

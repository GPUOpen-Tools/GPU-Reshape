using System;
using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using ReactiveUI;
using Runtime.ViewModels.Tools;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Tools
{
    public class PropertyViewModel : ToolViewModel
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public override StreamGeometry? Icon => ResourceLocator.GetIcon("ToolProperty");
        
        /// <summary>
        /// All configurations
        /// </summary>
        public IEnumerable<IPropertyViewModel>? SelectedPropertyConfigurations
        {
            get => _selectedPropertyConfigurations;
            set => this.RaiseAndSetIfChanged(ref _selectedPropertyConfigurations, value);
        }

        public PropertyViewModel()
        {
            // Bind to selected property
            App.Locator.GetService<IWorkspaceService>()?
                .WhenAnyValue(x => x.SelectedProperty)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Filter by configurations
                    SelectedPropertyConfigurations = x.Properties.Items.Where(p => p.Visibility == PropertyVisibility.Configuration);
                });
        }

        /// <summary>
        /// Internal configurations
        /// </summary>
        private IEnumerable<IPropertyViewModel>? _selectedPropertyConfigurations;
    }
}

using System;
using Avalonia.Controls;
using Avalonia.ExtendedToolkit.Controls.PropertyGrid.PropertyTypes;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Tools;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Views.Tools
{
    public partial class PropertyView : UserControl
    {
        public PropertyView()
        {
            InitializeComponent();

            // Bind context
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<PropertyViewModel>()
                .Subscribe(x =>
                {
                    // Bind configurations
                    x.WhenAnyValue(y => y.SelectedPropertyConfigurations)
                        .Subscribe(y =>
                        {
                            // Create new descriptor
                            PropertyGrid.SelectedObject = new Property.PropertyCollectionTypeDescriptor(y ?? Array.Empty<IPropertyViewModel>());
                            
                            // Clear previous metadata, internally a type based cache is used, this is not appropriate
                            MetadataRepository.Clear();
                            
                            // Finally, recreate the property setup with fresh metadata
                            PropertyGrid.ReloadCommand.Execute(null);
                            
                            // Expand all categories
                            PropertyGrid.Categories.ForEach(c => c.IsExpanded = true);
                        });
                });
        }
    }
}
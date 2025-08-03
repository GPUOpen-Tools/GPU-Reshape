using System.Linq;
using Avalonia.Controls;
using Avalonia.Controls.Shapes;
using Avalonia.Media;
using Avalonia.VisualTree;
using Studio.Extensions;

namespace Studio.Views.Controls
{
    public partial class PropertyGrid : UserControl
    {
        public PropertyGrid()
        {
            InitializeComponent();
            
            // Workaround for dynamic resource bound during visual parsing
            ItemsControl.LayoutUpdated += (_, _) =>
            {
                ItemsControl.GetVisualDescendants().OfType<DataGrid>().ForEach(dataGrid =>
                {
                    dataGrid.GetVisualDescendants()
                        .OfType<Rectangle>()
                        .Where(r => r.Name == "PART_ColumnHeadersAndRowsSeparator")
                        .ForEach(rect =>
                        {
                            rect.Fill = Brushes.Transparent;
                        }
                    );
                });
            };
        }
    }
}

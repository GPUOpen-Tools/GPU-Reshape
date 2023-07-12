using Avalonia.Controls;
using Dock.Avalonia.Controls;

namespace Studio.ViewModels.Controls.Themes
{
    public class CollapsableDockTabStrip : DockTabStrip
    {
        public CollapsableDockTabStrip()
        {
            SelectionMode = SelectionMode.Single;
        }
    }
}
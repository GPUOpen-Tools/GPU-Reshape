using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using Runtime.ViewModels.Tools;

namespace Studio.ViewModels.Tools
{
    public class ConnectionViewModel : ToolViewModel
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public override StreamGeometry? Icon => ResourceLocator.GetIcon("ToolConnection");
    }
}

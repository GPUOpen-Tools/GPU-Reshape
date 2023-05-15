using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;

namespace Runtime.ViewModels.Tools
{
    public abstract class ToolViewModel : Tool
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public abstract StreamGeometry? Icon { get; }
    }
}
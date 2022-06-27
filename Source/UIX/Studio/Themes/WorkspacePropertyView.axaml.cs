using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Studio.Views.Controls
{
    public partial class WorkspacePropertyView : UserControl
    {
        /// <summary>
        /// Title style for the given view
        /// </summary>
        public static readonly StyledProperty<string> TitleProperty = AvaloniaProperty.Register<Border, string>(nameof(Title));
        
        /// <summary>
        /// Title getter / setter
        /// </summary>
        public string Title
        {
            get => GetValue(TitleProperty);
            set => SetValue(TitleProperty, value);
        }
    }
}
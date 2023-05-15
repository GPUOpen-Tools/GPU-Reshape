using Avalonia.Data.Converters;

namespace Studio.ValueConverters.Themes
{
    public static class ThemeConverters
    {
        /// <summary>
        /// Default tab strip converter
        /// </summary>
        public static IValueConverter ToolControlTabStripDock = new ToolControlTabStripDockConverter();
    }
}
using Avalonia;
using Avalonia.Media;
using Avalonia.Media.TextFormatting;

namespace Runtime
{
    public static class TextFormatter
    {
        /// <summary>
        /// Helper formatter for glyph size estimates
        /// </summary>
        /// <param name="text">source text</param>
        /// <param name="fontSize">expected font size</param>
        /// <returns></returns>
        public static Size GetWidth(string text, double fontSize)
        {
            return new TextLayout(text, Typeface.Default, fontSize, null).Size;
        }
    }
}
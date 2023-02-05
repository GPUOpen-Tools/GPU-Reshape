using System;
using System.IO;
using System.Text;
using Avalonia;
using Avalonia.Markup.Xaml;
using Avalonia.Platform;

namespace Studio.MarkupExtensions
{
    public class FileExtension : MarkupExtension
    {
        /// <summary>
        /// Stream filename
        /// </summary>
        private readonly string FileName;

        public FileExtension(string fileName)
        {
            FileName = fileName;
        }
        
        public override object ProvideValue(IServiceProvider serviceProvider)
        {
            // Get asset provider
            var assets = AvaloniaLocator.Current.GetService<IAssetLoader>() ?? throw new Exception("Expected asset loader");

            // Attempt to read the contents
            using var reader = new StreamReader(assets.Open(new Uri("avares://GPUReshape/" + FileName)));
            return reader.ReadToEnd();
        }
    }
}
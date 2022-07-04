using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Avalonia;
using Avalonia.Media;
using AvaloniaEdit.Document;
using AvaloniaEdit.Rendering;
using DynamicData;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.Views.Editor
{
    public class ValidationBackgroundRenderer : IBackgroundRenderer
    {
        /// <summary>
        /// Current document
        /// </summary>
        public TextDocument? Document { get; set; }

        /// <summary>
        /// All validation objects
        /// </summary>
        public ObservableCollection<ValidationObject>? ValidationObjects { get; set; }

        /// <summary>
        /// Invoked on document draws
        /// </summary>
        /// <param name="textView"></param>
        /// <param name="drawingContext"></param>
        public void Draw(TextView textView, DrawingContext drawingContext)
        {
            if (Document == null)
            {
                return;
            }
            
            // Build lines
            textView.EnsureVisualLines();
            
            // Draw all objects
            foreach (ValidationObject validationObject in ValidationObjects ?? Enumerable.Empty<ValidationObject>())
            {
                if (validationObject.Segment == null)
                {
                    continue;
                }

                // Normalize Y
                uint Y = (uint)Math.Floor((validationObject.Segment.Location.Line / (float)Document.LineCount) * textView.Bounds.Height);

                // Default width
                uint Width = 15;
                
                // Line wise segment
                drawingContext.DrawRectangle(_validationBrush, null, new Rect(
                    textView.Bounds.Width - Width - 2.5, Y,
                    Width, 2.5
                ));
            }
        }

        /// <summary>
        /// Default validation brush
        /// </summary>
        private Brush _validationBrush = ResourceLocator.GetResource<SolidColorBrush>("ErrorBrush");

        /// <summary>
        /// Underlying layer
        /// </summary>
        public KnownLayer Layer { get; }
    }
}
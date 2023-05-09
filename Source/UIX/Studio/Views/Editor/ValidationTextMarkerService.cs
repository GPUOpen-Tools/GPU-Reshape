using System;
using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Media;
using AvaloniaEdit.Document;
using AvaloniaEdit.Rendering;
using Studio.Extensions;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.Views.Editor
{
    public class ValidationTextMarkerService : DocumentColorizingTransformer, IBackgroundRenderer, ITextViewConnect
    {
        public TextDocument? Document { get; set; }
        
        /// <summary>
        /// Current content view model
        /// </summary>
        public CodeShaderContentViewModel? ShaderContentViewModel { get; set; }

        /// <summary>
        /// Currently highlighted object
        /// </summary>
        public ValidationObject? HighlightObject { get; set; }

        /// <summary>
        /// Currently selected object
        /// </summary>
        public ValidationObject? SelectedObject { get; set; }

        /// <summary>
        /// Invoked on line draws / colorization 
        /// </summary>
        /// <param name="line"></param>
        protected override void ColorizeLine(DocumentLine line)
        {
            // For all segments in this line
            foreach (ValidationTextSegment marker in _markers.FindOverlappingSegments(line.Offset, line.Length))
            {
                ChangeLinePart(
                    Math.Max(marker.StartOffset, line.Offset),
                    Math.Min(marker.EndOffset, line.Offset + line.Length),
                    element =>
                    {
                        // Set background
                        element.TextRunProperties.BackgroundBrush = _validationBrush;

                        // Modify type face (todo...)
                        element.TextRunProperties.Typeface = new Typeface(
                            element.TextRunProperties.Typeface.FontFamily,
                            FontStyle.Normal,
                            FontWeight.Normal
                        );
                    }
                );
            }
        }

        /// <summary>
        /// Invoked on document draw
        /// </summary>
        /// <param name="textView"></param>
        /// <param name="drawingContext"></param>
        public void Draw(TextView textView, DrawingContext drawingContext)
        {
            if (!textView.VisualLinesValid || textView.VisualLines.Count == 0)
            {
                return;
            }

            // All visual, meaning visible / rendered lines
            var visualLines = textView.VisualLines;

            // View bounds
            int viewStart = visualLines.First().FirstDocumentLine.Offset;
            int viewEnd = visualLines.Last().LastDocumentLine.EndOffset;

            // Limit to current view
            foreach (ValidationTextSegment marker in _markers.FindOverlappingSegments(viewStart, viewEnd - viewStart))
            {
                BackgroundGeometryBuilder geoBuilder = new BackgroundGeometryBuilder();
                geoBuilder.AlignToWholePixels = true;
                geoBuilder.CornerRadius = 3;
                geoBuilder.AddSegment(textView, marker);

                // Create geometry
                Geometry geometry = geoBuilder.CreateGeometry();
                if (geometry != null)
                {
                    drawingContext.DrawGeometry(_validationBrush, null, geometry);
                }

                // Get all draw rects
                foreach (Rect rect in BackgroundGeometryBuilder.GetRectsForSegment(textView, marker))
                {
                    // Create formatted segment
                    FormattedText formatted = new FormattedText()
                    {
                        Text = $"{marker.Object?.Content} [{marker.Object?.Count}]",
                        Typeface = new Typeface(FontFamily.Default),
                        FontSize = 12
                    };
                    
                    // Set style
                    formatted.SetTextStyle(0, marker.Object?.Content.Length ?? 0, new SolidColorBrush(Colors.White));

                    // Final brush
                    IBrush brush = _validationBrush;

                    // Is highlighted?
                    if (marker.Object == HighlightObject)
                    {
                        brush = _validationHighlightBrush;
                    }

                    // Is selected?
                    if (marker.Object == SelectedObject)
                    {
                        brush = _validationSelectedBrush;
                    }
                    
                    // Border rectangle
                    drawingContext.DrawRectangle(brush, null, new Rect(
                        textView.Bounds.Width - formatted.Bounds.Width - _borderPadding * 2, rect.TopLeft.Y,
                        formatted.Bounds.Width + _borderPadding * 2, rect.BottomLeft.Y - rect.TopLeft.Y
                    ));
                    
                    // Draw validation error
                    drawingContext.DrawText(new SolidColorBrush(Colors.Black), new Point(textView.Bounds.Width - formatted.Bounds.Width - _borderPadding, rect.TopLeft.Y), formatted);
                }
            }
        }

        /// <summary>
        /// Add a new validation object
        /// </summary>
        /// <param name="validationObject"></param>
        public void Add(ValidationObject validationObject)
        {
            if (Document == null || validationObject.Segment == null || validationObject.Segment.Location.Line + 1 >= (Document?.Lines.Count ?? 0))
            {
                return;
            }

            // Mismatched file?
            if (validationObject.Segment.Location.FileUID != (ShaderContentViewModel?.SelectedShaderFileViewModel?.UID ?? uint.MaxValue))
            {
                return;
            }

            // Get line
            DocumentLine line = Document!.Lines[validationObject.Segment.Location.Line];
            
            // Create segment for bounds
            var segment = new ValidationTextSegment()
            {
                StartOffset = line.Offset,
                EndOffset = line.EndOffset,
                Length = line.Length,
                Object = validationObject
            };

            // Add lookup
            _markers.Add(segment);
            _segments.Add(validationObject, segment);
        }

        /// <summary>
        /// Resummarize all validation objects
        /// </summary>
        public void ResumarizeValidationObjects()
        {
            // Remove lookups
            _markers.Clear();
            _segments.Clear();
            
            // Add as new
            ShaderContentViewModel?.Object?.ValidationObjects.ForEach(x => Add(x));
        }

        /// <summary>
        /// Remove a validation object
        /// </summary>
        /// <param name="validationObject"></param>
        public void Remove(ValidationObject validationObject)
        {
            _markers.Remove(_segments[validationObject]);
            _segments.Remove(validationObject);
        }

        /// <summary>
        /// Find a validation object under a given point
        /// </summary>
        /// <param name="textView"></param>
        /// <param name="point"></param>
        /// <returns></returns>
        public ValidationObject? FindObjectUnder(TextView textView, Point point)
        {
            if (!textView.VisualLinesValid || textView.VisualLines.Count == 0)
            {
                return null;
            }
            
            // Find line of interest
            if (textView.GetVisualLineFromVisualTop(textView.VerticalOffset + point.Y) is not {} line)
            {
                return null;
            }
            
            // Distance to the right hand side
            double rightDistance = textView.Bounds.Width - point.X;

            // Limit to current view
            foreach (ValidationTextSegment marker in _markers.FindOverlappingSegments(line.FirstDocumentLine.Offset, line.LastDocumentLine.Offset - line.FirstDocumentLine.Offset))
            {
                // Create formatted segment
                FormattedText formatted = new FormattedText()
                {
                    Text = $"{marker.Object?.Content} [{marker.Object?.Count}]",
                    Typeface = new Typeface(FontFamily.Default),
                    FontSize = 12
                };
                    
                // Set style
                formatted.SetTextStyle(0, marker.Object?.Content.Length ?? 0, new SolidColorBrush(Colors.White));
                
                // Valid marker and under the cursor?
                if (marker.Object != null && formatted.Bounds.Width + _borderPadding * 2 >= rightDistance)
                {
                    return marker.Object;
                }
            }

            // None found
            return null;
        }

        /// <summary>
        /// Error brush
        /// </summary>
        private Brush _validationBrush = ResourceLocator.GetResource<SolidColorBrush>("ErrorBrush") ?? new SolidColorBrush(Colors.Red);

        /// <summary>
        /// Error selected brush
        /// </summary>
        private Brush _validationSelectedBrush = ResourceLocator.GetResource<SolidColorBrush>("ErrorSelectedBrush") ?? new SolidColorBrush(Colors.Red);

        /// <summary>
        /// Error highlight brush
        /// </summary>
        private Brush _validationHighlightBrush = ResourceLocator.GetResource<SolidColorBrush>("ErrorHighlightBrush") ?? new SolidColorBrush(Colors.Red);

        /// <summary>
        /// All active segments
        /// </summary>
        TextSegmentCollection<ValidationTextSegment> _markers = new();

        /// <summary>
        /// Object to segment mapping
        /// </summary>
        private Dictionary<ValidationObject, ValidationTextSegment> _segments = new();

        /// <summary>
        /// Default padding
        /// </summary>
        private int _borderPadding = 10;
        
        /// <summary>
        /// Underlying layer
        /// </summary>
        public KnownLayer Layer { get; }
    }
}
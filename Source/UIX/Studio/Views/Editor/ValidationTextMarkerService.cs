// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

using System;
using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Media;
using AvaloniaEdit.Document;
using AvaloniaEdit.Rendering;
using Runtime.ViewModels.IL;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
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
        public IShaderContentViewModel? ShaderContentViewModel { get; set; }

        /// <summary>
        /// Currently highlighted object
        /// </summary>
        public ValidationObject? HighlightObject { get; set; }

        /// <summary>
        /// Currently selected object
        /// </summary>
        public ValidationObject? SelectedObject { get; set; }

        /// <summary>
        /// Validation target
        /// </summary>
        public ValidationTarget Target { get; set; }

        /// <summary>
        /// Optional assembler
        /// </summary>
        public Assembler? Assembler { get; set; }

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
            // Get the actual line
            int line = GetValidationLine(validationObject.Segment!.Location);
            
            // Valid?
            if (Document == null || validationObject.Segment == null || line + 1 >= (Document?.Lines.Count ?? 0))
            {
                return;
            }

            // Mismatched file?
            if (!(ShaderContentViewModel?.IsObjectVisible(validationObject) ?? false))
            {
                return;
            }

            // Get line
            DocumentLine documentLine = Document!.Lines[line];
            
            // Create segment for bounds
            var segment = new ValidationTextSegment()
            {
                StartOffset = documentLine.Offset,
                EndOffset = documentLine.EndOffset,
                Length = documentLine.Length,
                Object = validationObject
            };

            // Add lookup
            _markers.Add(segment);
            _segments.Add(validationObject, segment);
        }


        /// <summary>
        /// Get the textual line of a location
        /// </summary>
        private int GetValidationLine(ShaderLocation location)
        {
            switch (Target)
            {
                case ValidationTarget.Source:
                {
                    return location.Line;
                }
                case ValidationTarget.IL:
                {
                    return (int)(Assembler?.GetMapping(location.BasicBlockId, location.InstructionIndex).Line ?? 0);
                }
                default:
                    throw new ArgumentOutOfRangeException();
            }
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
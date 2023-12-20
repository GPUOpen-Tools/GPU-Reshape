// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
using Avalonia.Media;
using AvaloniaEdit.Document;
using AvaloniaEdit.Rendering;
using Runtime.ViewModels.IL;
using Studio.Extensions;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.Views.Editor
{
    public class ValidationTextMarkerService : DocumentColorizingTransformer, IBackgroundRenderer
    {
        public TextDocument? Document { get; set; }
        
        /// <summary>
        /// Current content view model
        /// </summary>
        public ITextualShaderContentViewModel? ShaderContentViewModel { get; set; }

        /// <summary>
        /// Invoked on line draws / colorization 
        /// </summary>
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
        /// Invoked on document drawing
        /// </summary>
        public void Draw(TextView textView, DrawingContext drawingContext)
        {
            // Nothing
        }

        /// <summary>
        /// Add a new validation object
        /// </summary>
        public void Add(ValidationObject validationObject)
        {
            // Get the actual line
            int line = ShaderContentViewModel?.TransformLine(validationObject.Segment!.Location) ?? 0;
            
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
        /// Error brush
        /// </summary>
        private Brush _validationBrush = ResourceLocator.GetResource<SolidColorBrush>("ErrorBrush") ?? new SolidColorBrush(Colors.Red);

        /// <summary>
        /// All active segments
        /// </summary>
        TextSegmentCollection<ValidationTextSegment> _markers = new();

        /// <summary>
        /// Object to segment mapping
        /// </summary>
        private Dictionary<ValidationObject, ValidationTextSegment> _segments = new();

        /// <summary>
        /// Underlying layer
        /// </summary>
        public KnownLayer Layer { get; }
    }
}
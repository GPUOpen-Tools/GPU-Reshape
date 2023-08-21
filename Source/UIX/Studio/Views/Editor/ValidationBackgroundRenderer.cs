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
using System.Collections.ObjectModel;
using System.Linq;
using Avalonia;
using Avalonia.Media;
using AvaloniaEdit.Document;
using AvaloniaEdit.Rendering;
using DynamicData;
using Runtime.ViewModels.IL;
using Runtime.ViewModels.Shader;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;
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
        /// Current content view model
        /// </summary>
        public IShaderContentViewModel? ShaderContentViewModel { get; set; }

        /// <summary>
        /// Validation target
        /// </summary>
        public ValidationTarget Target { get; set; }
        
        /// <summary>
        /// Optional assembler
        /// </summary>
        public Assembler? Assembler { get; set; }

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
            // Valid?
            if (ShaderContentViewModel == null || Document == null || !ShaderContentViewModel.IsOverlayVisible())
            {
                return;
            }
            
            // Build lines
            textView.EnsureVisualLines();
            
            // Draw all objects
            foreach (ValidationObject validationObject in ValidationObjects ?? Enumerable.Empty<ValidationObject>())
            {
                // Valid file?
                if (!ShaderContentViewModel.IsObjectVisible(validationObject))
                {
                    continue;
                }

                // Get the line
                int line = GetValidationLine(validationObject.Segment!.Location);
                
                // Normalize Y
                uint y = (uint)Math.Floor((line / (float)Document.LineCount) * textView.Bounds.Height);

                // Default width
                uint width = 15;
                
                // Line wise segment
                drawingContext.DrawRectangle(_validationBrush, null, new Rect(
                    textView.Bounds.Width - width - 2.5, y,
                    width, 2.5
                ));
            }
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
        /// Default validation brush
        /// </summary>
        private Brush _validationBrush = ResourceLocator.GetResource<SolidColorBrush>("ErrorBrush") ?? new SolidColorBrush(Colors.Red);

        /// <summary>
        /// Underlying layer
        /// </summary>
        public KnownLayer Layer { get; }
    }
}
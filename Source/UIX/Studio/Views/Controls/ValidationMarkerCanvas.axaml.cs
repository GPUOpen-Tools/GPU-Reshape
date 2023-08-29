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
using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using AvaloniaEdit.Rendering;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.Views.Controls
{
    public partial class ValidationMarkerCanvas : UserControl
    {
        /// <summary>
        /// Text view the canvas tracks
        /// </summary>
        public TextView TextView
        {
            get => _textView;
            set
            {
                _textView = value;
                OnTextViewChanged();
            }
        }

        /// <summary>
        /// Detail command to propagate to the markers
        /// </summary>
        public ICommand? DetailCommand { get; set; }

        /// <summary>
        /// Current content view model
        /// </summary>
        public ITextualShaderContentViewModel? ShaderContentViewModel { get; set; }
        
        /// <summary>
        /// Constructor
        /// </summary>
        public ValidationMarkerCanvas()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Invoked on text view changes
        /// </summary>
        private void OnTextViewChanged()
        {
            // Bind events requiring layout changes
            TextView.ScrollOffsetChanged += (_, _) => UpdateLayout();
            TextView.DocumentChanged += (_, _) => UpdateLayout();
            TextView.EffectiveViewportChanged += (_, _) => UpdateLayout();
        }

        /// <summary>
        /// Add a new validation object
        /// </summary>
        public void Add(ValidationObject validationObject)
        {
            Add(new []{ validationObject });
        }

        /// <summary>
        /// Add a range of validation objects
        /// </summary>
        public void Add(IEnumerable<ValidationObject> objects)
        {
            // All changed views
            List<ValidationMarkerView> updatedViews = new();

            // Process all objects
            foreach (ValidationObject validationObject in objects)
            {
                // Ignore objects with no segments
                if (validationObject.Segment == null)
                {
                    continue;
                }

                // Already mapped?
                if (_viewLookup.ContainsKey(validationObject))
                {
                    return;
                }
                
                // Get the owning marker view
                ValidationMarkerView markerView = GetMarkerView(validationObject.Segment.Location);

                // Create association to the marker
                _viewLookup.Add(validationObject, markerView);

                // Add validation boject
                markerView.ViewModel.Objects.Add(validationObject);

                // Set first selection if needed
                if (markerView.ViewModel.SelectedObject == null)
                {
                    markerView.ViewModel.SelectedObject = validationObject;
                }

                // Mark the marker as updated
                updatedViews.Add(markerView);
            }

            // Any layouts changed?
            if (updatedViews.Count > 0)
            {
                UpdateLayout(updatedViews);
            }
        }

        /// <summary>
        /// Remove a validation object
        /// </summary>
        public void Remove(ValidationObject validationObject)
        {
            // Ignore objects with no segments
            if (validationObject.Segment == null)
            {
                return;
            }
            
            // Not part of any marker?
            if (!_viewLookup.ContainsKey(validationObject))
            {
                return;
            }

            // Get the marker view
            ValidationMarkerView view = _viewLookup[validationObject];

            // Remove the object association
            _viewLookup.Remove(validationObject);

            // Remove the validation object from its marker
            view.ViewModel.Objects.Remove(validationObject);

            // No objects left?
            if (view.ViewModel.Objects.Count == 0)
            {
                // Remove marker entry
                _markers.Remove(view);
                
                // Remove line lookup
                _lineMarkers.Remove(GetMarkerKey(validationObject.Segment.Location.FileUID, view.ViewModel.SourceLine));
                
                // Remove the control
                MarkerGrid.Children.Remove(view);
            }
            
            // Invalidate selected object
            if (view.ViewModel.SelectedObject == validationObject)
            {
                view.ViewModel.SelectedObject = view.ViewModel.Objects[0];
            }
        }

        /// <summary>
        /// Update the whole layout
        /// </summary>
        public void UpdateLayout()
        {
            UpdateLayout(_markers);
        }

        /// <summary>
        /// Update layouts for a set of views
        /// </summary>
        public void UpdateLayout(IEnumerable<ValidationMarkerView> views)
        {
            // get current text view bounds
            int firstLine = TextView.GetDocumentLineByVisualTop(TextView.ScrollOffset.Y).LineNumber;
            int lastLine = TextView.GetDocumentLineByVisualTop(TextView.ScrollOffset.Y + TextView.Height).LineNumber;
            
            // Process all views
            foreach (ValidationMarkerView view in views)
            {
                // Get view model
                var viewModel = (ValidationMarkerViewModel)view.DataContext!;

                // Document line starts from 1
                int documentLine = viewModel.SourceLine + 1;

                // View outside the current region?
                if (documentLine < firstLine || documentLine > lastLine)
                {
                    view.IsVisible = false;
                    continue;
                }
                
                // View invalid or rejected by the content view model?
                if (viewModel.Objects.Count == 0 || !(ShaderContentViewModel?.IsObjectVisible(viewModel.Objects[0]) ?? false))
                {
                    view.IsVisible = false;
                    continue;
                }

                // Mark as visible
                view.IsVisible = true;

                // Update layout
                UpdateObject(view);
            }
        }

        /// <summary>
        /// Update a single object
        /// </summary>
        private void UpdateObject(ValidationMarkerView view)
        {
            //Get view model
            var viewModel = (ValidationMarkerViewModel)view.DataContext!;
            
            // Get the effective vertical offset
            double top = TextView.GetVisualTopByDocumentLine(viewModel.SourceLine + 1) - TextView.ScrollOffset.Y;
                
            // Set new margin
            view.Margin = new Thickness(
                0,
                top - 1,
                0,
                0
            );
        }

        /// <summary>
        /// Get the marker view for a shader location
        /// </summary>
        private ValidationMarkerView GetMarkerView(ShaderLocation location)
        {
            // Always operate on the transformed line
            int line = ShaderContentViewModel?.TransformLine(location) ?? 0;
            
            // Compose the key
            UInt64 key = GetMarkerKey(location.FileUID, line);
            
            // New key entirely?
            if (!_lineMarkers.ContainsKey(key))
            {
                // Create marker view model
                ValidationMarkerViewModel viewModel = new()
                {
                    SourceLine = line,
                    DetailCommand = DetailCommand,
                    ShaderContentViewModel = ShaderContentViewModel
                };

                // Create marker view
                ValidationMarkerView view = new()
                {
                    DataContext = viewModel,
                    MaxHeight = TextView.DefaultLineHeight
                };
                
                // Track marker
                _markers.Add(view);
                
                // Create marker association
                _lineMarkers[key] = view;

                // Add marker to visual grid
                MarkerGrid.Children.Add(view);
            }

            // OK
            return _lineMarkers[key];
        }

        /// <summary>
        /// Compose a marker key
        /// </summary>
        private UInt64 GetMarkerKey(int fileUID, int line)
        {
            return ((UInt64)fileUID << 32) | (uint)line;
        }

        /// <summary>
        /// All markers
        /// </summary>
        private List<ValidationMarkerView> _markers = new();

        /// <summary>
        /// Keys to marker view associations
        /// </summary>
        private Dictionary<UInt64, ValidationMarkerView> _lineMarkers = new();

        /// <summary>
        /// Validation objects to marker view associations
        /// </summary>
        private Dictionary<ValidationObject, ValidationMarkerView> _viewLookup = new();

        /// <summary>
        /// Internal text view state
        /// </summary>
        private TextView _textView;
    }
}
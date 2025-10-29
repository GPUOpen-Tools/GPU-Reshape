// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
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
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using AvaloniaEdit.Rendering;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.Views.Controls
{
    public partial class SourceObjectMarkerCanvas : UserControl
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
        /// Current content view model
        /// </summary>
        public ITextualContent? ShaderContentViewModel { get; set; }
        
        /// <summary>
        /// Constructor
        /// </summary>
        public SourceObjectMarkerCanvas()
        {
            InitializeComponent();

            // Bind view model
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<SourceObjectMarkerCanvasViewModel>()
                .WhereNotNull()
                .Subscribe(vm =>
                {
                    // Bind all source objects
                    vm.SourceObjects.ToObservableChangeSet()
                        .AsObservableList()
                        .Connect()
                        .OnItemAdded(OnItemAdded)
                        .OnItemRemoved(OnItemRemoved)
                        .Subscribe();
                });
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
        /// Add a new source object
        /// </summary>
        private void OnItemAdded(ITextualSourceObject sourceObject)
        {
            OnItemAdded(new []{ sourceObject });
        }

        /// <summary>
        /// Add a range of source objects
        /// </summary>
        private void OnItemAdded(IEnumerable<ITextualSourceObject> objects)
        {
            // All changed views
            List<TextualSourceMarkerView> updatedViews = new();

            // Process all objects
            foreach (ITextualSourceObject sourceObject in objects)
            {
                // Ignore objects with no segments
                if (sourceObject.Segment == null)
                {
                    continue;
                }

                // Already mapped?
                if (_viewLookup.ContainsKey(sourceObject))
                {
                    return;
                }
                
                // Get the owning marker view
                TextualSourceMarkerView markerView = GetMarkerView(sourceObject.Segment.Location);

                // Create association to the marker
                _viewLookup.Add(sourceObject, markerView);

                // Find the category
                TextualSourceObjectMarkerCategoryViewModel category = FindOrAddCategory(markerView, sourceObject.OverlayCategory);

                // Add source object
                category.Objects.Add(sourceObject);

                // Set first selection if needed
                if (category.SelectedObject == null)
                {
                    category.SelectedObject = sourceObject;
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
        /// Try to find a category, or create it
        /// </summary>
        private TextualSourceObjectMarkerCategoryViewModel FindOrAddCategory(TextualSourceMarkerView view, string category)
        {
            TextualSourceObjectMarkerCategoryViewModel? categoryViewModel = view.ViewModel.CategoryObjects.FirstOrDefault(x => x.Category == category);
            if (categoryViewModel == null)
            {
                // Not found, create it
                categoryViewModel = new TextualSourceObjectMarkerCategoryViewModel()
                {
                    Category = category,
                    ShaderContentViewModel = view.ViewModel.ShaderContentViewModel
                };
                
                view.ViewModel.CategoryObjects.Add(categoryViewModel);
            }

            return categoryViewModel;
        }

        /// <summary>
        /// Remove a source object
        /// </summary>
        private void OnItemRemoved(ITextualSourceObject sourceObject)
        {
            // Ignore objects with no segments
            if (sourceObject.Segment == null)
            {
                return;
            }
            
            // Not part of any marker?
            if (!_viewLookup.ContainsKey(sourceObject))
            {
                return;
            }

            // Get the marker view
            TextualSourceMarkerView view = _viewLookup[sourceObject];

            // Remove the object association
            _viewLookup.Remove(sourceObject);

            // Must exist
            if (view.ViewModel.CategoryObjects.FirstOrDefault(x => x.Category == sourceObject.OverlayCategory) is not { } categoryViewModel)
            {
                return;
            }
            
            // Remove the source object from its marker
            categoryViewModel.Objects.Remove(sourceObject);
            
            // Is the category empty?
            if (categoryViewModel.Objects.Count == 0)
            {
                view.ViewModel.CategoryObjects.Remove(categoryViewModel);
            }
            else
            {
                // Invalidate selected object
                if (categoryViewModel.SelectedObject == sourceObject)
                {
                    categoryViewModel.SelectedObject = categoryViewModel.Objects[0];
                }
            }

            // No categories left?
            if (view.ViewModel.CategoryObjects.Count == 0)
            {
                // Remove marker entry
                _markers.Remove(view);
                
                // Remove line lookup
                _lineMarkers.Remove(GetMarkerKey(sourceObject.Segment.Location.FileUID, view.ViewModel.SourceLine));
                
                // Remove the control
                MarkerGrid.Children.Remove(view);
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
        public void UpdateLayout(IEnumerable<TextualSourceMarkerView> views)
        {
            // get current text view bounds
            int firstLine = TextView.GetDocumentLineByVisualTop(TextView.ScrollOffset.Y).LineNumber;
            int lastLine = TextView.GetDocumentLineByVisualTop(TextView.ScrollOffset.Y + TextView.Height).LineNumber;
            
            // Process all views
            foreach (TextualSourceMarkerView view in views)
            {
                // Get view model
                var viewModel = (TextualSourceObjectMarkerViewModel)view.DataContext!;

                // Document line starts from 1
                int documentLine = viewModel.SourceLine + 1;

                // View outside the current region?
                if (documentLine < firstLine || documentLine > lastLine)
                {
                    view.IsVisible = false;
                    continue;
                }
                
                // Check if we have a single visible object
                bool hasAnyVisibleObject = false;
                foreach (TextualSourceObjectMarkerCategoryViewModel categoryViewModel in viewModel.CategoryObjects)
                {
                    foreach (ITextualSourceObject sourceObject in categoryViewModel.Objects)
                    {
                        if (sourceObject.Segment != null && (ShaderContentViewModel?.IsLocationVisible(sourceObject.Segment.Location) ?? false))
                        {
                            hasAnyVisibleObject = true;
                            break;
                        }
                    }
                }
                
                // View invalid or rejected by the content view model?
                if (!hasAnyVisibleObject)
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
        private void UpdateObject(TextualSourceMarkerView view)
        {
            //Get view model
            var viewModel = (TextualSourceObjectMarkerViewModel)view.DataContext!;
            
            // Get the effective vertical offset
            double top = TextView.GetVisualTopByDocumentLine(viewModel.SourceLine + 1) - TextView.ScrollOffset.Y;
                
            // Set new margin
            view.Margin = new Thickness(
                0,
                top,
                0,
                0
            );
        }

        /// <summary>
        /// Get the marker view for a shader location
        /// </summary>
        private TextualSourceMarkerView GetMarkerView(ShaderLocation location)
        {
            // Always operate on the transformed line
            int line = ShaderContentViewModel?.TransformLine(location) ?? 0;
            
            // Compose the key
            UInt64 key = GetMarkerKey(location.FileUID, line);
            
            // New key entirely?
            if (!_lineMarkers.ContainsKey(key))
            {
                // Create marker view model
                TextualSourceObjectMarkerViewModel viewModel = new()
                {
                    SourceLine = line,
                    DetailCommand = ((SourceObjectMarkerCanvasViewModel)DataContext!).DetailCommand,
                    ShaderContentViewModel = ShaderContentViewModel
                };

                // Create marker view
                TextualSourceMarkerView view = new()
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
        private List<TextualSourceMarkerView> _markers = new();

        /// <summary>
        /// Keys to marker view associations
        /// </summary>
        private Dictionary<UInt64, TextualSourceMarkerView> _lineMarkers = new();

        /// <summary>
        /// Source objects to marker view associations
        /// </summary>
        private Dictionary<ITextualSourceObject, TextualSourceMarkerView> _viewLookup = new();

        /// <summary>
        /// Internal text view state
        /// </summary>
        private TextView _textView;
    }
}
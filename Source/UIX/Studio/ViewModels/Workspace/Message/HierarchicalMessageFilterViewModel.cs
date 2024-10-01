﻿// 
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
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using System.Reactive.Disposables;
using Avalonia.Media;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.ViewModels.IL;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.ValueConverters;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;

namespace Studio.ViewModels.Workspace.Message
{
    public class HierarchicalMessageFilterViewModel : ReactiveObject
    {
        /// <summary>
        /// The root category item
        /// </summary>
        public ObservableCategoryItem Root { get; } = new();

        /// <summary>
        /// Optional, query filtering
        /// </summary>
        public HierarchicalMessageQueryFilterViewModel? QueryFilterViewModel
        {
            get => _queryFilterViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _queryFilterViewModel, value);
                OnQueryFilterViewModelChanged();
            }
        }

        /// <summary>
        /// Virtualized hierarchy
        /// </summary>
        public HierarchicalMessageVirtualizationViewModel Virtualization { get; } = new();

        /// <summary>
        /// Workspace collection property
        /// </summary>
        public IPropertyViewModel? PropertyViewModel
        {
            get => _propertyViewModel;
            set => this.RaiseAndSetIfChanged(ref _propertyViewModel, value);
        }

        /// <summary>
        /// Current hierarchical mode mask
        /// </summary>
        public HierarchicalMode Mode
        {
            get => _mode;
            set
            {
                this.RaiseAndSetIfChanged(ref _mode, value);
                ResummarizeValidationObjects();
            }
        }

        /// <summary>
        /// Current severity mask
        /// </summary>
        public ValidationSeverity Severity
        {
            get => _severity;
            set
            {
                this.RaiseAndSetIfChanged(ref _severity, value);
                ResummarizeValidationObjects();
            }
        }
        
        /// <summary>
        /// Hide all validation objects with missing symbols?
        /// </summary>
        public bool HideMissingSymbols
        {
            get => _hideMissingSymbols;
            set
            {
                this.RaiseAndSetIfChanged(ref _hideMissingSymbols, value);
                ResummarizeValidationObjects();
            }
        }

        public HierarchicalMessageFilterViewModel()
        {
            // Virtualize the base range
            Virtualization.Virtualize(Root);
        }

        /// <summary>
        /// Bind this filter to a source
        /// </summary>
        public void Bind(ObservableCollection<ValidationObject> source)
        {
            // Keep it around for re-summarization
            _source = source;
            
            // Bind objects for filtering
            source
                .ToObservableChangeSet()
                .OnItemAdded(OnValidationItemAdded)
                .OnItemRemoved(OnValidationItemRemoved)
                .Subscribe();
        }

        /// <summary>
        /// Invoked on filter changes
        /// </summary>
        private void OnQueryFilterViewModelChanged()
        {
            // Resummarize on query changes
            QueryFilterViewModel?
                .WhenAnyValue(x => x.QueryViewModel)
                .Subscribe(_ => ResummarizeValidationObjects());
        }

        /// <summary>
        /// Expand all items
        /// </summary>
        public void Expand()
        {
            SetExpandedState(Root, true);
        }

        /// <summary>
        /// Collapse all items
        /// </summary>
        public void Collapse()
        {
            SetExpandedState(Root, false);
        }

        /// <summary>
        /// Set the expanded state for an item and its children
        /// </summary>
        private void SetExpandedState(IObservableTreeItem item, bool state)
        {
            if (item != Root)
            {
                item.IsExpanded = state;
            }

            item.Items.ForEach(x => SetExpandedState(x, state));
        }

        /// <summary>
        /// Invoked when an object was added
        /// </summary>
        private void OnValidationItemAdded(ValidationObject obj)
        {
            FilterValidationObject(obj);
        }

        /// <summary>
        /// Invoked when an object was removed
        /// </summary>
        private void OnValidationItemRemoved(ValidationObject obj)
        {
            RemoveValidationObject(obj);
        }

        /// <summary>
        /// Clear all filters
        /// </summary>
        public void Clear()
        {
            // Remove all bindings
            _filterDisposable.Clear();
            
            // Clear the virtualization, avoids needless reactions
            Virtualization.Clear();
            
            // Remove items and associations
            Root.Items.Clear();
            _objects.Clear();
            _parents.Clear();
        }

        /// <summary>
        /// Re-filter all validation objects
        /// Generally speaking this shouldn't be needed with correct bindings
        /// </summary>
        public void ResummarizeValidationObjects()
        {
            Clear();

            // Filter all items again
            _source?.ForEach(FilterValidationObject);

            // Virtualize the range again
            Virtualization.Virtualize(Root);
        }
        
        /// <summary>
        /// Filter a validation object
        /// </summary>
        private void FilterValidationObject(ValidationObject obj)
        {
            // Excluded by severity?
            if (!Severity.HasFlag(obj.Severity))
            {
                return;
            }
            
            // Excluded by missing symbols?
            if (_hideMissingSymbols && string.IsNullOrEmpty(obj.Extract))
            {
                return;
            }
            
            // Passed
            FilterCategorizedValidationObject(obj);
        }

        /// <summary>
        /// Remove a validation object entirely
        /// </summary>
        private void RemoveValidationObject(ValidationObject obj)
        {
            // Association may not exist if it was merged
            if (_objects.GetOrNull(obj) is not { } item)
            {
                return;
            }

            // Remove from parent
            IObservableTreeItem parent = _parents[item];
            parent.Items.Remove(item);
            
            // Remove associations
            _objects.Remove(obj, item);
            _parents.Remove(item);
            
            // Parent may now be empty
            Prune(parent);
        }

        /// <summary>
        /// Refilter a validation object
        /// </summary>
        private void RefilterValidationObject(ValidationObject obj)
        {
            RemoveValidationObject(obj);
            FilterValidationObject(obj);
        }
        
        /// <summary>
        /// Filter a validation object and create its categorization
        /// </summary>
        private void FilterCategorizedValidationObject(ValidationObject obj)
        {
            // Always start from root
            IObservableTreeItem item = Root;
            
            // Create observable, guaranteed to exist now
            ObservableMessageItem observable = new()
            {
                ValidationObject = obj,
                ShaderViewModel = ResolveShaderViewModel(obj)
            };

            // Create the decorations
            observable.FilenameDecoration = CompositeFilename(obj, observable);
            observable.ExtractDecoration = CompositeExtract(obj, observable);

            // Nested by type?
            if (_mode.HasFlag(HierarchicalMode.FilterByType))
            {
                item = GetOrCreateTypeCategory(item, obj, observable) ?? item;
            }
            
            // Nested by shader?
            if (_mode.HasFlag(HierarchicalMode.FilterByShader))
            {
                item = GetOrCreateShaderCategory(item, obj, observable) ?? item;
            }

            // If at root, categorization failed
            if (item == Root && _mode != HierarchicalMode.None)
            {
                item = GetOrCreateNoCategory();
            }

            // Check if the query matches the object
            if (!IsQueryMatch(item, observable))
            {
                Prune(item);
                return;
            }
            
            // If there's already a matching message, ignore this
            // Effectively merging it
            if (FindMergeCandidate(item, observable) is {})
            {
                Prune(item);
                return;
            }
            
            // Create lookups
            _objects.Add(obj, observable);
            _parents.Add(observable, item);
            
            // OK
            item.Items.Add(observable);
        }

        /// <summary>
        /// Prune a tree
        /// </summary>
        private void Prune(IObservableTreeItem item)
        {
            // Ignore roots
            if (item == Root || item.Items.Count > 0)
            {
                return;
            }

            // Parent must exist
            IObservableTreeItem parent = _parents[item];
            
            // Remove from parent
            parent.Items.Remove(item);
            _parents.Remove(item);
            
            // Parent itself may also need pruning
            Prune(parent);
        }

        /// <summary>
        /// Check if a query matches an observable
        /// </summary>
        private bool IsQueryMatch(IObservableTreeItem item, ObservableMessageItem observable)
        {
            // No filter?
            if (QueryFilterViewModel is not { QueryViewModel: {} query })
            {
                return true;
            }

            // Assume matching
            bool match = true;

            // Test general query
            if (!string.IsNullOrWhiteSpace(query.GeneralQuery))
            {
                // First, match the observable decorations
                match = observable.FilenameDecoration.Contains(query.GeneralQuery, StringComparison.InvariantCultureIgnoreCase) ||
                        (observable.ValidationObject?.Content.Contains(query.GeneralQuery, StringComparison.InvariantCultureIgnoreCase) ?? false) ||
                        observable.ExtractDecoration.Contains(query.GeneralQuery, StringComparison.InvariantCultureIgnoreCase);
                
                // Then, match the parent tree
                // If any of the parent exists, so must the child
                match = match || IsQueryGeneralMatch(item, query.GeneralQuery);
            }

            // Test shader
            if (!string.IsNullOrWhiteSpace(query.Shader))
            {
                match = match && observable.FilenameDecoration.Contains(query.Shader, StringComparison.InvariantCultureIgnoreCase);
            }

            // Test message
            if (!string.IsNullOrWhiteSpace(query.Message) && observable.ValidationObject != null)
            {
                match = match && observable.ValidationObject.Content.Contains(query.Message, StringComparison.InvariantCultureIgnoreCase);
            }

            // Test code
            if (!string.IsNullOrWhiteSpace(query.Code))
            {
                match = match && observable.ExtractDecoration.Contains(query.Code, StringComparison.InvariantCultureIgnoreCase);
            }

            // OK
            return match;
        }

        /// <summary>
        /// Check if there's a general match for an item tree
        /// </summary>
        private bool IsQueryGeneralMatch(IObservableTreeItem item, string query)
        {
            // Check the general text
            bool match = item.Text.Contains(query, StringComparison.InvariantCultureIgnoreCase);

            // Check parents
            if (_parents.TryGetValue(item, out IObservableTreeItem? parent))
            {
                match = match || IsQueryGeneralMatch(parent, query);
            }

            // OK
            return match;
        }

        /// <summary>
        /// Check if an item has a matching message
        /// </summary>
        private ObservableMessageItem? FindMergeCandidate(IObservableTreeItem item, ObservableMessageItem observable)
        {
            foreach (IObservableTreeItem observableTreeItem in item.Items)
            {
                if (observableTreeItem is not ObservableMessageItem messageItem)
                {
                    continue;
                }
                
                // Only merge from same shader
                if (messageItem.ShaderViewModel != observable.ShaderViewModel)
                {
                    continue;
                }

                // Only merge matching messages
                if (messageItem.ValidationObject?.Content != observable.ValidationObject?.Content)
                {
                    continue;
                }

                // Do a rough match between the decorations
                // This should guarantee some "uniqueness"
                if (messageItem.ExtractDecoration != observable.ExtractDecoration ||
                    messageItem.FilenameDecoration != observable.FilenameDecoration)
                {
                    continue;
                }

                // Matching item
                return messageItem;
            }

            // No match found
            return null;
        }

        /// <summary>
        /// Composite a decorated filename
        /// </summary>
        private string CompositeFilename(ValidationObject obj, ObservableMessageItem observable)
        {
            // Valid filename?
            // May "fail" with whitespaces
            if (!string.IsNullOrWhiteSpace(observable.ShaderViewModel?.Filename))
            {
                return observable.ShaderViewModel.Filename;
            }

            // Shader may have been lost
            if (observable.ShaderViewModel == null)
            {
                return "Shader Not Found";
            }
            
            // Report by GUID
            return $"Shader {observable.ShaderViewModel?.GUID}";
        }

        /// <summary>
        /// Composite a decorated extract
        /// </summary>
        private string CompositeExtract(ValidationObject obj, ObservableMessageItem observable)
        {
            // Valid extract?
            // May "fail" with whitespaces
            if (!string.IsNullOrWhiteSpace(observable.ValidationObject?.Extract))
            {
                return observable.ValidationObject.Extract;
            }

            // This shouldn't happen
            if (observable.ShaderViewModel == null || obj.Segment == null)
            {
                return "[Null]";
            }
            
            // Inform the code pooler
            _shaderCodeService?.EnqueueShaderIL(observable.ShaderViewModel);
            
            // The object does not have a valid extract, enqueue the program
            if (observable.ShaderViewModel.Program == null)
            {
                observable.ShaderViewModel
                    .WhenAnyValue(x => x.Program)
                    .WhereNotNull()
                    .Subscribe(_ => RefilterValidationObject(obj))
                    .DisposeWith(_filterDisposable);
                
                return "Assembling...";
            }

            // Try to assemble it
            string? assembled = AssemblerUtils.AssembleSingle(observable.ShaderViewModel.Program, obj.Segment.Location);
            return assembled ?? "[Failed to assemble]";
        }

        /// <summary>
        /// Get the uncategorized item
        /// </summary>
        private IObservableTreeItem GetOrCreateNoCategory()
        {
            return GetCategory(Root, "Uncategorized") ?? AddCategory(Root, new ObservableCategoryItem()
            {
                Text = "Uncategorized",
                Icon = _noCategoryIcon
            });
        }

        /// <summary>
        /// Create a validation type categorization
        /// </summary>
        private IObservableTreeItem? GetOrCreateTypeCategory(IObservableTreeItem item, ValidationObject obj, ObservableMessageItem observable)
        {
            // Create category
            return GetCategory(item, obj.Content) ?? AddCategory(item, new ObservableCategoryItem()
            {
                Text = obj.Content,
                Icon = _validationTypeConverter.Convert(obj.Severity, typeof(Geometry), null, CultureInfo.InvariantCulture) as StreamGeometry,
                StatusColor = _validationTypeConverter.Convert(obj.Severity, typeof(IBrush), null, CultureInfo.InvariantCulture) as IBrush
            });
        }

        /// <summary>
        /// Create a shader (view model) categorization
        /// </summary>
        private IObservableTreeItem? GetOrCreateShaderCategory(IObservableTreeItem item, ValidationObject obj, ObservableMessageItem observable)
        {
            // No shader? Ignore it for now
            if (observable.ShaderViewModel == null)
            {
                return null;
            }

            // Final header
            string header;
            
            // We're cataloging on the filename, if not available wait on it
            if (string.IsNullOrEmpty(observable.ShaderViewModel.Filename))
            {
                observable.ShaderViewModel
                    .WhenAnyValue(x => x.Filename)
                    .WhereNotNullOrEmpty()
                    .Subscribe(_ => RefilterValidationObject(obj))
                    .DisposeWith(_filterDisposable);
                
                header = $"Shader {observable.ShaderViewModel.GUID}";
            }
            else
            {
                header = $"{observable.ShaderViewModel.Filename} - Shader {observable.ShaderViewModel.GUID}";
            }

            // Create category
            return GetCategory(item, header) ?? AddCategory(item, new ObservableShaderCategoryItem()
            {
                Text = header,
                ShaderViewModel = observable.ShaderViewModel,
                Icon = _shaderCategoryIcon
            });
        }

        /// <summary>
        /// Resolve the shader associated with a validation object
        /// </summary>
        private ShaderViewModel? ResolveShaderViewModel(ValidationObject obj)
        {
            // Lazy get services
            _shaderCollectionViewModel ??= _propertyViewModel?.GetProperty<IShaderCollectionViewModel>();
            _shaderCodeService         ??= _propertyViewModel?.GetService<IShaderCodeService>();

            // If no segment, wait on it
            if (obj.Segment == null)
            {
                obj.WhenAnyValue(x => x.Segment)
                    .WhereNotNull()
                    .Subscribe(_ => RefilterValidationObject(obj))
                    .DisposeWith(_filterDisposable);
                
                return null;
            }

            // Missing shader?
            if (_shaderCollectionViewModel?.GetOrAddShader(obj.Segment.Location.SGUID) is not {} shaderViewModel)
            {
                // This shouldn't happen, it's erroneous, so don't enqueue anything
                return null;
            }
            
            // Inform the code pooler
            _shaderCodeService?.EnqueueShaderContents(shaderViewModel);

            // All ready!
            return shaderViewModel;
        }

        /// <summary>
        /// Get an existing category from an item
        /// </summary>
        private ObservableCategoryItem? GetCategory(IObservableTreeItem item, string content)
        {
            return item.Items.FirstOrDefault(x => x.Text == content) as ObservableCategoryItem;
        }

        /// <summary>
        /// Add a new category to an item
        /// Just a helper function
        /// </summary>
        private ObservableCategoryItem AddCategory(IObservableTreeItem item, ObservableCategoryItem observableCategory)
        {
            item.Items.Add(observableCategory);
            _parents.Add(observableCategory, item);
            return observableCategory;
        }

        /// <summary>
        /// Internal hierarhical mask
        /// </summary>
        private HierarchicalMode _mode = HierarchicalMode.FilterByType | HierarchicalMode.FilterByShader;

        /// <summary>
        /// Internal missing symbol state
        /// </summary>
        private bool _hideMissingSymbols = false;
        
        /// <summary>
        /// Internal bidirectional object association
        /// </summary>
        private ObjectDictionary<ValidationObject, ObservableMessageItem> _objects = new();

        /// <summary>
        /// Internal parent association
        /// </summary>
        private Dictionary<IObservableTreeItem, IObservableTreeItem> _parents = new();

        /// <summary>
        /// Internal workspace collection property
        /// </summary>
        private IPropertyViewModel? _propertyViewModel;

        /// <summary>
        /// Workspace shader collection
        /// </summary>
        private IShaderCollectionViewModel? _shaderCollectionViewModel;
        
        /// <summary>
        /// Workspace code pooling service
        /// </summary>
        private IShaderCodeService? _shaderCodeService;

        /// <summary>
        /// Icon used for shaders
        /// </summary>
        private StreamGeometry? _shaderCategoryIcon = ResourceLocator.GetIcon("ToolShaderTree");

        /// <summary>
        /// Icon used for uncategorized data
        /// </summary>
        private StreamGeometry? _noCategoryIcon = ResourceLocator.GetIcon("Question");

        /// <summary>
        /// Internal source collection
        /// </summary>
        private ObservableCollection<ValidationObject>? _source;
        
        /// <summary>
        /// Type converter for grouped typing
        /// </summary>
        private ValidationSeverityToIconConverter _validationTypeConverter = new();

        /// <summary>
        /// Shared disposer when resummarizing
        /// </summary>
        private CompositeDisposable _filterDisposable = new();

        /// <summary>
        /// Internal query filter view model
        /// </summary>
        private HierarchicalMessageQueryFilterViewModel? _queryFilterViewModel;
        
        /// <summary>
        /// Internal severity mask
        /// </summary>
        private ValidationSeverity _severity = ValidationSeverity.All;
    }   
}
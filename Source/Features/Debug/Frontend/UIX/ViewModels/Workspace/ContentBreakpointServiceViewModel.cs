using System;
using System.Collections.Generic;
using System.Reactive.Disposables;
using DynamicData;
using DynamicData.Binding;
using GRS.Features.Debug.UIX.ViewModels;
using GRS.Features.Debug.UIX.ViewModels.Utils;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;

namespace GRS.Features.Debug.UIX.Workspace;

public class ContentBreakpointServiceViewModel : ReactiveObject, IDestructableObject
{
    /// <summary>
    /// Content we're binding for
    /// </summary>
    public ITextualContent Content
    {
        get => _content;
        set => this.RaiseAndSetIfChanged(ref _content, value);
    }

    /// <summary>
    /// Collection we're binding from
    /// </summary>
    public BreakpointCollectionViewModel BreakpointCollectionViewModel
    {
        get => _breakpointCollectionViewModel;
        set => this.RaiseAndSetIfChanged(ref _breakpointCollectionViewModel, value);
    }

    /// <summary>
    /// Bind all breakpoints
    /// </summary>
    public void Bind()
    {
        _breakpointCollectionViewModel.SourceBindings
            .ToObservableChangeSet()
            .OnItemAdded(OnSourceAdded)
            .OnItemRemoved(OnSourceRemoved)
            .Subscribe();
        
        _breakpointCollectionViewModel.TextualBindings
            .ToObservableChangeSet()
            .OnItemAdded(OnTextualAdded)
            .OnItemRemoved(OnBindingRemoved)
            .Subscribe();
    }

    /// <summary>
    /// Invoked on item adds
    /// </summary>
    private void OnSourceAdded(BreakpointViewModelSourceBinding binding)
    {
        BreakpointMappingUtils.SubscribeInstructionLineMapping(Content, binding.BreakpointViewModel.Disposable, binding.Source.Mapping, associationViewModel =>
        {
            // Create source key
            var key = Tuple.Create(
                binding.BreakpointViewModel,
                associationViewModel.Location!.Value.Line,
                associationViewModel.Location!.Value.Column
            );

            // Ignore source-wise duplicates
            if (!_sourceBindingSet.Add(key))
            {
                return;
            }

            // Create object
            BreakpointSourceObject sourceObject = new()
            {
                Content = "Breakpoint",
                DetailViewModel = binding.BreakpointViewModel,
                Segment = new ShaderSourceSegment
                {
                    Location = associationViewModel.Location!.Value
                }
            };

            // Register source object
            Content.MarkerCanvasViewModel.SourceObjects.Add(sourceObject);
            
            // Remove on breakpoint disposing
            binding.BreakpointViewModel.Disposable.Add(Disposable.Create(() =>
            {
                Content.MarkerCanvasViewModel.SourceObjects.Remove(sourceObject);
            }));
            
            // Always select by default
            Content.SelectedTextualSourceObject = sourceObject;
            Content.MarkerCanvasViewModel.DetailCommand?.Execute(sourceObject);
        });
    }

    /// <summary>
    /// Invoked on item removed
    /// </summary>
    private void OnSourceRemoved(BreakpointViewModelSourceBinding binding)
    {
        // Slow remove
        _sourceBindingSet.RemoveWhere(x => x.Item1 == binding.BreakpointViewModel);

        // Remove source object
        if (_sourceObjects.TryGetValue(binding, out BreakpointSourceObject? sourceObject))
        {
            Content.MarkerCanvasViewModel.SourceObjects.Remove(sourceObject);
            _sourceObjects.Remove(binding);
        }
    }

    /// <summary>
    /// Invoked on item adds
    /// </summary>
    private void OnTextualAdded(BreakpointViewModelTextualBinding binding)
    {
        // Create textual key
        var key = Tuple.Create(
            binding.BreakpointViewModel,
            binding.LineBase0
        );

        // Ignore textual-wise duplicates
        if (!_textualBindingSet.Add(key))
        {
            return;
        }

        // Create object
        BreakpointSourceObject sourceObject = new()
        {
            Content = "Breakpoint",
            DetailViewModel = binding.BreakpointViewModel,
            Segment = new ShaderSourceSegment
            {
                Location = new ShaderLocation()
                {
                    Line = binding.LineBase0
                }
            }
        };

        // Register source object
        Content.MarkerCanvasViewModel.SourceObjects.Add(sourceObject);
        
        // Remove on breakpoint disposing
        binding.BreakpointViewModel.Disposable.Add(Disposable.Create(() =>
        {
            Content.MarkerCanvasViewModel.SourceObjects.Remove(sourceObject);
        }));
        
        // Always select by default
        Content.SelectedTextualSourceObject = sourceObject;
        Content.MarkerCanvasViewModel.DetailCommand?.Execute(sourceObject);
    }

    /// <summary>
    /// Invoked on item removed
    /// </summary>
    private void OnBindingRemoved(BreakpointViewModelTextualBinding binding)
    {
        // Slow remove
        _sourceBindingSet.RemoveWhere(x => x.Item1 == binding.BreakpointViewModel);

        // Remove source object
        if (_sourceObjects.TryGetValue(binding, out BreakpointSourceObject? sourceObject))
        {
            Content.MarkerCanvasViewModel.SourceObjects.Remove(sourceObject);
            _sourceObjects.Remove(binding);
        }
    }

    /// <summary>
    /// Internal content
    /// </summary>
    private ITextualContent _content;
    
    /// <summary>
    /// Internal breakpoint collection
    /// </summary>
    private BreakpointCollectionViewModel _breakpointCollectionViewModel;

    /// <summary>
    /// All source binding sets
    /// </summary>
    private HashSet<Tuple<BreakpointViewModel, int, int>> _sourceBindingSet = new();

    /// <summary>
    /// All textual binding sets
    /// </summary>
    private HashSet<Tuple<BreakpointViewModel, int>> _textualBindingSet = new();

    /// <summary>
    /// All source objects
    /// </summary>
    private Dictionary<object, BreakpointSourceObject> _sourceObjects = new();
}

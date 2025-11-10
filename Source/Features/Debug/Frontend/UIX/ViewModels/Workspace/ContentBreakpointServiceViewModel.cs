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
        _breakpointCollectionViewModel.Bindings
            .ToObservableChangeSet()
            .OnItemAdded(OnAdded)
            .OnItemRemoved(OnRemoved)
            .Subscribe();
    }

    /// <summary>
    /// Invoked on item adds
    /// </summary>
    private void OnAdded(BreakpointViewModelBinding binding)
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
    private void OnRemoved(BreakpointViewModelBinding binding)
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
    /// All source objects
    /// </summary>
    private Dictionary<BreakpointViewModelBinding, BreakpointSourceObject> _sourceObjects = new();
}

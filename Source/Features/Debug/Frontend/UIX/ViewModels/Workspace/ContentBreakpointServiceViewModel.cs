using System;
using System.Collections.Generic;
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
        BreakpointMappingUtils.SubscribeInstructionLineMapping(Content, binding.Source.Mapping, associationViewModel =>
        {
            // Create source key
            var key = Tuple.Create(
                binding.BreakpointViewModel,
                associationViewModel.Location!.Value.Line,
                associationViewModel.Location!.Value.Column
            );

            // Ignore source-wise duplicates
            if (!_sourceObjects.Add(key))
            {
                return;
            }

            // Create and register source object
            Content.MarkerCanvasViewModel.SourceObjects.Add(binding.BreakpointViewModel.TextualSourceObject = new BreakpointSourceObject()
            {
                Content = "Breakpoint",
                DetailViewModel = binding.BreakpointViewModel,
                Segment = new ShaderSourceSegment
                {
                    Location = associationViewModel.Location!.Value
                }
            });
            
            // Always select by default
            Content.SelectedTextualSourceObject = binding.BreakpointViewModel.TextualSourceObject;
            Content.MarkerCanvasViewModel.DetailCommand?.Execute(binding.BreakpointViewModel.TextualSourceObject);
        });
    }

    /// <summary>
    /// Invoked on item removed
    /// </summary>
    private void OnRemoved(BreakpointViewModelBinding binding)
    {
        // Slow remove
        _sourceObjects.RemoveWhere(x => x.Item1 == binding.BreakpointViewModel);
        
        // Remove source object
        Content.MarkerCanvasViewModel.SourceObjects.Remove(binding.BreakpointViewModel.TextualSourceObject);
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
    /// All source-wise objects
    /// </summary>
    private HashSet<Tuple<BreakpointViewModel, int, int>> _sourceObjects = new();
}

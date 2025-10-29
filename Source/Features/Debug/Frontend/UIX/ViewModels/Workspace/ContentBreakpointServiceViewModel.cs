using System;
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
        _breakpointCollectionViewModel.Breakpoints
            .ToObservableChangeSet()
            .OnItemAdded(OnAdded)
            .OnItemRemoved(OnRemoved)
            .Subscribe();
    }

    /// <summary>
    /// Invoked on item adds
    /// </summary>
    private void OnAdded(BreakpointViewModel breakpointViewModel)
    {
        BreakpointMappingUtils.SubscribeInstructionLineMapping(Content, breakpointViewModel.SourceBinding!.Mapping, associationViewModel =>
        {
            // Create and register source object
            Content.MarkerCanvasViewModel.SourceObjects.Add(breakpointViewModel.TextualSourceObject = new BreakpointSourceObject()
            {
                Content = "Breakpoint",
                DetailViewModel = breakpointViewModel,
                Segment = new ShaderSourceSegment
                {
                    Location = associationViewModel.Location!.Value
                }
            });
            
            // Always select by default
            Content.SelectedTextualSourceObject = breakpointViewModel.TextualSourceObject;
            Content.MarkerCanvasViewModel.DetailCommand?.Execute(breakpointViewModel.TextualSourceObject);
        });
    }

    /// <summary>
    /// Invoked on item removed
    /// </summary>
    private void OnRemoved(BreakpointViewModel breakpointViewModel)
    {
        // Remove source object
        Content.MarkerCanvasViewModel.SourceObjects.Remove(breakpointViewModel.TextualSourceObject);
    }

    /// <summary>
    /// Internal content
    /// </summary>
    private ITextualContent _content;
    
    /// <summary>
    /// Internal breakpoint collection
    /// </summary>
    private BreakpointCollectionViewModel _breakpointCollectionViewModel;
}

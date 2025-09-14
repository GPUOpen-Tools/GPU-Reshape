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

public class ShaderContentBreakpointServiceViewModel : ReactiveObject, IDestructableObject
{
    /// <summary>
    /// Content we're binding for
    /// </summary>
    public ITextualShaderContentViewModel ContentViewModel
    {
        get => _contentViewModel;
        set => this.RaiseAndSetIfChanged(ref _contentViewModel, value);
    }

    /// <summary>
    /// Collection we're binding from
    /// </summary>
    public ShaderBreakpointCollectionViewModel BreakpointCollectionViewModel
    {
        get => _breakpointCollectionViewModel;
        set => this.RaiseAndSetIfChanged(ref _breakpointCollectionViewModel, value);
    }

    /// <summary>
    /// Bind all breakpoints
    /// </summary>
    public void Bind()
    {
        _breakpointCollectionViewModel. Breakpoints
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
        BreakpointMappingUtils.SubscribeInstructionLineMapping(ContentViewModel, breakpointViewModel.SourceBinding!.Mapping, associationViewModel =>
        {
            // Create and register source object
            ContentViewModel.MarkerCanvasViewModel.SourceObjects.Add(breakpointViewModel.TextualSourceObject = new BreakpointSourceObject()
            {
                Content = "Breakpoint",
                DetailViewModel = breakpointViewModel,
                Segment = new ShaderSourceSegment
                {
                    Location = associationViewModel.Location!.Value
                }
            });
            
            // Always select by default
            ContentViewModel.SelectedTextualSourceObject = breakpointViewModel.TextualSourceObject;
            ContentViewModel.MarkerCanvasViewModel.DetailCommand?.Execute(breakpointViewModel.TextualSourceObject);
        });
    }

    /// <summary>
    /// Invoked on item removed
    /// </summary>
    private void OnRemoved(BreakpointViewModel breakpointViewModel)
    {
        // Remove source object
        ContentViewModel.MarkerCanvasViewModel.SourceObjects.Remove(breakpointViewModel.TextualSourceObject);
    }

    /// <summary>
    /// Internal content
    /// </summary>
    private ITextualShaderContentViewModel _contentViewModel;
    
    /// <summary>
    /// Internal breakpoint collection
    /// </summary>
    private ShaderBreakpointCollectionViewModel _breakpointCollectionViewModel;
}

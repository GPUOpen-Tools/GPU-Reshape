using GRS.Features.Debug.UIX.ViewModels.Controls;
using ReactiveUI;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointVariableViewModel : ReactiveObject
{
    /// <summary>
    /// Currently selected breakpoint
    /// </summary>
    public BreakpointViewModel BreakpointViewModel
    {
        get => _breakpointViewModel;
        set => this.RaiseAndSetIfChanged(ref _breakpointViewModel, value);
    }
    
    /// <summary>
    /// Internal view model
    /// </summary>
    private BreakpointViewModel _breakpointViewModel;
}

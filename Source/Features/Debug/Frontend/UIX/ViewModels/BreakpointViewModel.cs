using ReactiveUI;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointViewModel : ReactiveObject
{
    /// <summary>
    /// Currently assigned view model
    /// </summary>
    public IBreakpointDisplayViewModel? DisplayViewModel
    {
        get => _displayViewModel;
        set => this.RaiseAndSetIfChanged(ref _displayViewModel, value);
    }

    /// <summary>
    /// The source location of the breakpoint
    /// </summary>
    public SourceBinding? SourceBinding
    {
        get => _sourceBinding;
        set => this.RaiseAndSetIfChanged(ref _sourceBinding, value);
    }
    
    /// <summary>
    /// Statically allocated UID
    /// </summary>
    public uint UID { get; set; }
    
    /// <summary>
    /// Type of this breakpoint
    /// </summary>
    public BreakpointType Type { get; set; }
    
    /// <summary>
    /// Internal view model
    /// </summary>
    private IBreakpointDisplayViewModel? _displayViewModel;
    
    /// <summary>
    /// Internal source binding
    /// </summary>
    private SourceBinding? _sourceBinding;
}

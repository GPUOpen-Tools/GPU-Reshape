using ReactiveUI;
using GRS.Features.Debug.UIX.Settings;
using Studio.Services;

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
    /// Current streaming size
    /// </summary>
    public ulong StreamSize { get; set; }

    /// <summary>
    /// Type of this breakpoint
    /// </summary>
    public BreakpointDisplayMode DisplayMode { get; set; }

    public BreakpointViewModel()
    {
        StreamSize = GetDefaultSize();
    }

    /// <summary>
    /// Get the default streaming size of a breakpoint
    /// </summary>
    public static uint GetDefaultSize()
    {
        // TODO[dbg]: Ugly, have a standardized unit somewhere
        
        if (ServiceRegistry.Get<ISettingsService>()?.Get<DebugSettingViewModel>() is { } debugSettingViewModel)
        {
            return debugSettingViewModel.DefaultBreakpointMemoryMb * 1000000;
        }

        return 32000000;
    }

    /// <summary>
    /// Internal view model
    /// </summary>
    private IBreakpointDisplayViewModel? _displayViewModel;

    /// <summary>
    /// Internal source binding
    /// </summary>
    private SourceBinding? _sourceBinding;
}
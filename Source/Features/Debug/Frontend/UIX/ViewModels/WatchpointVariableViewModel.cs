using System;
using ReactiveUI;

namespace GRS.Features.Debug.UIX.ViewModels;

public class WatchpointVariableViewModel : ReactiveObject
{
    /// <summary>
    /// Currently selected watchpoint
    /// </summary>
    public WatchpointViewModel WatchpointViewModel
    {
        get => _watchpointViewModel;
        set => this.RaiseAndSetIfChanged(ref _watchpointViewModel, value);
    }

    /// <summary>
    /// Currently selected variable
    /// </summary>
    public WatchpointDebugVariable? WatchpointDebugVariable
    {
        get => _watchpointDebugVariable;
        set => this.RaiseAndSetIfChanged(ref _watchpointDebugVariable, value);
    }

    public WatchpointVariableViewModel()
    {
        // Bind value changes
        this.WhenAnyValue(x => x.WatchpointViewModel)
            .WhereNotNull()
            .Subscribe(x =>
            {
                x.WhenAnyValue(y => y.SelectedDebugValue)
                    .Subscribe(OnValueChanged);
            });
    }

    /// <summary>
    /// Invoked on value changes
    /// </summary>
    private void OnValueChanged(WatchpointDebugValue? value)
    {
        WatchpointDebugVariable = null;
            
        if (value == null)
        {
            return;
        }
        
        foreach (WatchpointDebugVariable variable in WatchpointViewModel.DebugVariables)
        {
            if (IsValueOf(variable.Value, value))
            {
                WatchpointDebugVariable = variable;
                break;
            }
        }
    }

    /// <summary>
    /// Check of a value is that of a variable
    /// </summary>
    private bool IsValueOf(WatchpointDebugValue value, WatchpointDebugValue query)
    {
        if (value == query)
        {
            return true;
        }

        foreach (WatchpointDebugValue childValue in value.Values)
        {
            if (IsValueOf(childValue, query))
            {
                return true;
            }
        }

        return false;
    }
    
    /// <summary>
    /// Internal view model
    /// </summary>
    private WatchpointViewModel _watchpointViewModel;

    /// <summary>
    /// Internal variable
    /// </summary>
    private WatchpointDebugVariable? _watchpointDebugVariable;
}

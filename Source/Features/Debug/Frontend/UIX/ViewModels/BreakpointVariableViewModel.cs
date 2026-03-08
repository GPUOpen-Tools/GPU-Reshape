using System;
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
    /// Currently selected variable
    /// </summary>
    public BreakpointDebugVariable? BreakpointDebugVariable
    {
        get => _breakpointDebugVariable;
        set => this.RaiseAndSetIfChanged(ref _breakpointDebugVariable, value);
    }

    public BreakpointVariableViewModel()
    {
        // Bind value changes
        this.WhenAnyValue(x => x.BreakpointViewModel)
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
    private void OnValueChanged(BreakpointDebugValue? value)
    {
        BreakpointDebugVariable = null;
            
        if (value == null)
        {
            return;
        }
        
        foreach (BreakpointDebugVariable variable in BreakpointViewModel.DebugVariables)
        {
            if (IsValueOf(variable.Value, value))
            {
                BreakpointDebugVariable = variable;
                break;
            }
        }
    }

    /// <summary>
    /// Check of a value is that of a variable
    /// </summary>
    private bool IsValueOf(BreakpointDebugValue value, BreakpointDebugValue query)
    {
        if (value == query)
        {
            return true;
        }

        foreach (BreakpointDebugValue childValue in value.Values)
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
    private BreakpointViewModel _breakpointViewModel;

    /// <summary>
    /// Internal variable
    /// </summary>
    private BreakpointDebugVariable? _breakpointDebugVariable;
}

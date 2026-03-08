using System;
using System.Linq;
using System.Reactive.Disposables;
using Avalonia.Media;
using DynamicData;
using DynamicData.Binding;
using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.ViewModels.Controls;
using ReactiveUI;
using Runtime.ViewModels.Tools;
using Studio;
using Studio.Models.IL;
using Studio.Services;
using Studio.ViewModels.Controls;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointStackViewModel : ToolViewModel
{
    /// <summary>
    /// Tooling icon
    /// </summary>
    public override StreamGeometry? Icon => ResourceLocator.GetIcon("Stack");

    /// <summary>
    /// Tooling tip
    /// </summary>
    public override string? ToolTip => "Breakpoint stack variables";
    
    /// <summary>
    /// Root item
    /// </summary>
    public BreakpointStackTreeItemViewModel Root { get; } = new()
    {
        IsExpanded = true
    };

    /// <summary>
    /// Currently selected item
    /// </summary>
    public BreakpointStackTreeItemViewModel? SelectedStackTreeItemViewModel
    {
        get => _selectedStackTreeItemViewModel;
        set => this.RaiseAndSetIfChanged(ref _selectedStackTreeItemViewModel, value);
    }
        
    /// <summary>
    /// Is the help message visible?
    /// </summary>
    public bool IsHelpVisible => Root.Items.Count == 0;

    public BreakpointStackViewModel()
    {
        Title = "Locals";

        // Subscribe to all breakpoints
        ServiceRegistry.Get<BreakpointService>()?
            .WhenAnyValue(x => x.SelectedBreakpointViewModel)
            .Subscribe(OnBreakpointBound);
        
        // Bind to selections
        this.WhenAnyValue(x => x.SelectedStackTreeItemViewModel)
            .WhereNotNull()
            .Subscribe(OnSelectionChanged);
    }

    /// <summary>
    /// Invoked on selection changes
    /// </summary>
    private void OnSelectionChanged(BreakpointStackTreeItemViewModel item)
    {
        if (_selectedBreakpointViewModel == null)
        {
            return;
        }
        
        // Assign the selection
        _selectedBreakpointViewModel.SelectedDebugValue = GetValueFromViewModel(item);
    }

    /// <summary>
    /// Get the underlying value binding
    /// </summary>
    private BreakpointDebugValue? GetValueFromViewModel(BreakpointStackTreeItemViewModel item)
    {
        switch (item.ViewModel)
        {
            default:
                return null;
            case BreakpointDebugVariable var:
                return var.Value;
            case BreakpointDebugValue value:
                return value;
        }
    }

    /// <summary>
    /// Invoked on breakpoint changes
    /// </summary>
    private void OnBreakpointBound(BreakpointViewModel? breakpointViewModel)
    {
        _disposable.Clear();
        Root.Items.Clear();

        // Null is valid
        _selectedBreakpointViewModel = breakpointViewModel;
        if (breakpointViewModel == null)
        {
            return;
        }
        
        // Bind all variables
        breakpointViewModel.DebugVariables
            .ToObservableChangeSet()
            .OnItemAdded(OnVariableAdded)
            .OnItemRemoved(OnVariableRemoved)
            .Subscribe()
            .DisposeWith(_disposable);
        
        // Bind on selection changes
        breakpointViewModel
            .WhenAnyValue(x => x.SelectedDebugValue)
            .Subscribe(value =>
            {
                if (value == null)
                {
                    SelectedStackTreeItemViewModel = null;
                    return;
                }
                
                // Just find the first item
                SelectedStackTreeItemViewModel = FindItemViewModel(Root, value);
            })
            .DisposeWith(_disposable);
    }

    /// <summary>
    /// Find the owning view model
    /// </summary>
    private BreakpointStackTreeItemViewModel? FindItemViewModel(BreakpointStackTreeItemViewModel item, BreakpointDebugValue value)
    {
        if (GetValueFromViewModel(item) == value)
        {
            return item;
        }

        foreach (IObservableTreeItem child in item.Items)
        {
            if (FindItemViewModel((BreakpointStackTreeItemViewModel)child, value) is { } childResult)
            {
                return childResult;
            }
        }

        return null;
    }

    /// <summary>
    /// Invoked on variable additions
    /// </summary>
    private void OnVariableAdded(BreakpointDebugVariable variable)
    {
        var item = new BreakpointStackTreeItemViewModel();
        CreateValueTree(item, variable.Value);
        Root.Items.Add(item);

        // Replace with variable defaults
        item.ViewModel = variable;
        item.Text = variable.Decoration;

        UpdateHelp();
    }

    /// <summary>
    /// Create the full recursive value tree
    /// </summary>
    private void CreateValueTree(BreakpointStackTreeItemViewModel item, BreakpointDebugValue variableValue)
    {
        // Assign info
        item.Text = variableValue.Decoration;
        item.ViewModel = variableValue;

        // Create children
        foreach (BreakpointDebugValue childValue in variableValue.Values)
        {
            var childItem = new BreakpointStackTreeItemViewModel();
            CreateValueTree(childItem, childValue);
            item.Items.Add(childItem);
        }

        // Summarize reconstruction states
        if (variableValue.Values.Length > 0)
        {
            if (item.Items.All(x => ((BreakpointStackTreeItemViewModel)x).ReconstructionState == BreakpointValueReconstructionState.Full))
            {
                item.ReconstructionState = BreakpointValueReconstructionState.Full;
            }
            else
            {
                item.ReconstructionState = item.Items.Any(x => ((BreakpointStackTreeItemViewModel)x).ReconstructionState == BreakpointValueReconstructionState.Full)
                    ? BreakpointValueReconstructionState.Partial
                    : BreakpointValueReconstructionState.None;
            }
        }
        else
        {
            item.ReconstructionState = variableValue.HasReconstruction
                ? BreakpointValueReconstructionState.Full
                : BreakpointValueReconstructionState.None;
        }
        
        // Only auto-expand structural types
        item.IsExpanded = variableValue.Type.Kind == TypeKind.Struct;
    }

    /// <summary>
    /// Invoked on variable removals
    /// </summary>
    private void OnVariableRemoved(BreakpointDebugVariable variable)
    {
        // Try to find it
        if (Root.Items.FirstOrDefault(x => x.ViewModel == variable) is { } item)
        {
            Root.Items.Remove(item);
            UpdateHelp();
        }
    }

    /// <summary>
    /// Update help visibility
    /// </summary>
    private void UpdateHelp()
    {
        this.RaisePropertyChanged(nameof(IsHelpVisible));
    }

    /// <summary>
    /// Shared event disposable
    /// </summary>
    private CompositeDisposable _disposable = new();

    /// <summary>
    /// Internal selection
    /// </summary>
    private BreakpointStackTreeItemViewModel? _selectedStackTreeItemViewModel;

    /// <summary>
    /// Internal breakpoint
    /// </summary>
    private BreakpointViewModel? _selectedBreakpointViewModel;
}

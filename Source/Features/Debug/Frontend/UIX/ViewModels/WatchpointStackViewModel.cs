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

public class WatchpointStackViewModel : ToolViewModel
{
    /// <summary>
    /// Tooling icon
    /// </summary>
    public override StreamGeometry? Icon => ResourceLocator.GetIcon("Stack");

    /// <summary>
    /// Tooling tip
    /// </summary>
    public override string? ToolTip => "Watchpoint stack variables";
    
    /// <summary>
    /// Root item
    /// </summary>
    public WatchpointStackTreeItemViewModel Root { get; } = new()
    {
        IsExpanded = true
    };

    /// <summary>
    /// Currently selected item
    /// </summary>
    public WatchpointStackTreeItemViewModel? SelectedStackTreeItemViewModel
    {
        get => _selectedStackTreeItemViewModel;
        set => this.RaiseAndSetIfChanged(ref _selectedStackTreeItemViewModel, value);
    }
        
    /// <summary>
    /// Is the help message visible?
    /// </summary>
    public bool IsHelpVisible => Root.Items.Count == 0;

    public WatchpointStackViewModel()
    {
        Title = "Locals";

        // Subscribe to all watchpoints
        ServiceRegistry.Get<WatchpointService>()?
            .WhenAnyValue(x => x.SelectedWatchpointViewModel)
            .Subscribe(OnWatchpointBound);
        
        // Bind to selections
        this.WhenAnyValue(x => x.SelectedStackTreeItemViewModel)
            .WhereNotNull()
            .Subscribe(OnSelectionChanged);
    }

    /// <summary>
    /// Invoked on selection changes
    /// </summary>
    private void OnSelectionChanged(WatchpointStackTreeItemViewModel item)
    {
        if (_selectedWatchpointViewModel == null)
        {
            return;
        }
        
        // Assign the selection
        _selectedWatchpointViewModel.SelectedDebugValue = GetValueFromViewModel(item);
    }

    /// <summary>
    /// Get the underlying value binding
    /// </summary>
    private WatchpointDebugValue? GetValueFromViewModel(WatchpointStackTreeItemViewModel item)
    {
        switch (item.ViewModel)
        {
            default:
                return null;
            case WatchpointDebugVariable var:
                return var.Value;
            case WatchpointDebugValue value:
                return value;
        }
    }

    /// <summary>
    /// Invoked on watchpoint changes
    /// </summary>
    private void OnWatchpointBound(WatchpointViewModel? watchpointViewModel)
    {
        _disposable.Clear();
        Root.Items.Clear();

        // Null is valid
        _selectedWatchpointViewModel = watchpointViewModel;
        if (watchpointViewModel == null)
        {
            return;
        }
        
        // Bind all variables
        watchpointViewModel.DebugVariables
            .ToObservableChangeSet()
            .OnItemAdded(OnVariableAdded)
            .OnItemRemoved(OnVariableRemoved)
            .Subscribe()
            .DisposeWith(_disposable);
        
        // Bind on selection changes
        watchpointViewModel
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
    private WatchpointStackTreeItemViewModel? FindItemViewModel(WatchpointStackTreeItemViewModel item, WatchpointDebugValue value)
    {
        if (GetValueFromViewModel(item) == value)
        {
            return item;
        }

        foreach (IObservableTreeItem child in item.Items)
        {
            if (FindItemViewModel((WatchpointStackTreeItemViewModel)child, value) is { } childResult)
            {
                return childResult;
            }
        }

        return null;
    }

    /// <summary>
    /// Invoked on variable additions
    /// </summary>
    private void OnVariableAdded(WatchpointDebugVariable variable)
    {
        var item = new WatchpointStackTreeItemViewModel();
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
    private void CreateValueTree(WatchpointStackTreeItemViewModel item, WatchpointDebugValue variableValue)
    {
        // Assign info
        item.Text = variableValue.Decoration;
        item.ViewModel = variableValue;

        // Create children
        foreach (WatchpointDebugValue childValue in variableValue.Values)
        {
            var childItem = new WatchpointStackTreeItemViewModel();
            CreateValueTree(childItem, childValue);
            item.Items.Add(childItem);
        }

        // Summarize reconstruction states
        if (variableValue.Values.Length > 0)
        {
            if (item.Items.All(x => ((WatchpointStackTreeItemViewModel)x).ReconstructionState == WatchpointValueReconstructionState.Full))
            {
                item.ReconstructionState = WatchpointValueReconstructionState.Full;
            }
            else
            {
                item.ReconstructionState = item.Items.Any(x => ((WatchpointStackTreeItemViewModel)x).ReconstructionState == WatchpointValueReconstructionState.Full)
                    ? WatchpointValueReconstructionState.Partial
                    : WatchpointValueReconstructionState.None;
            }
        }
        else
        {
            item.ReconstructionState = variableValue.HasReconstruction
                ? WatchpointValueReconstructionState.Full
                : WatchpointValueReconstructionState.None;
        }
        
        // Only auto-expand structural types
        item.IsExpanded = variableValue.Type.Kind == TypeKind.Struct;
    }

    /// <summary>
    /// Invoked on variable removals
    /// </summary>
    private void OnVariableRemoved(WatchpointDebugVariable variable)
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
    private WatchpointStackTreeItemViewModel? _selectedStackTreeItemViewModel;

    /// <summary>
    /// Internal watchpoint
    /// </summary>
    private WatchpointViewModel? _selectedWatchpointViewModel;
}

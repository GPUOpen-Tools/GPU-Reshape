using ReactiveUI;
using Avalonia.Media;
using System.Collections.ObjectModel;
using GRS.Features.Debug.UIX.Models;
using Studio.ViewModels.Controls;

namespace GRS.Features.Debug.UIX.ViewModels.Controls;

public class WatchpointStackTreeItemViewModel : ReactiveObject, IObservableTreeItem
{
    /// <summary>
    /// Display text of this item
    /// </summary>
    public string Text
    {
        get { return _text; }
        set { this.RaiseAndSetIfChanged(ref _text, value); }
    }
        
    /// <summary>
    /// Hosted view model
    /// </summary>
    public object? ViewModel
    {
        get => _viewModel;
        set => this.RaiseAndSetIfChanged(ref _viewModel, value);
    }
    
    /// <summary>
    /// Assigned reconstruction state
    /// </summary>
    public WatchpointValueReconstructionState ReconstructionState { get; set; }
    
    /// <summary>
    /// Expansion state
    /// </summary>
    public bool IsExpanded
    {
        get { return _isExpanded; }
        set { this.RaiseAndSetIfChanged(ref _isExpanded, value); }
    }

    /// <summary>
    /// Foreground color of the item
    /// </summary>
    public IBrush? StatusColor
    {
        get => _statusColor;
        set => this.RaiseAndSetIfChanged(ref _statusColor, value);
    }
        
    /// <summary>
    /// All child items
    /// </summary>
    public ObservableCollection<IObservableTreeItem> Items { get; } = new();

    /// <summary>
    /// Internal text state
    /// </summary>
    private string _text = "WatchpointStackTreeItemViewModel";

    /// <summary>
    /// Internal status color
    /// </summary>
    private IBrush? _statusColor = Brushes.White;

    /// <summary>
    /// Internal object
    /// </summary>
    private object? _viewModel;
        
    /// <summary>
    /// Internal expansion state
    /// </summary>
    private bool _isExpanded = true;
}
using System.Collections.ObjectModel;
using Avalonia.Media;
using ReactiveUI;
using Studio.ViewModels.Controls;

namespace GRS.Features.Debug.UIX.ViewModels;

public class LooseTreeItemViewModel : ReactiveObject, IObservableTreeItem
{
    /// <summary>
    /// Text of this item
    /// </summary>
    public string Text { get; set; }
    
    /// <summary>
    /// Is this item expanded?
    /// </summary>
    public bool IsExpanded { get; set; } = true;
    
    /// <summary>
    /// Assigned view model, optional
    /// </summary>
    public object? ViewModel { get; set; }
    
    /// <summary>
    /// Status color
    /// </summary>
    public IBrush? StatusColor { get; set; }

    /// <summary>
    /// All children of this item
    /// </summary>
    public ObservableCollection<IObservableTreeItem> Items { get; set; } = new();
}

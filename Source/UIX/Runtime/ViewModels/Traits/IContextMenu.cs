namespace Runtime.ViewModels.Traits;

public interface IContextMenu
{
    /// <summary>
    /// Populate all view models for a given sender
    /// </summary>
    void PopulateViewModels(object? sender);
}

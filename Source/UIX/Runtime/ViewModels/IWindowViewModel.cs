namespace Studio.ViewModels;

public interface IWindowViewModel
{
    /// <summary>
    /// Close the window
    /// </summary>
    void Close();

    /// <summary>
    /// Is this window visible?
    /// </summary>
    bool IsVisible();
}

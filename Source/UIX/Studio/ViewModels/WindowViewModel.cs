using Avalonia.Controls;

namespace Studio.ViewModels;

public class WindowViewModel : IWindowViewModel
{
    /// <summary>
    /// Window being represented
    /// </summary>
    public Window? Window { get; set; }

    /// <summary>
    /// Invoked on close requests
    /// </summary>
    public void Close()
    {
        Window?.Close();
    }

    /// <summary>
    /// Is the window visible?
    /// </summary>
    public bool IsVisible()
    {
        return Window?.IsVisible ?? false;
    }
}
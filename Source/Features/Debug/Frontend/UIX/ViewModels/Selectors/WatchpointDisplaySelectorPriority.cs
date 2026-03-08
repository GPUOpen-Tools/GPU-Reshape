namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public static class WatchpointDisplaySelectorPriority
{
    /// <summary>
    /// The display doesn't support this format
    /// </summary>
    public static int? Unsupported = null;
    
    /// <summary>
    /// The display supports it, but not ideal
    /// </summary>
    public static int Supported = 0;
    
    /// <summary>
    /// The format is suited for this display
    /// </summary>
    public static int Preferred = 10;
    
    /// <summary>
    /// Fully ideal display model
    /// </summary>
    public static int Optimal = 100;
}
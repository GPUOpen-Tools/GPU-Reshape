namespace Studio.Views;

public enum ViewType
{
    /// <summary>
    /// Primary view, semantically undefined
    /// </summary>
    Primary,
    
    /// <summary>
    /// View for general configuration
    /// </summary>
    Config,
    
    /// <summary>
    /// Status for general configuration
    /// </summary>
    Status,
    
    /// <summary>
    /// Overlay for general configuration
    /// </summary>
    Overlay,
    
    /// <summary>
    /// Dedicated window
    /// </summary>
    Window
}
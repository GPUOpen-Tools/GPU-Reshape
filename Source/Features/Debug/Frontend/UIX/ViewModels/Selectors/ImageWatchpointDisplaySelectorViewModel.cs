using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.Settings;
using Message.CLR;
using Studio.Services;

namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public class ImageWatchpointDisplaySelectorViewModel : IWatchpointDisplaySelectorViewModel
{
    public int? GetPriority(DebugWatchpointStreamMessage message)
    {
        // Get the typed data
        var order       = (WatchpointDataOrder)message.dataOrder;
        var compression = (WatchpointCompression)message.dataCompression;

        // Loose data not supported
        if (order == WatchpointDataOrder.Loose)
        {
            return WatchpointDisplaySelectorPriority.Unsupported;
        }

        // Volumetric data isn't supported yet
        if (message.dataStaticDepth > 1)
        {
            return WatchpointDisplaySelectorPriority.Unsupported;
        }

        // Check if we're in the image size limits
        if (_debugSettings != null && (
            message.dataStaticWidth > _debugSettings.MaxWatchpointImageSizePerAxis ||
            message.dataStaticHeight > _debugSettings.MaxWatchpointImageSizePerAxis
        ))
        {
            return WatchpointDisplaySelectorPriority.Unsupported;
        }

        // If compressed, optimal
        if (compression == WatchpointCompression.FPUNorm8888)
        {
            return WatchpointDisplaySelectorPriority.Optimal;
        }
        
        // Otherwise, the preferred display mode
        return WatchpointDisplaySelectorPriority.Preferred;
    }

    /// <summary>
    /// Settings
    /// </summary>
    private static DebugSettingViewModel? _debugSettings = ServiceRegistry.Get<ISettingsService>()?.Get<DebugSettingViewModel>();
}
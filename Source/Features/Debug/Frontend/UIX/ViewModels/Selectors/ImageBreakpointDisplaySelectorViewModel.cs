using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.Settings;
using Message.CLR;
using Studio.Services;

namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public class ImageBreakpointDisplaySelectorViewModel : IBreakpointDisplaySelectorViewModel
{
    public int? GetPriority(DebugBreakpointStreamMessage message)
    {
        // Get the typed data
        var order       = (BreakpointDataOrder)message.dataOrder;
        var compression = (BreakpointCompression)message.dataCompression;

        // Loose data not supported
        if (order == BreakpointDataOrder.Loose)
        {
            return BreakpointDisplaySelectorPriority.Unsupported;
        }

        // Volumetric data isn't supported yet
        if (message.dataStaticDepth > 1)
        {
            return BreakpointDisplaySelectorPriority.Unsupported;
        }

        // Check if we're in the image size limits
        if (_debugSettings != null && (
            message.dataStaticWidth > _debugSettings.MaxBreakpointImageSizePerAxis ||
            message.dataStaticHeight > _debugSettings.MaxBreakpointImageSizePerAxis
        ))
        {
            return BreakpointDisplaySelectorPriority.Unsupported;
        }

        // If compressed, optimal
        if (compression == BreakpointCompression.FPUNorm8888)
        {
            return BreakpointDisplaySelectorPriority.Optimal;
        }
        
        // Otherwise, the preferred display mode
        return BreakpointDisplaySelectorPriority.Preferred;
    }

    /// <summary>
    /// Settings
    /// </summary>
    private static DebugSettingViewModel? _debugSettings = ServiceRegistry.Get<ISettingsService>()?.Get<DebugSettingViewModel>();
}
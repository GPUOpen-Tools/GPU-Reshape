using System.Collections.Generic;
using GRS.Features.Debug.UIX.ViewModels;
using Message.CLR;
using Studio.ViewModels.Workspace;

namespace GRS.Features.Debug.UIX.Workspace;

public class BreakpointRegistryService : IPropertyService
{
    /// <summary>
    /// Breakpoint uid lookup
    /// </summary>
    public Dictionary<uint, BreakpointViewModel> Lookup { get; } = new();
    
    /// <summary>
    /// Parent workspace
    /// </summary>
    public IWorkspaceViewModel WorkspaceViewModel { get; set; }

    /// <summary>
    /// Register a new breakpoint
    /// </summary>
    public void Register(BreakpointViewModel breakpointViewModel)
    {
        breakpointViewModel.UID = _allocationCounter++;
        Lookup.Add(breakpointViewModel.UID, breakpointViewModel);

        // Add breakpoint to the backend
        if (WorkspaceViewModel.Connection?.GetSharedBus() is { } bus)
        {
            var msg = bus.Add<RegisterDebugBreakpointMessage>();
            msg.uid = breakpointViewModel.UID;
            msg.type = (uint)breakpointViewModel.Type;
        }
    }
    
    /// <summary>
    /// Deregister a breakpoint
    /// </summary>
    /// <param name="breakpointViewModel"></param>
    public void Deregister(BreakpointViewModel breakpointViewModel)
    {
        Lookup.Remove(breakpointViewModel.UID);

        // Inform the backend that the breakpoint was removed
        if (WorkspaceViewModel.Connection?.GetSharedBus() is { } bus)
        {
            var msg = bus.Add<DeregisterDebugBreakpointMessage>();
            msg.uid = breakpointViewModel.UID;
        }
    }

    /// <summary>
    /// Monotonic breakpoint counter
    /// </summary>
    private uint _allocationCounter = 0;
}

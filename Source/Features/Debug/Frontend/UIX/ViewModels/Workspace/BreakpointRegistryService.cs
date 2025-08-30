using System.Collections.Generic;
using GRS.Features.Debug.UIX.ViewModels;
using Message.CLR;
using Studio.ViewModels.Workspace;

namespace GRS.Features.Debug.UIX.Workspace;

public class BreakpointRegistryService : IPropertyService
{
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

        lock (_lookup)
        {
            _lookup.Add(breakpointViewModel.UID, breakpointViewModel);
        }

        // Add breakpoint to the backend
        if (WorkspaceViewModel.Connection?.GetSharedBus() is { } bus)
        {
            var msg = bus.Add<RegisterDebugBreakpointMessage>();
            msg.uid = breakpointViewModel.UID;
            msg.streamSize = (uint)breakpointViewModel.StreamSize;
            msg.captureMode = (uint)breakpointViewModel.CaptureMode;
        }
    }
    
    /// <summary>
    /// Deregister a breakpoint
    /// </summary>
    /// <param name="breakpointViewModel"></param>
    public void Deregister(BreakpointViewModel breakpointViewModel)
    {
        lock (_lookup)
        {
            _lookup.Remove(breakpointViewModel.UID);
        }

        // Inform the backend that the breakpoint was removed
        if (WorkspaceViewModel.Connection?.GetSharedBus() is { } bus)
        {
            var msg = bus.Add<DeregisterDebugBreakpointMessage>();
            msg.uid = breakpointViewModel.UID;
        }
    }

    /// <summary>
    /// Reallocate the backing memory for a breakpoint
    /// </summary>
    /// <param name="breakpointViewModel"></param>
    public void Reallocate(BreakpointViewModel breakpointViewModel)
    {
        if (WorkspaceViewModel.Connection?.GetSharedBus() is { } bus)
        {
            var msg = bus.Add<ReallocateDebugBreakpointMessage>();
            msg.uid = breakpointViewModel.UID;
            msg.streamSize = (uint)breakpointViewModel.StreamSize;
        }
    }

    /// <summary>
    /// Get a breakpoint
    /// </summary>
    public BreakpointViewModel? GetBreakpoint(uint uid)
    {
        lock (_lookup)
        {
            return _lookup.GetValueOrDefault(uid);
        }
    }

    /// <summary>
    /// Monotonic breakpoint counter
    /// </summary>
    private uint _allocationCounter = 0;
    
    /// <summary>
    /// Breakpoint uid lookup
    /// </summary>
    private Dictionary<uint, BreakpointViewModel> _lookup = new();
}

using System;
using System.Collections.Generic;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using GRS.Features.Debug.UIX.ViewModels;
using Message.CLR;
using ReactiveUI;
using Studio.ViewModels.Workspace;

namespace GRS.Features.Debug.UIX.Workspace;

public class WatchpointRegistryService : IPropertyService
{
    /// <summary>
    /// Parent workspace
    /// </summary>
    public IWorkspaceViewModel WorkspaceViewModel { get; set; }

    /// <summary>
    /// Register a new watchpoint
    /// </summary>
    public void Register(WatchpointViewModel watchpointViewModel)
    {
        watchpointViewModel.UID = _allocationCounter++;

        lock (_lookup)
        {
            _lookup.Add(watchpointViewModel.UID, watchpointViewModel);
        }

        // Add watchpoint to the backend
        if (WorkspaceViewModel.Connection?.GetSharedBus() is { } bus)
        {
            var msg = bus.Add<RegisterDebugWatchpointMessage>();
            msg.uid = watchpointViewModel.UID;
            msg.streamSize = (uint)watchpointViewModel.StreamSize;
            msg.captureMode = (uint)watchpointViewModel.CaptureMode;
        }
        
        BindRegistrationProperties(watchpointViewModel);
    }

    /// <summary>
    /// Bind all properties
    /// </summary>
    private void BindRegistrationProperties(WatchpointViewModel watchpointViewModel)
    {
        // Bind capture re-registration
        watchpointViewModel
            .WhenAnyValue(x => x.CaptureMode)
            .Skip(1)
            .Subscribe(_ => Reregister(watchpointViewModel))
            .DisposeWith(watchpointViewModel.Disposable);
    }

    /// <summary>
    /// Reregister a watchpoint
    /// </summary>
    public void Reregister(WatchpointViewModel watchpointViewModel)
    {
        // Re-allocate the UID, avoids timing issues
        Deregister(watchpointViewModel);
        Register(watchpointViewModel);

        // Reinstrument with the new capture mode
        watchpointViewModel.EnqueueAllShaderParentBus();
    }
    
    /// <summary>
    /// Deregister a watchpoint
    /// </summary>
    public void Deregister(WatchpointViewModel watchpointViewModel)
    {
        lock (_lookup)
        {
            _lookup.Remove(watchpointViewModel.UID);
        }

        // Inform the backend that the watchpoint was removed
        if (WorkspaceViewModel.Connection?.GetSharedBus() is { } bus)
        {
            var msg = bus.Add<DeregisterDebugWatchpointMessage>();
            msg.uid = watchpointViewModel.UID;
        }
    }

    /// <summary>
    /// Reallocate the backing memory for a watchpoint
    /// </summary>
    public void Reallocate(WatchpointViewModel watchpointViewModel)
    {
        if (WorkspaceViewModel.Connection?.GetSharedBus() is { } bus)
        {
            var msg = bus.Add<ReallocateDebugWatchpointMessage>();
            msg.uid = watchpointViewModel.UID;
            msg.streamSize = (uint)watchpointViewModel.StreamSize;
        }
    }

    /// <summary>
    /// Get a watchpoint
    /// </summary>
    public WatchpointViewModel? GetWatchpoint(uint uid)
    {
        lock (_lookup)
        {
            return _lookup.GetValueOrDefault(uid);
        }
    }

    /// <summary>
    /// Monotonic watchpoint counter
    /// </summary>
    private uint _allocationCounter = 0;
    
    /// <summary>
    /// Watchpoint uid lookup
    /// </summary>
    private Dictionary<uint, WatchpointViewModel> _lookup = new();
}

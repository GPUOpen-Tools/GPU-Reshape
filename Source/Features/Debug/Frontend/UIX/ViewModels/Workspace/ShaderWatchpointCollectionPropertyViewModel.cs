using System;
using System.Text;
using DynamicData;
using DynamicData.Binding;
using GRS.Features.Debug.UIX.Models;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace GRS.Features.Debug.UIX.ViewModels;

public class ShaderWatchpointCollectionPropertyViewModel : BasePropertyViewModel, IInstrumentationProperty
{
    /// <summary>
    /// Feature info
    /// </summary>
    public FeatureInfo FeatureInfo { get; set; }

    /// <summary>
    /// The shader this collection represents
    /// </summary>
    public ShaderPropertyViewModel ShaderPropertyProperty
    {
        get => _shaderPropertyProperty;
        set
        {
            this.RaiseAndSetIfChanged(ref _shaderPropertyProperty, value);
            OnShaderChanged();
        }
    }

    /// <summary>
    /// The underlying collection we're mirroring
    /// </summary>
    public required WatchpointCollectionViewModel CollectionViewModel
    {
        get => _collectionViewModel;
        set => this.RaiseAndSetIfChanged(ref _collectionViewModel, value);
    }

    /// <summary>
    /// Constructor
    /// </summary>
    public ShaderWatchpointCollectionPropertyViewModel() : base("Watchpoint", PropertyVisibility.WorkspaceTool)
    {
        // Bind to collection
        this.WhenAnyValue(x => x.CollectionViewModel).WhereNotNull().Subscribe(x =>
        {
            x.SourceBindings.ToObservableChangeSet()
                .OnItemAdded(OnAdded)
                .OnItemRemoved(OnRemoved)
                .Subscribe();
        });
    }

    /// <summary>
    /// Invoked when the assigned shader changes
    /// </summary>
    private void OnShaderChanged()
    {
        
    }

    /// <summary>
    /// Invoked on watchpoint addition
    /// </summary>
    private void OnAdded(WatchpointViewModelSourceBinding obj)
    {
        ShaderPropertyProperty.EnqueueBus();
    }

    /// <summary>
    /// Invoked on watchpoint removal
    /// </summary>
    private void OnRemoved(WatchpointViewModelSourceBinding obj)
    {
        ShaderPropertyProperty.EnqueueBus();
    }

    /// <summary>
    /// Commit all watchpoint data
    /// </summary>
    public void Commit(InstrumentationState state)
    {
        state.FeatureBitMask |= FeatureInfo.FeatureBit;

        // Watchpoint stream
        ReadWriteMessageStream watchpointStream = new();
        
        // Store all watchpoints
        StaticMessageView<DebugWatchpointMessage, ReadWriteMessageStream> view = new(watchpointStream);
        foreach (WatchpointViewModelSourceBinding binding in CollectionViewModel.SourceBindings)
        {
            WatchpointConfig watchpointConfig = binding.WatchpointViewModel.GetWatchpointConfig();
            
            var watchpoint = view.Add();
            watchpoint.codeOffset = binding.Source!.Mapping.CodeOffset;
            watchpoint.uid = binding.WatchpointViewModel.UID;
            watchpoint.flags = (uint)watchpointConfig.Flags;
            watchpoint.variableHash = binding.WatchpointViewModel.SelectedDebugValue?.VariableHash ?? 0;
            watchpoint.valueId = binding.WatchpointViewModel.SelectedDebugValue?.ValueId ?? 0;
            watchpoint.markerHash32 = 0;

            // Hash marker if requested
            if (!string.IsNullOrEmpty(binding.WatchpointViewModel.Marker))
            {
                watchpoint.markerHash32 = System.IO.Hashing.Crc32.HashToUInt32(Encoding.ASCII.GetBytes(binding.WatchpointViewModel.Marker));
            }
        }

        // Create config
        var config = state.SpecializationStream.Add<DebugConfigMessage>(new DebugConfigMessage.AllocationInfo()
        {
            watchpointsByteSize = (ulong)watchpointStream.GetSpan().Length
        });
        
        // Store watchpoints
        config.watchpoints.Store(watchpointStream);
    }

    /// <summary>
    /// Internal shader view model
    /// </summary>
    private ShaderPropertyViewModel _shaderPropertyProperty;

    /// <summary>
    /// Internal collection
    /// </summary>
    private WatchpointCollectionViewModel _collectionViewModel;
}
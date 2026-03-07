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

public class ShaderBreakpointCollectionPropertyViewModel : BasePropertyViewModel, IInstrumentationProperty
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
    public required BreakpointCollectionViewModel CollectionViewModel
    {
        get => _collectionViewModel;
        set => this.RaiseAndSetIfChanged(ref _collectionViewModel, value);
    }

    /// <summary>
    /// Constructor
    /// </summary>
    public ShaderBreakpointCollectionPropertyViewModel() : base("Breakpoint", PropertyVisibility.WorkspaceTool)
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
    /// Invoked on breakpoint addition
    /// </summary>
    private void OnAdded(BreakpointViewModelSourceBinding obj)
    {
        ShaderPropertyProperty.EnqueueBus();
    }

    /// <summary>
    /// Invoked on breakpoint removal
    /// </summary>
    private void OnRemoved(BreakpointViewModelSourceBinding obj)
    {
        ShaderPropertyProperty.EnqueueBus();
    }

    /// <summary>
    /// Commit all breakpoint data
    /// </summary>
    public void Commit(InstrumentationState state)
    {
        state.FeatureBitMask |= FeatureInfo.FeatureBit;

        // Breakpoint stream
        ReadWriteMessageStream breakpointStream = new();
        
        // Store all breakpoints
        StaticMessageView<DebugBreakpointMessage, ReadWriteMessageStream> view = new(breakpointStream);
        foreach (BreakpointViewModelSourceBinding binding in CollectionViewModel.SourceBindings)
        {
            BreakpointConfig breakpointConfig = binding.BreakpointViewModel.GetBreakpointConfig();
            
            var breakpoint = view.Add();
            breakpoint.codeOffset = binding.Source!.Mapping.CodeOffset;
            breakpoint.uid = binding.BreakpointViewModel.UID;
            breakpoint.flags = (uint)breakpointConfig.Flags;
            breakpoint.variableId = binding.BreakpointViewModel.SelectedDebugValue?.VariableId ?? 0;
            breakpoint.valueId = binding.BreakpointViewModel.SelectedDebugValue?.ValueId ?? 0;
            breakpoint.markerHash32 = 0;

            // Hash marker if requested
            if (!string.IsNullOrEmpty(binding.BreakpointViewModel.Marker))
            {
                breakpoint.markerHash32 = System.IO.Hashing.Crc32.HashToUInt32(Encoding.ASCII.GetBytes(binding.BreakpointViewModel.Marker));
            }
        }

        // Create config
        var config = state.SpecializationStream.Add<DebugConfigMessage>(new DebugConfigMessage.AllocationInfo()
        {
            breakpointsByteSize = (ulong)breakpointStream.GetSpan().Length
        });
        
        // Store breakpoints
        config.breakpoints.Store(breakpointStream);
    }

    /// <summary>
    /// Internal shader view model
    /// </summary>
    private ShaderPropertyViewModel _shaderPropertyProperty;

    /// <summary>
    /// Internal collection
    /// </summary>
    private BreakpointCollectionViewModel _collectionViewModel;
}
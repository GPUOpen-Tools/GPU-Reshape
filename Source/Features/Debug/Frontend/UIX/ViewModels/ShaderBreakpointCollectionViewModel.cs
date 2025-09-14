using System;
using System.Collections.ObjectModel;
using System.Linq;
using DynamicData;
using DynamicData.Binding;
using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.Workspace;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace GRS.Features.Debug.UIX.ViewModels;

public class ShaderBreakpointCollectionViewModel : BasePropertyViewModel, IInstrumentationProperty
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
    /// All breakpoints for the shader
    /// </summary>
    public ObservableCollection<BreakpointViewModel> Breakpoints { get; } = new();

    /// <summary>
    /// Constructor
    /// </summary>
    public ShaderBreakpointCollectionViewModel() : base("Breakpoint", PropertyVisibility.WorkspaceTool)
    {
        Breakpoints.ToObservableChangeSet()
            .OnItemAdded(OnAdded)
            .OnItemRemoved(OnRemoved)
            .Subscribe();
    }

    /// <summary>
    /// Invoked when the assigned shader changes
    /// </summary>
    private void OnShaderChanged()
    {
        _breakpointRegistryService = ShaderPropertyProperty.GetWorkspaceCollection()?.GetService<BreakpointRegistryService>();
    }

    /// <summary>
    /// Invoked on breakpoint addition
    /// </summary>
    private void OnAdded(BreakpointViewModel obj)
    {
        _breakpointRegistryService?.Register(obj);
        ShaderPropertyProperty.EnqueueBus();
    }

    /// <summary>
    /// Invoked on breakpoint removal
    /// </summary>
    private void OnRemoved(BreakpointViewModel obj)
    {
        _breakpointRegistryService?.Deregister(obj);
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
        foreach (BreakpointViewModel breakpointViewModel in Breakpoints.Where(x => x.SourceBinding != null))
        {
            BreakpointConfig breakpointConfig = breakpointViewModel.GetBreakpointConfig();
            
            var breakpoint = view.Add();
            breakpoint.codeOffset = breakpointViewModel.SourceBinding!.Mapping.CodeOffset;
            breakpoint.uid = breakpointViewModel.UID;
            breakpoint.flags = (uint)breakpointConfig.Flags;
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
    /// Workspace breakpoint registry
    /// </summary>
    private BreakpointRegistryService? _breakpointRegistryService;
    
    /// <summary>
    /// Internal shader view model
    /// </summary>
    private ShaderPropertyViewModel _shaderPropertyProperty;
}
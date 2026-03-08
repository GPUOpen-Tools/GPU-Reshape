using System;
using System.Linq;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using DynamicData;
using DynamicData.Binding;
using GRS.Features.Debug.UIX.Models;
using ReactiveUI;
using Runtime.Utils.Workspace;
using Runtime.ViewModels.Shader;
using Studio;
using Studio.Models.IL;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;

namespace GRS.Features.Debug.UIX.ViewModels.Utils;

public static class WatchpointMappingUtils
{
    /// <summary>
    /// For a given association set, find the single representative instruction to instrument
    /// </summary>
    public static AssembledInstructionMapping? FindRepresentativeInstruction(ShaderViewModel shaderViewModel, ShaderInstructionAssociationViewModel associationViewModel)
    {
        // Get the instructions from the mappings
        MappedInstruction[] pairs = FindRepresentativeInstructionSet(shaderViewModel, associationViewModel);
        
        // Walk back
        for (int i = pairs.Length - 1; i >= 0; i--)
        {
            MappedInstruction instr = pairs[i];

            // Exclude some instructions
            switch (instr.Instruction.OpCode)
            {
                default:
                    return instr.Mapping;
                case OpCode.Return:
                    continue;
            }
        }

        // Nothing interesting
        return null;
    }
    
    /// <summary>
    /// Find all representative instructions
    /// </summary>
    public static MappedInstruction[] FindRepresentativeInstructionSet(ShaderViewModel shaderViewModel, ShaderInstructionAssociationViewModel associationViewModel)
    {
        return associationViewModel.Mappings
            .Select(x => new MappedInstruction{Mapping = x, Instruction = shaderViewModel.Program!.GetInstruction(x)!})
            .Where(x => x.Instruction != null)
            .ToArray()!;
    }

    /// <summary>
    /// Subscribe to a single line source mapping
    /// </summary>
    public static void SubscribeSourceLineMapping(ITextualContent content, CompositeDisposable disposable, int lineBase0, Action<ShaderViewModel, ShaderInstructionAssociationViewModel> action)
    {
        // Get the association view model
        if (content.TransformSourceLine(lineBase0) is not { } associationViewModel)
        {
            Logging.Error($"Failed to associate line {lineBase0} against the shader GUID");
            return;
        }

        // Subscribe to all future associations
        associationViewModel.Associations
            .ToObservableChangeSet()
            .OnItemAdded(pair =>
            {
                // Wait for population
                pair.Association
                    .WhenAnyValue(x => x.Populated)
                    .Where(x => x)
                    .Take(1)
                    .Subscribe(populated =>
                {
                    ShaderUtils.SubscribeDeferredProgram(content.PropertyCollection!, pair.ShaderViewModel, () =>
                    {
                        action(pair.ShaderViewModel, pair.Association);
                    });
                }).DisposeWith(disposable);
            })
            .Subscribe()
            .DisposeWith(disposable);
    }

    /// <summary>
    /// Subscribe to a many instruction mapping
    /// </summary>
    public static void SubscribeInstructionLineMapping(ITextualContent content, CompositeDisposable disposable, AssembledInstructionMapping mapping, Action<ShaderInstructionSourceAssociationViewModel> action)
    {
        // Get the association view model
        if (content.TransformInstructionLine(mapping) is not { } associationViewModel)
        {
            Logging.Error($"Failed to associate line {mapping} against the shader GUID");
            return;
        }

        // Subscribe to all future associations
        associationViewModel.Associations
            .ToObservableChangeSet()
            .OnItemAdded(pair =>
            {
                // Shared disposable
                CompositeDisposable disposable = new();

                // Wait for population
                pair.Association
                    .WhenAnyValue(x => x.Location)
                    .WhereNotNull()
                    .Take(1)
                    .Subscribe(_ =>
                {
                    ShaderUtils.SubscribeDeferredProgram(content.PropertyCollection!, pair.ShaderViewModel, () =>
                    {
                        action(pair.Association);
                    });
                }).DisposeWith(disposable);
            })
            .Subscribe()
            .DisposeWith(disposable);
    }
}

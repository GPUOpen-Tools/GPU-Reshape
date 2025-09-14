using DynamicData;
using GRS.Features.Debug.UIX.Models;
using Runtime.Models.Objects;
using Runtime.ViewModels.Shader;
using Runtime.ViewModels.Traits;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace GRS.Features.Debug.UIX.ViewModels.Utils;

public static class BreakpointUtils
{
    /// <summary>
    /// Add a new breakpoint from a given line
    /// </summary>
    public static void AddBreakpoint(ShaderBreakpointCollectionViewModel collection, ITextualShaderContentViewModel content, int lineBase0, BreakpointCaptureMode captureMode)
    {
        BreakpointMappingUtils.SubscribeSourceLineMapping(content, lineBase0, associationViewModel =>
        {
            AddBreakpoint(content, collection, associationViewModel, captureMode);
        });
    }
    
    /// <summary>
    /// Add a new breakpoint on a mapped instruction
    /// </summary>
    public static void AddBreakpoint(ITextualShaderContentViewModel content, ShaderBreakpointCollectionViewModel collection, ShaderInstructionAssociationViewModel associationViewModel, BreakpointCaptureMode captureMode)
    {
        // No relevant mappings
        if (associationViewModel.Mappings.Count == 0)
        {
            return;
        }
        
        // Just choose the first for now
        if (BreakpointMappingUtils.FindRepresentativeInstruction(content.ShaderViewModel!, associationViewModel) is not { } mapping)
        {
            return;
        }
        
        // Translate to segment
        ShaderSourceSegment segment = new()
        {
            Location = new ShaderLocation
            {
                BasicBlockId = mapping.BasicBlockId,
                InstructionIndex = mapping.InstructionIndex,
                FileUID = (int)ShaderLocation.InvalidFileUID
            }
        };
            
        // None found, add it
        collection.Breakpoints.Add(new BreakpointViewModel
        {
            ShaderProperty = collection.ShaderPropertyProperty,
            ShaderSourceSegment = segment,
            CaptureMode = captureMode,
            SourceBinding = new SourceBinding
            {
                Mapping = mapping,
                AssociationViewModel = associationViewModel
            }
        });
    }

    /// <summary>
    /// Get the breakpoint collection from a shader
    /// </summary>
    public static ShaderBreakpointCollectionViewModel? GetShaderBreakpointCollection(IPropertyViewModel propertyViewModel, ShaderViewModel shaderViewModel)
    {
        // TODO[dbg]: Standardize this, and only when breakpoints are added
        
        // Find the debug feature
        FeatureInfo? featureInfo = propertyViewModel
            .GetWorkspaceCollection()?
            .GetWorkspaceCollection()?
            .GetProperty<IFeatureCollectionViewModel>()?
            .GetFeature("Debug");

        // Must have shader collection
        var shaderCollectionViewModel = propertyViewModel.GetProperty<IShaderCollectionViewModel>();
        if (shaderCollectionViewModel == null)
        {
            return null;
        }

        // Find or create shader property
        var shaderPropertyViewModel = shaderCollectionViewModel.GetPropertyWhere<ShaderPropertyViewModel>(x => x.Shader.GUID == shaderViewModel.GUID);
        if (shaderPropertyViewModel == null)
        {
            shaderCollectionViewModel.Properties.Add(shaderPropertyViewModel = new ShaderPropertyViewModel
            {
                Parent = shaderCollectionViewModel,
                ConnectionViewModel = shaderCollectionViewModel.ConnectionViewModel,
                Shader = new ShaderIdentifier()
                {
                    GUID = shaderViewModel.GUID,
                    Descriptor =
                        $"Shader {shaderViewModel.GUID} - {System.IO.Path.GetFileName(shaderViewModel.Filename)}"
                }
            });
        }

        // Get breakpoint collection for the shader
        var collectionProperty = shaderPropertyViewModel.GetProperty<ShaderBreakpointCollectionViewModel>();
        if (collectionProperty == null)
        {
            shaderPropertyViewModel.Properties.Add(collectionProperty = new ShaderBreakpointCollectionViewModel()
            {
                FeatureInfo = featureInfo!.Value,
                ShaderPropertyProperty = shaderPropertyViewModel,
                Parent = shaderPropertyViewModel
            });
        }

        return collectionProperty;
    }
}

using System.Reactive.Disposables;
using DynamicData;
using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.Workspace;
using Runtime.Models.Objects;
using Runtime.ViewModels.Shader;
using Runtime.ViewModels.Traits;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
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
    public static void AddBreakpoint(BreakpointCollectionViewModel collection, ITextualContent content, int lineBase0, BreakpointCaptureMode captureMode)
    {
        // Create breakpoint
        BreakpointViewModel breakpointViewModel = new()
        {
            PropertyViewModel = collection.PropertyViewModel,
            CaptureMode = captureMode
        };
        
        // Register against registry
        collection.PropertyViewModel.GetService<BreakpointRegistryService>()?.Register(breakpointViewModel);
        
        // Bind the breakpoint
        BreakpointMappingUtils.SubscribeSourceLineMapping(content, breakpointViewModel.Disposable, lineBase0, (shaderViewModel, associationViewModel) =>
        {
            AddBreakpoint(breakpointViewModel, shaderViewModel, collection, associationViewModel, captureMode);
        });
    }
    
    /// <summary>
    /// Add a new breakpoint on a mapped instruction
    /// </summary>
    public static void AddBreakpoint(BreakpointViewModel breakpointViewModel, ShaderViewModel shaderViewModel, BreakpointCollectionViewModel collection, ShaderInstructionAssociationViewModel associationViewModel, BreakpointCaptureMode captureMode)
    {
        // No relevant mappings
        if (associationViewModel.Mappings.Count == 0)
        {
            return;
        }
        
        // Just choose the first for now
        if (BreakpointMappingUtils.FindRepresentativeInstruction(shaderViewModel, associationViewModel) is not { } mapping)
        {
            return;
        }

        // We always create shader properties with the given shader collection
        // The file collections are mere mirrors
        if (collection.PropertyViewModel.GetProperty<BreakpointCollectionRegistryViewModel>()?.FindOrAdd(shaderViewModel) is not { } shaderCollection)
        {
            return;
        }
        
        // A breakpoint has been added to a specific shader, so, create the property
        ShaderBreakpointCollectionPropertyViewModel? property = FindOrCreateShaderCollectionProperty(shaderCollection, shaderViewModel);
        if (property == null)
        {
            return;
        }

        // Keep track of property
        breakpointViewModel.ShaderProperties.Add((ShaderPropertyViewModel)property.Parent!);

        // Create binding
        BreakpointViewModelBinding binding = new()
        {
            BreakpointViewModel = breakpointViewModel,
            Source = new()
            {
                Mapping = mapping,
                AssociationViewModel = associationViewModel
            }
        };
        
        // Always add it to the physical collection
        shaderCollection.Bindings.Add(binding);

        // If this is a logical collection, mirror it
        if (collection != shaderCollection)
        {
            collection.Bindings.Add(binding);
        }
    }

    /// <summary>
    /// Create a shader collection property
    /// </summary>
    public static ShaderBreakpointCollectionPropertyViewModel? FindOrCreateShaderCollectionProperty(BreakpointCollectionViewModel collectionViewModel, ShaderViewModel shaderViewModel)
    {
        // TODO[dbg]: Standardize this, and only when breakpoints are added
        
        // Find the debug feature
        FeatureInfo? featureInfo = collectionViewModel.PropertyViewModel
            .GetWorkspaceCollection()?
            .GetWorkspaceCollection()?
            .GetProperty<IFeatureCollectionViewModel>()?
            .GetFeature("Debug");

        // Must have shader collection
        var shaderCollectionViewModel = collectionViewModel.PropertyViewModel.GetProperty<IShaderCollectionViewModel>();
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
                    Descriptor = $"Shader {shaderViewModel.GUID} - {System.IO.Path.GetFileName(shaderViewModel.Filename)}"
                }
            });
        }

        // Get breakpoint collection for the shader
        var collectionProperty = shaderPropertyViewModel.GetProperty<ShaderBreakpointCollectionPropertyViewModel>();
        if (collectionProperty == null)
        {
            shaderPropertyViewModel.Properties.Add(collectionProperty = new ShaderBreakpointCollectionPropertyViewModel()
            {
                FeatureInfo = featureInfo!.Value,
                ShaderPropertyProperty = shaderPropertyViewModel,
                CollectionViewModel = collectionViewModel,
                Parent = shaderPropertyViewModel
            });

            // Remove on collection destruction
            collectionViewModel.Disposable.Add(Disposable.Create(() =>
            {
                shaderPropertyViewModel.Properties.Remove(collectionProperty);
            }));
        }

        return collectionProperty;
    }

    /// <summary>
    /// Get the breakpoint collection of a view model
    /// </summary>
    public static BreakpointCollectionViewModel? GetShaderBreakpointCollection(IPropertyViewModel propertyViewModel, object viewModel)
    {
        // Get shared registry
        var collectionRegistry = propertyViewModel.GetProperty<BreakpointCollectionRegistryViewModel>();
        if (collectionRegistry == null)
        {
            return null;
        }

        // Find or add view model
        return collectionRegistry.FindOrAdd(viewModel);
    }
}

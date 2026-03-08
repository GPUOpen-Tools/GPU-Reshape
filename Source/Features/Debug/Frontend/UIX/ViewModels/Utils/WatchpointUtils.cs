using System.Reactive.Disposables;
using DynamicData;
using GRS.Features.Debug.UIX.Models;
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

public static class WatchpointUtils
{
    /// <summary>
    /// Add a new watchpoint from a given line
    /// </summary>
    public static void AddWatchpoint(WatchpointCollectionViewModel collection, ITextualContent content, int lineBase0, WatchpointCaptureMode captureMode)
    {
        // Create watchpoint
        WatchpointViewModel watchpointViewModel = new()
        {
            PropertyViewModel = collection.PropertyViewModel,
            CaptureMode = captureMode
        };
        
        // Register with collection
        collection.Register(new WatchpointViewModelTextualBinding()
        {
            WatchpointViewModel = watchpointViewModel,
            LineBase0 = lineBase0
        });
        
        // Bind the watchpoint
        WatchpointMappingUtils.SubscribeSourceLineMapping(content, watchpointViewModel.Disposable, lineBase0, (shaderViewModel, associationViewModel) =>
        {
            AddWatchpointBinding(watchpointViewModel, shaderViewModel, collection, associationViewModel, captureMode);
        });
    }
    
    /// <summary>
    /// Add a new watchpoint on a mapped instruction
    /// </summary>
    public static void AddWatchpointBinding(WatchpointViewModel watchpointViewModel, ShaderViewModel shaderViewModel, WatchpointCollectionViewModel collection, ShaderInstructionAssociationViewModel associationViewModel, WatchpointCaptureMode captureMode)
    {
        // No relevant mappings
        if (associationViewModel.Mappings.Count == 0)
        {
            return;
        }
        
        // Just choose the first for now
        if (WatchpointMappingUtils.FindRepresentativeInstruction(shaderViewModel, associationViewModel) is not { } mapping)
        {
            return;
        }

        // We always create shader properties with the given shader collection
        // The file collections are mere mirrors
        if (collection.PropertyViewModel.GetProperty<WatchpointCollectionRegistryViewModel>()?.FindOrAdd(shaderViewModel) is not { } shaderCollection)
        {
            return;
        }
        
        // A watchpoint has been added to a specific shader, so, create the property
        ShaderWatchpointCollectionPropertyViewModel? property = FindOrCreateShaderCollectionProperty(shaderCollection, shaderViewModel);
        if (property == null)
        {
            return;
        }

        // Keep track of property
        watchpointViewModel.ShaderProperties.Add((ShaderPropertyViewModel)property.Parent!);

        // Create binding
        WatchpointViewModelSourceBinding binding = new()
        {
            WatchpointViewModel = watchpointViewModel,
            Source = new()
            {
                Mapping = mapping,
                AssociationViewModel = associationViewModel
            }
        };
        
        // Add it to the physical collection
        shaderCollection.SourceBindings.Add(binding);
    }

    /// <summary>
    /// Create a shader collection property
    /// </summary>
    public static ShaderWatchpointCollectionPropertyViewModel? FindOrCreateShaderCollectionProperty(WatchpointCollectionViewModel collectionViewModel, ShaderViewModel shaderViewModel)
    {
        // TODO[dbg]: Standardize this, and only when watchpoints are added
        
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

        // Get watchpoint collection for the shader
        var collectionProperty = shaderPropertyViewModel.GetProperty<ShaderWatchpointCollectionPropertyViewModel>();
        if (collectionProperty == null)
        {
            shaderPropertyViewModel.Properties.Add(collectionProperty = new ShaderWatchpointCollectionPropertyViewModel()
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
    /// Get the watchpoint collection of a view model
    /// </summary>
    public static WatchpointCollectionViewModel? GetShaderWatchpointCollection(IPropertyViewModel propertyViewModel, object viewModel)
    {
        // Get shared registry
        var collectionRegistry = propertyViewModel.GetProperty<WatchpointCollectionRegistryViewModel>();
        if (collectionRegistry == null)
        {
            return null;
        }

        // Find or add view model
        return collectionRegistry.FindOrAdd(viewModel);
    }
}

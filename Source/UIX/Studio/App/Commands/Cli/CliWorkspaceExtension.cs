using System;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;

namespace Studio.App.Commands.Cli;

public class CliWorkspaceExtension : IWorkspaceExtension
{
    /// <summary>
    /// Install this extension
    /// </summary>
    public void Install(IWorkspaceViewModel workspaceViewModel)
    {
        _shaderCollection  = workspaceViewModel.PropertyCollection.GetProperty<IShaderCollectionViewModel>();
        _shaderCodeService = workspaceViewModel.PropertyCollection.GetService<IShaderCodeService>();

        // Subscribe to all validation objects
        if (workspaceViewModel.PropertyCollection.GetProperty<MessageCollectionViewModel>() is { } messageCollection)
        {
            messageCollection.ValidationObjects.ToObservableChangeSet()
                .AsObservableList()
                .Connect()
                .OnItemAdded(OnValidationObjectAdded)
                .Subscribe();
        }
    }

    /// <summary>
    /// Invoked on validation object additions
    /// </summary>
    private void OnValidationObjectAdded(ValidationObject obj)
    {
        obj.WhenAnyValue(x => x.Segment).WhereNotNull().Subscribe(segment =>
        {
            Logging.Info($"Enqueueing shader contents for SGUID {segment.Location.SGUID}");
            
            // Whenever a validation object is enqueued, immediately start pooling its shader contents
            // for later reporting purposes.
            if (_shaderCollection?.GetOrAddShader(segment.Location.SGUID) is { } shader)
            {
                _shaderCodeService?.EnqueueShaderContents(shader);
                _shaderCodeService?.EnqueueShaderIL(shader);
            }
        });
    }

    /// <summary>
    /// Workspace shaders
    /// </summary>
    private IShaderCollectionViewModel? _shaderCollection;
    
    /// <summary>
    /// Workspace code service
    /// </summary>
    private IShaderCodeService? _shaderCodeService;
}
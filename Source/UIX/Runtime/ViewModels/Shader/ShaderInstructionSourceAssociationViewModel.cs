using ReactiveUI;
using Studio.Models.Workspace.Objects;

namespace Runtime.ViewModels.Shader;

public class ShaderInstructionSourceAssociationViewModel : ReactiveObject
{
    /// <summary>
    /// Assigned location
    /// </summary>
    public ShaderLocation? Location
    {
        get => _location;
        set => this.RaiseAndSetIfChanged(ref _location, value);
    }

    /// <summary>
    /// Internal location
    /// </summary>
    private ShaderLocation? _location;
}

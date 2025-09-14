using ReactiveUI;
using System.Collections.ObjectModel;
using Studio.Models.Workspace.Objects;

namespace Runtime.ViewModels.Shader;

public class ShaderInstructionAssociationViewModel : ReactiveObject
{
    /// <summary>
    /// All discovered mappings
    /// </summary>
    public ObservableCollection<AssembledInstructionMapping> Mappings { get; } = new();

    /// <summary>
    /// Has all mappings been populated?
    /// </summary>
    public bool Populated
    {
        get => _populated;
        set => this.RaiseAndSetIfChanged(ref _populated, value);
    }
    
    /// <summary>
    /// Internal state
    /// </summary>
    private bool _populated = false;
}

using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.Models.Workspace.Objects;

namespace GRS.Features.Debug.UIX.ViewModels;

public class SourceBinding : ReactiveObject
{
    /// <summary>
    /// Source-wise line of the instruction
    /// </summary>
    public AssembledInstructionMapping Mapping
    {
        get => _mapping;
        set => this.RaiseAndSetIfChanged(ref _mapping, value);
    }

    /// <summary>
    /// Backend code-offset of the instruction
    /// </summary>
    public ShaderInstructionAssociationViewModel AssociationViewModel
    {
        get => _associationViewModel;
        set => this.RaiseAndSetIfChanged(ref _associationViewModel, value);
    }

    /// <summary>
    /// Internal line
    /// </summary>
    private AssembledInstructionMapping _mapping;
    
    /// <summary>
    /// Internal code-offset
    /// </summary>
    private ShaderInstructionAssociationViewModel _associationViewModel;
}
using Studio.Models.IL;
using Studio.Models.Workspace.Objects;

namespace GRS.Features.Debug.UIX.Models;

public struct MappedInstruction
{
    /// <summary>
    /// The mapping for this instruction
    /// </summary>
    public required AssembledInstructionMapping Mapping;
    
    /// <summary>
    /// The mapped instruction
    /// </summary>
    public required Instruction Instruction;
}

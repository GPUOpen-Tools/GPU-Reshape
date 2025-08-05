namespace Studio.Models.Workspace.Objects;

public struct AssembledInstructionMapping
{
    /// <summary>
    /// Instruction basic block id
    /// </summary>
    public uint BasicBlockId { get; set; }
        
    /// <summary>
    /// Instruction linear index into the basic block
    /// </summary>
    public uint InstructionIndex { get; set; }
        
    /// <summary>
    /// Backend code offset used for association
    /// </summary>
    public uint CodeOffset { get; set; }
}

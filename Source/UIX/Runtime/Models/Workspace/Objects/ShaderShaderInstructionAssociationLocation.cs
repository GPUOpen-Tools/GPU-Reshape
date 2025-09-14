using System;

namespace Studio.Models.Workspace.Objects;

public record struct ShaderShaderInstructionAssociationLocation
{
    /// <summary>
    /// Originating shader UID
    /// </summary>
    public UInt64 SGUID { get; set; }
        
    /// <summary>
    /// Line offset
    /// </summary>
    public int Line { get; set; }
        
    /// <summary>
    /// File identifier
    /// </summary>
    public int FileUID { get; set; }
}

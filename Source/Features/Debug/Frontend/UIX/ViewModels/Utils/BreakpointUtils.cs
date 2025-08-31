using GRS.Features.Debug.UIX.Models;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;

namespace GRS.Features.Debug.UIX.ViewModels.Utils;

public static class BreakpointUtils
{
    public static void AddBreakpoint(ShaderBreakpointCollectionViewModel collection, ITextualShaderContentViewModel content, int lineBase0, BreakpointCaptureMode captureMode)
    {
        // Find the instruction representing the current line
        AssembledInstructionMapping mapping = content.TransformInstruction(lineBase0);

        // Translate to segment
        ShaderSourceSegment segment = new()
        {
            Location = new ShaderLocation()
            {
                BasicBlockId = mapping.BasicBlockId,
                InstructionIndex = mapping.InstructionIndex,
                FileUID = (int)ShaderLocation.InvalidFileUID
            }
        };
            
        // None found, add it
        collection.Breakpoints.Add(new BreakpointViewModel
        {
            ShaderProperty = collection.ShaderProperty,
            ShaderContentViewModel = content,
            ShaderSourceSegment = segment,
            CaptureMode = captureMode,
            SourceBinding = new SourceBinding
            {
                InstructionLine = lineBase0,
                InstructionCodeOffset = mapping.CodeOffset
            }
        });
    }
}

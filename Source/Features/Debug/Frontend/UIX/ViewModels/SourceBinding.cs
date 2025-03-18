using ReactiveUI;

namespace GRS.Features.Debug.UIX.ViewModels;

public class SourceBinding : ReactiveObject
{
    /// <summary>
    /// Source-wise line of the instruction
    /// </summary>
    public int InstructionLine
    {
        get => _instructionLine;
        set => this.RaiseAndSetIfChanged(ref _instructionLine, value);
    }

    /// <summary>
    /// Backend code-offset of the instruction
    /// </summary>
    public uint InstructionCodeOffset
    {
        get => _instructionCodeOffset;
        set => this.RaiseAndSetIfChanged(ref _instructionCodeOffset, value);
    }

    /// <summary>
    /// Internal line
    /// </summary>
    private int _instructionLine = 0;
    
    /// <summary>
    /// Internal code-offset
    /// </summary>
    private uint _instructionCodeOffset = 0;
}
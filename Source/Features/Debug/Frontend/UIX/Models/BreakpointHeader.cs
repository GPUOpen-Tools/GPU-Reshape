using System.Runtime.InteropServices;
using Studio.Models.Instrumentation;

namespace GRS.Features.Debug.UIX.Models;

/// <summary>
/// Mirror of Cxx Format, keep up to date
/// </summary>

[System.Flags]
public enum BreakpointFlag
{
    None = 0,
    AllowImageFPUNorm8888Compression = 1 << 0,
}

public enum BreakpointCompression
{
    None,
    FPUNorm8888
}

public enum BreakpointDataOrder
{
    None,
    Static,
    Dynamic,
    Loose
}

public enum BreakpointCaptureMode {
    FirstEvent,
    AllEvents
};

[StructLayout(LayoutKind.Explicit, Size = 52, CharSet = CharSet.Ansi)]
public struct LooseBreakpointHeader
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(LooseBreakpointHeader)) / sizeof(uint));

    [FieldOffset(0)]
    public ExecutionInfo executionInfo;
    
    [FieldOffset(40)]
    public uint threadX;

    [FieldOffset(44)]
    public uint threadY;
    
    [FieldOffset(48)]
    public uint threadZ;
}

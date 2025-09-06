using System.Runtime.InteropServices;
using Studio.Models.Instrumentation;

namespace GRS.Features.Debug.UIX.Models;

/// <summary>
/// Source Mirrors
/// - Source/Features/Debug/Backend/Include/Features/Debug/BreakpointHeader.h
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

[StructLayout(LayoutKind.Explicit, Size = 92, CharSet = CharSet.Ansi)]
public struct LooseBreakpointHeader
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(LooseBreakpointHeader)) / sizeof(uint));

    [FieldOffset(0)]
    public ExecutionInfo executionInfo;
    
    [FieldOffset(80)]
    public uint threadX;

    [FieldOffset(84)]
    public uint threadY;
    
    [FieldOffset(88)]
    public uint threadZ;
}

[StructLayout(LayoutKind.Explicit, Size = 4, CharSet = CharSet.Ansi)]
public struct DynamicBreakpointHeader
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(DynamicBreakpointHeader)) / sizeof(uint));

    [FieldOffset(0)]
    public uint thread;
}

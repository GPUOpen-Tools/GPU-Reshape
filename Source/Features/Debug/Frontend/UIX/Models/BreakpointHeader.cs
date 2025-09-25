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
    EarlyDepthStencil =  1 << 1,
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
    FirstViewport,
    AllEvents
};

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct LooseBreakpointHeader
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(LooseBreakpointHeader)) / sizeof(uint));

    public ExecutionInfo executionInfo;
    public uint threadX;
    public uint threadY;
    public uint threadZ;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct DynamicBreakpointHeader
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(DynamicBreakpointHeader)) / sizeof(uint));

    public uint thread;
}

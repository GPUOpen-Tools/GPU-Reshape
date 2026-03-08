using System.Runtime.InteropServices;
using Studio.Models.Instrumentation;

namespace GRS.Features.Debug.UIX.Models;

/// <summary>
/// Source Mirrors
/// - Source/Features/Debug/Backend/Include/Features/Debug/WatchpointHeader.h
/// </summary>

[System.Flags]
public enum WatchpointFlag
{
    None = 0,
    AllowImageFPUNorm8888Compression = 1 << 0,
    EarlyDepthStencil =  1 << 1,
}

public enum WatchpointCompression
{
    None,
    FPUNorm8888
}

public enum WatchpointDataOrder
{
    None,
    Static,
    Dynamic,
    Loose
}

public enum WatchpointCaptureMode {
    FirstEvent,
    FirstViewport,
    AllEvents
};

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct LooseWatchpointHeader
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(LooseWatchpointHeader)) / sizeof(uint));

    public ExecutionInfo executionInfo;
    public uint threadX;
    public uint threadY;
    public uint threadZ;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct DynamicWatchpointHeader
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(DynamicWatchpointHeader)) / sizeof(uint));

    public uint thread;
}

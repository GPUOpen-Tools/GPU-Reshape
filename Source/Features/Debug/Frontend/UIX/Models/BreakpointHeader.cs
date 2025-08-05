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
    Dynamic
}

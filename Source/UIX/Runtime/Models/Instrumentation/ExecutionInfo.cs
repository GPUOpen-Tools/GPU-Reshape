using System.Runtime.InteropServices;
using Message.CLR;

namespace Studio.Models.Instrumentation;

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public unsafe struct ExecutionInfo
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(ExecutionInfo)) / sizeof(uint));

    [StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
    public struct DrawInfo
    {
        public ExecutionDrawFlag drawFlags;
        public uint vertexCountPerInstance;
        public uint indexCountPerInstance;
        public uint instanceCount;
        public uint startVertex;
        public uint startIndex;
        public uint startInstance;
        public uint vertexOffset;
        public uint instanceOffset;
    }
    
    [StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
    public struct DispatchInfo
    {
        public uint groupCountX;
        public uint groupCountY;
        public uint groupCountZ;
    }
    
    [StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
    public struct Viewport
    {
        public uint width;
        public uint height;
    }
    
    public uint rollingExecutionUID;
    public uint rollingViewportUID;
    public ExecutionFlag executionFlags;
    public uint pipelineUID;
    public fixed uint markerHashes32[5];
    public uint queueUID;
    public DrawInfo drawInfo;
    public DispatchInfo dispatchInfo;
    public Viewport viewport;
}

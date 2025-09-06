using System.Runtime.InteropServices;
using Message.CLR;

namespace Studio.Models.Instrumentation;

[StructLayout(LayoutKind.Explicit, Size = 80, CharSet = CharSet.Ansi)]
public struct ExecutionInfo
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(ExecutionInfo)) / sizeof(uint));

    [StructLayout(LayoutKind.Explicit, Size = 36, CharSet = CharSet.Ansi)]
    public struct DrawInfo
    {
        [FieldOffset(0)]
        public ExecutionDrawFlag drawFlags;

        [FieldOffset(4)] 
        public uint vertexCountPerInstance;
        
        [FieldOffset(8)] 
        public uint indexCountPerInstance;
        
        [FieldOffset(12)] 
        public uint instanceCount;
        
        [FieldOffset(16)]
        public uint startVertex;
        
        [FieldOffset(20)]
        public uint startIndex;
        
        [FieldOffset(24)]
        public uint startInstance;
        
        [FieldOffset(28)]
        public uint vertexOffset;
        
        [FieldOffset(32)]
        public uint instanceOffset;
    }
    
    [StructLayout(LayoutKind.Explicit, Size = 12, CharSet = CharSet.Ansi)]
    public struct DispatchInfo
    {
        [FieldOffset(0)]
        public uint groupCountX;
        
        [FieldOffset(4)]
        public uint groupCountY;
        
        [FieldOffset(8)]
        public uint groupCountZ;
    }
    
    [StructLayout(LayoutKind.Explicit, Size = 8, CharSet = CharSet.Ansi)]
    public struct Viewport
    {
        [FieldOffset(0)]
        public uint width;
        
        [FieldOffset(4)]
        public uint height;
    }
    
    [FieldOffset(0)]
    public uint rollingExecutionUID;

    [FieldOffset(4)]
    public ExecutionFlag executionFlags;
    
    [FieldOffset(8)]
    public uint pipelineUID;

    [FieldOffset(12)]
    public uint scopeUID;

    [FieldOffset(16)]
    public uint queueUID;
    
    [FieldOffset(20)]
    public DrawInfo drawInfo;
    
    [FieldOffset(56)]
    public DispatchInfo dispatchInfo;
    
    [FieldOffset(68)]
    public Viewport viewport;
    
    [FieldOffset(76)]
    public uint pad20;
}

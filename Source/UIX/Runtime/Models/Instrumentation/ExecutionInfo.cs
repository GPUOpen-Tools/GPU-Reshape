using System.Runtime.InteropServices;
using Message.CLR;

namespace Studio.Models.Instrumentation;

[StructLayout(LayoutKind.Explicit, Size = 40, CharSet = CharSet.Ansi)]
public struct ExecutionInfo
{
    public static uint DWordCount = (uint)(Marshal.SizeOf(typeof(ExecutionInfo)) / sizeof(uint));

    [StructLayout(LayoutKind.Explicit, Size = 8, CharSet = CharSet.Ansi)]
    public struct DrawInfo
    {
        [FieldOffset(0)]
        public uint vertexCount;
        
        [FieldOffset(4)]
        public uint indexCount;
    }
    
    [StructLayout(LayoutKind.Explicit, Size = 8, CharSet = CharSet.Ansi)]
    public struct DispatchInfo
    {
        [FieldOffset(0)]
        public uint groupCountX;
        
        [FieldOffset(4)]
        public uint groupCountY;
        
        [FieldOffset(8)]
        public uint groupCountZ;
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
    
    [FieldOffset(28)]
    public DispatchInfo dispatchInfo;
}

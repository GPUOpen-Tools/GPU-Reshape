using System;

namespace Studio.ViewModels.Workspace.Listeners
{
    [Flags]
    public enum ShaderCodePoolingState
    {
        None = 0,
        Contents = 1 << 0,
        IL = 1 << 1,
        BlockGraph = 1 << 2,
    }
}

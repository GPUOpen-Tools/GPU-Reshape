using System.Text;
using Message.CLR;
using Studio.ViewModels.Workspace;

namespace Runtime.Utils.Workspace;

public static class TracebackUtils
{
    /// <summary>
    /// Format a traceback model
    /// </summary>
    public static string Format(IWorkspaceViewModel _, Traceback traceback)
    {
        StringBuilder builder = new();

        if (traceback.executionFlag.HasFlag(ExecutionFlag.Indirect))
        {
            builder.Append("Indirect ");
        }

        if (traceback.executionFlag == ExecutionFlag.Draw)
        {
            builder.Append("Draw");
        }

        if (traceback.executionFlag == ExecutionFlag.Dispatch)
        {
            builder.Append("Dispatch");
        }

        if (traceback.executionFlag == ExecutionFlag.Raytracing)
        {
            builder.Append("Raytracing");
        }

        // TODO: Pipeline collection
        builder.Append($", Pipeline {traceback.pipelineUid}");

        // Thread indices
        builder.Append($", Thread [{traceback.threadX}, {traceback.threadY}, {traceback.threadZ}]");
        
        // Launch dimensions
        if (traceback.executionFlag == ExecutionFlag.Dispatch)
        {
            builder.Append($", Thread Groups [{traceback.kernelLaunchX}, {traceback.kernelLaunchY}, {traceback.kernelLaunchZ}]");
        }

        return builder.ToString();
    }
}

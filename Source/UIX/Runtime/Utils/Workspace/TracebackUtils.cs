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
        builder.Append(Format(traceback.executionFlag));

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

    /// <summary>
    /// Format a traceback execution flag
    /// </summary>
    public static string Format(ExecutionFlag executionFlag)
    {
        StringBuilder builder = new();

        if (executionFlag.HasFlag(ExecutionFlag.Indirect))
        {
            builder.Append("Indirect ");
        }

        if (executionFlag == ExecutionFlag.Draw)
        {
            builder.Append("Draw");
        }

        if (executionFlag == ExecutionFlag.Dispatch)
        {
            builder.Append("Dispatch");
        }

        if (executionFlag == ExecutionFlag.Raytracing)
        {
            builder.Append("Raytracing");
        }
        
        return builder.ToString();
    }
}

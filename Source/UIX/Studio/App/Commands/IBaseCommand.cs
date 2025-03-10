using System.CommandLine;
using System.CommandLine.Invocation;
using System.Threading.Tasks;

namespace Studio.App.Commands;

public interface IBaseCommand : ICommandHandler
{
    /// <summary>
    /// Stub handler
    /// </summary>
    int ICommandHandler.Invoke(InvocationContext context)
    {
        throw new System.NotSupportedException();
    }
    
    /// <summary>
    /// Stub async handler
    /// </summary>
    Task<int> ICommandHandler.InvokeAsync(InvocationContext context)
    {
        throw new System.NotSupportedException();
    }
}

public static class CommandExtensions
{
    /// <summary>
    /// Create a new command
    /// </summary>
    /// <param name="handler">default handler</param>
    /// <param name="options">all given options</param>
    /// <returns></returns>
    public static Command Make(this Command self, ICommandHandler handler, Option[] options)
    {
        self.Handler = handler;
        
        foreach (Option option in options)
        {
            self.AddOption(option);
        }
        
        return self;
    }
}
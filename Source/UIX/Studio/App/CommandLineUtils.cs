using System.Collections.Generic;

namespace Studio.App;

public static class CommandLineUtils
{
    /// <summary>
    /// Split a command into its logical sub-commands
    /// </summary>
    public static List<string[]> SplitCommands(string[] args)
    {
        List<string[]> commands = new();
        List<string>   command  = new();
        
        // Just split by &&
        foreach (string arg in args)
        {
            if (arg == "&&")
            {
                commands.Add(command.ToArray());
                command = new();
                continue;
            }
            
            command.Add(arg);
        }

        // Dangling command
        if (command.Count > 0)
        {
            commands.Add(command.ToArray());
        }

        return commands;
    }
}
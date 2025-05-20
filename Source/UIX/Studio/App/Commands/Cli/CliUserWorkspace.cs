// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

using System;
using System.IO;
using Newtonsoft.Json;

namespace Studio.App.Commands.Cli;

public class CliUserWorkspaceConfig
{
    /// <summary>
    /// Enables detailed reporting
    /// </summary>
    public bool Detail { get; set; } = false;

    /// <summary>
    /// Enabled coverage reporting, limits streaming per message
    /// </summary>
    public bool Coverage { get; set; } = false;

    /// <summary>
    /// Redirect application outputs to this process
    /// </summary>
    public bool RedirectOutput { get; set; } = false;

    /// <summary>
    /// Suspend the deferred initialization thread
    /// </summary>
    public bool SuspendDeferredInitialization { get; set; } = false;
}

public class CliUserWorkspace
{
    /// <summary>
    /// Workspace configuration
    /// </summary>
    public CliUserWorkspaceConfig Config { get; set; } = new();

    /// <summary>
    /// All enabled features
    /// </summary>
    public string[] Features = Array.Empty<string>();
    
    /// <summary>
    /// Deserialize from string
    /// </summary>
    public static CliUserWorkspace? Deserialize(string contents)
    {
        // Just try to load it, assume invalid if failed
        try
        {
            CliUserWorkspace workspace = new();
            JsonConvert.PopulateObject(contents, workspace);
            return workspace;
        }
        catch
        {
            Logging.Error("Failed to deserialize user workspace data");
            return null;
        }
    }

    /// <summary>
    /// Deserialize from file
    /// </summary>
    public static CliUserWorkspace? DeserializeFile(string path)
    {
        // Just try to load it, assume invalid if failed
        try
        {
            return Deserialize(File.ReadAllText(path));
        }
        catch
        {
            Logging.Error("Failed to read user workspace file");
            return null;
        }
    }
}
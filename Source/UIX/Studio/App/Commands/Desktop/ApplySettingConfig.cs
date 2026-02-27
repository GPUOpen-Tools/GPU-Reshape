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

using System.IO;
using Newtonsoft.Json;

namespace Studio.App.Commands.Desktop;

public class ApplySettingSymbolConfig
{
    /// <summary>
    /// All paths
    /// </summary>
    public string[] Paths { get; set; } = { };

    /// <summary>
    /// Recursive?
    /// </summary>
    public bool IncludeSubDirectories { get; set; } = false;
}

public class ApplySettingSourceConfig
{
    /// <summary>
    /// All paths
    /// </summary>
    public string[] Paths { get; set; } = { };

    /// <summary>
    /// Recursive?
    /// </summary>
    public bool IncludeSubDirectories { get; set; } = false;
}

public class ApplySettingConfig
{
    /// <summary>
    /// App name to register for
    /// </summary>
    public string ApplicationName { get; set; } = string.Empty;
    
    /// <summary>
    /// Symbol configuration
    /// </summary>
    public ApplySettingSymbolConfig Symbol { get; set; } = new();
    
    /// <summary>
    /// Source configuration
    /// </summary>
    public ApplySettingSourceConfig Source { get; set; } = new();

    /// <summary>
    /// Deserialize from string
    /// </summary>
    public static ApplySettingConfig? Deserialize(string contents)
    {
        // Just try to load it, assume invalid if failed
        try
        {
            ApplySettingConfig workspace = new();
            JsonConvert.PopulateObject(contents, workspace);
            return workspace;
        }
        catch
        {
            Logging.Error("Failed to deserialize user setting data");
            return null;
        }
    }

    /// <summary>
    /// Deserialize from file
    /// </summary>
    public static ApplySettingConfig? DeserializeFile(string path)
    {
        // Just try to load it, assume invalid if failed
        try
        {
            return Deserialize(File.ReadAllText(path));
        }
        catch
        {
            Logging.Error("Failed to read user setting file");
            return null;
        }
    }
}
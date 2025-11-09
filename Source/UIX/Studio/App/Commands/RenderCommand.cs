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
using System.Collections.Generic;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.IO;
using System.Threading.Tasks;
using Avalonia.Platform;
using Scriban;
using Studio.App.Commands.Render;

namespace Studio.App.Commands;

public class RenderCommand : IBaseCommand
{
    /// <summary>
    /// Create the render command
    /// </summary>
    public static Command Create()
    {
        // Setup command
        return new Command("render").Make(new RenderCommand(), new Option[]
        {
            Report,
            Out
        });
    }

    static RenderCommand()
    {
        
    }

    /// <summary>
    /// Command invocation
    /// </summary>
    public async Task<int> InvokeAsync(InvocationContext context)
    {
        string outPath = context.ParseResult.GetValueForOption(Out)!;

        // Deserialize and model the report
        if (RenderReportModel.DeserializeFile(context.ParseResult.GetValueForOption(Report)!) is not { } model)
        {
            return 1;
        }

        RenderReportTemplateModel template = new()
        {
            Model = model
        };

        // Try to render
        if (!template.Render(outPath))
        {
            return 1;
        }

        // OK
        return 0;
    }

    /// <summary>
    /// Report file option
    /// </summary>
    private static readonly Option<string> Report = new("-report", "The input report path (json)")
    {
        IsRequired = true
    };
    
    /// <summary>
    /// Output file option
    /// </summary>
    private static readonly Option<string> Out = new("-out", "The output path (must include extension, dictates render mode [html])")
    {
        IsRequired = true
    };
}
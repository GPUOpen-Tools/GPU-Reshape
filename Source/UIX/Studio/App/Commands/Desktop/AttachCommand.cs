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
using System.CommandLine;
using System.CommandLine.Invocation;
using System.IO;
using System.Threading.Tasks;
using Avalonia.Threading;
using Studio.Services;
using Studio.ViewModels.Workspace;

namespace Studio.App.Commands;

public class AttachCommand : IBaseCommand
{
    /// <summary>
    /// Create the attach command
    /// </summary>
    public static Command Create()
    {
        // Setup command
        return new Command("attach").Make(new AttachCommand(), new Option[]
        {
            PID,
            Token,
            Process
        });
    }

    /// <summary>
    /// Command invocation
    /// </summary>
    public Task<int> InvokeAsync(InvocationContext context)
    {
        _context = context;
        
        // Subscribe
        _connectionViewModel.Connected.Subscribe(_ => Dispatcher.UIThread.InvokeAsync(OnRemoteConnected));
        
        // Start connection
        _connectionViewModel.Connect("127.0.0.1", null);
        
        // OK
        return Task.FromResult(0);
    }

    /// <summary>
    /// Invoked on remote host connection
    /// </summary>
    private void OnRemoteConnected()
    {
        // Get provider
        var provider = ServiceRegistry.Get<IWorkspaceService>();

        // Create process workspace
        var workspace = new ProcessWorkspaceViewModel(_context.ParseResult.GetValueForOption(Token)!)
        {
            Connection = _connectionViewModel
        };

        // Create application
        _connectionViewModel.Application = new ApplicationInfoViewModel()
        {
            Guid = Guid.NewGuid(),
            Pid = (ulong)Int32.Parse(_context.ParseResult.GetValueForOption(PID)!),
            Process = Path.GetFileName(_context.ParseResult.GetValueForOption(Process)!),
            DecorationMode = ApplicationDecorationMode.ProcessOnly
        };

        // Configure and register workspace
        provider?.Install(workspace);
        provider?.Add(workspace, false);
    }

    /// <summary>
    /// Process identifier
    /// </summary>
    private static readonly Option<string> PID = new("-pid", "The process id to attach to")
    {
        IsRequired = true
    };
    
    /// <summary>
    /// Workspace device token
    /// </summary>
    private static readonly Option<string> Token = new("-token", "The reserved token used on launch")
    {
        IsRequired = true
    };
    
    /// <summary>
    /// Name of the process
    /// </summary>
    private static readonly Option<string> Process = new("-process", "The process name")
    {
        IsRequired = true
    };
    
    /// <summary>
    /// Internal connection
    /// </summary>
    private ConnectionViewModel _connectionViewModel = new();
    
    /// <summary>
    /// Internal context
    /// </summary>
    private InvocationContext _context;
}
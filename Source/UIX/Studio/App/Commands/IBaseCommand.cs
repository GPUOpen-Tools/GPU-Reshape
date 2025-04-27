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
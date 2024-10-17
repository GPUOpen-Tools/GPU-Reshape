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

using Avalonia;
using DynamicData;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Logging;

namespace Studio
{
    public class Logging
    {
        /// <summary>
        /// View model getter
        /// </summary>
        public static ILoggingViewModel? ViewModel => ServiceRegistry.Get<ILoggingService>()?.ViewModel;

        /// <summary>
        /// Add a new event log
        /// </summary>
        /// <param name="severity"></param>
        /// <param name="message"></param>
        public static void Add(LogSeverity severity, string message)
        {
            ViewModel?.Events.Add(new LogEvent()
            {
                Severity = severity,
                Message = message
            });
        }

        /// <summary>
        /// Add an info event log
        /// </summary>
        /// <param name="message"></param>
        public static void Info(string message)
        {
            Add(LogSeverity.Info, message);
        }

        /// <summary>
        /// Add a warning event log
        /// </summary>
        /// <param name="message"></param>
        public static void Warning(string message)
        {
            Add(LogSeverity.Warning, message);
        }

        /// <summary>
        /// Add an error event log
        /// </summary>
        /// <param name="message"></param>
        public static void Error(string message)
        {
            Add(LogSeverity.Error, message);
        }
    }
}
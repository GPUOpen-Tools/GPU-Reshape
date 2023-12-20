// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

using ReactiveUI;

namespace Studio.ViewModels.Workspace.Objects
{
    public class ResourceValidationInstance : ReactiveObject
    {
        /// <summary>
        /// Instance message
        /// </summary>
        public string Message
        {
            get => _message;
            set => this.RaiseAndSetIfChanged(ref _message, value);
        }

        /// <summary>
        /// Instance message
        /// </summary>
        public string MessageNoNotify
        {
            get => _message;
            set => _message = value;
        }

        /// <summary>
        /// Number of occurrences
        /// </summary>
        public ulong Count
        {
            get => _count;
            set => this.RaiseAndSetIfChanged(ref _count, value);
        }

        /// <summary>
        /// Number of occurrences
        /// </summary>
        public ulong CountNoNotify
        {
            get => _count;
            set => _count = value;
        }

        /// <summary>
        /// Internal count
        /// </summary>
        private ulong _count;

        /// <summary>
        /// Internal message
        /// </summary>
        private string _message = string.Empty;
    }
}
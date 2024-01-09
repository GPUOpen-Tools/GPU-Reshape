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

using System.Threading.Tasks;
using Studio.ViewModels;

namespace Studio.Services
{
    public interface IWindowService
    {
        /// <summary>
        /// Shared layout
        /// </summary>
        public ILayoutViewModel? LayoutViewModel { get; set; }
        
        /// <summary>
        /// Open a window for a given view model
        /// </summary>
        /// <param name="viewModel">view model</param>
        Task<object?> OpenFor(object viewModel);

        /// <summary>
        /// Request exit
        /// </summary>
        void Exit();
    }
    
    public static class WindowServiceExtensions
    {
        /// <summary>
        /// Typed OpenFor
        /// </summary>
        public static Task<T?> OpenFor<T>(this IWindowService self, object viewModel) where T : class
        {
            return self.OpenFor(viewModel).ContinueWith<T?>(x => (T?)x.Result);
        }
    }
}
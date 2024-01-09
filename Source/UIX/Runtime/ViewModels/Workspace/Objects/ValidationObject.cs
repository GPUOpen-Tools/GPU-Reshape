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

using ReactiveUI;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Objects
{
    public class ValidationObject : ReactiveObject
    {
        /// <summary>
        /// Number of messages
        /// </summary>
        public uint Count
        {
            get => _count;
            set => this.RaiseAndSetIfChanged(ref _count, value);
        }

        /// <summary>
        /// Shader wise segment of this object
        /// </summary>
        public ShaderSourceSegment? Segment
        {
            get => _segment;
            set
            {
                if (_segment != value)
                {
                    this.RaiseAndSetIfChanged(ref _segment, value);
                    this.RaisePropertyChanged(nameof(Extract));
                }
            }
        }

        /// <summary>
        /// Associated detail view model, optional
        /// </summary>
        public IValidationDetailViewModel? DetailViewModel
        {
            get => _detailViewModel;
            set => this.RaiseAndSetIfChanged(ref _detailViewModel, value);
        }

        /// <summary>
        /// General contents
        /// </summary>
        public string Content
        {
            get => _content;
            set => this.RaiseAndSetIfChanged(ref _content, value);
        }

        /// <summary>
        /// General contents
        /// </summary>
        public string Extract => Segment?.Extract ?? string.Empty;

        /// <summary>
        /// Increment the count without a reactive raise
        /// </summary>
        /// <param name="silentCount"></param>
        public void IncrementCountNoRaise(uint silentCount)
        {
            _count += silentCount;
        }

        /// <summary>
        /// Notify that the count has changed
        /// </summary>
        public void NotifyCountChanged()
        {
            this.RaisePropertyChanged(nameof(Count));
        }

        /// <summary>
        /// Internal count
        /// </summary>
        private uint _count;

        /// <summary>
        /// Internal content
        /// </summary>
        private string _content = string.Empty;

        /// <summary>
        /// Internal segment
        /// </summary>
        private ShaderSourceSegment? _segment;

        /// <summary>
        /// Internal extract
        /// </summary>
        private string _extract = string.Empty;

        /// <summary>
        /// Internal detail view model
        /// </summary>
        private IValidationDetailViewModel? _detailViewModel;
    }
}
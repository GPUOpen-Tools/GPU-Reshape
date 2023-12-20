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

namespace Studio.ViewModels
{
    public class DialogViewModel : ReactiveObject
    {
        /// <summary>
        /// Result of the dialog
        /// </summary>
        public bool Result
        {
            get => _result;
            set => this.RaiseAndSetIfChanged(ref _result, value);
        }
    
        /// <summary>
        /// Did the user request this message to be hidden?
        /// </summary>
        public bool HideNextTime
        {
            get => _hideNextTime;
            set => this.RaiseAndSetIfChanged(ref _hideNextTime, value);
        }
        
        /// <summary>
        /// Should the "hide next time" query be shown?
        /// </summary>
        public bool ShowHideNextTime
        {
            get => _showHideNextTime;
            set => this.RaiseAndSetIfChanged(ref _showHideNextTime, value);
        }
        
        /// <summary>
        /// Title of the dialog
        /// </summary>
        public string Title
        {
            get => _title;
            set => this.RaiseAndSetIfChanged(ref _title, value);
        }
        
        /// <summary>
        /// Contents of the dialog
        /// </summary>
        public string Content
        {
            get => _content;
            set => this.RaiseAndSetIfChanged(ref _content, value);
        }
        
        /// <summary>
        /// Internal result state
        /// </summary>
        private bool _result;
        
        /// <summary>
        /// Internal hide state
        /// </summary>
        private bool _hideNextTime;
        
        /// <summary>
        /// Internal show hide state
        /// </summary>
        private bool _showHideNextTime;
        
        /// <summary>
        /// Internal title state
        /// </summary>
        private string _title;
        
        /// <summary>
        /// Internal content state
        /// </summary>
        private string _content;
    }
}
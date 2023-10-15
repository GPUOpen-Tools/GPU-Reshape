// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

using System.Collections.ObjectModel;
using ReactiveUI;
using Studio.Services.Suspension;

namespace Studio.ViewModels.Setting
{
    public class BaseSettingViewModel : ReactiveObject, ISettingViewModel
    {
        /// <summary>
        /// Given header
        /// </summary>
        public string Header
        {
            get => _header;
            set => this.RaiseAndSetIfChanged(ref _header, value);
        }

        /// <summary>
        /// Setting visibility
        /// </summary>
        public SettingVisibility Visibility
        {
            get => _visibility;
            set => this.RaiseAndSetIfChanged(ref _visibility, value);
        }

        /// <summary>
        /// Items within
        /// </summary>
        [DataMember(DataMemberContract.SubContracted)]
        public ObservableCollection<ISettingViewModel> Items { get; } = new();

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="name"></param>
        public BaseSettingViewModel(string name = "Unknown")
        {
            Header = name;
        }

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "Settings";

        /// <summary>
        /// Internal visibility
        /// </summary>
        private SettingVisibility _visibility = SettingVisibility.Tool;
    }
}
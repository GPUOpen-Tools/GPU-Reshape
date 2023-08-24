// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
using Avalonia;
using ReactiveUI;
using Studio.Services;
using Studio.Services.Suspension;
using Studio.ViewModels;
using Studio.ViewModels.Setting;
using UIX.Resources;

namespace GRS.Features.Initialization.UIX
{
    public class Data : BaseSettingViewModel
    {
        /// <summary>
        /// Should the user be warned on the initialization feature?
        /// </summary>
        [DataMember]
        public bool WarnOnInitialization
        {
            get => _warnOnInitialization;
            set => this.RaiseAndSetIfChanged(ref _warnOnInitialization, value);
        }
        
        public Data() : base("Initialization")
        {
            
        }

        /// <summary>
        /// Start a conditional warning dialog, does not run if previously marked as hidden
        /// </summary>
        public async Task<bool> ConditionalWarning()
        {
            // Don't warn?
            if (!WarnOnInitialization)
            {
                return false;
            }
            
            // Show window
            var vm = await AvaloniaLocator.Current.GetService<IWindowService>()!.OpenFor<DialogViewModel>(new DialogViewModel()
            {
                Title = Resources.Initialization_Warning_Title,
                Content = Resources.Initialization_Warning_Content,
                ShowHideNextTime = true
            });

            // Failed to open?
            if (vm == null)
            {
                return false;
            }

            // User requested not to warn next time?
            if (vm.HideNextTime)
            {
                WarnOnInitialization = false;
            }
            
            // OK
            return !vm.Result;
        }

        /// <summary>
        /// Internal initialization state
        /// </summary>
        private bool _warnOnInitialization = true;
    }
}
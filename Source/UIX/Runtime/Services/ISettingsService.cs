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

using Studio.ViewModels.Setting;

namespace Studio.Services
{
    public interface ISettingsService
    {
        /// <summary>
        /// Root view model
        /// </summary>
        public ISettingViewModel ViewModel { get; }
    }

    public static class SettingsServiceExtensions
    {
        /// <summary>
        /// Get a setting
        /// </summary>
        public static T? Get<T>(this ISettingsService self) where T : ISettingViewModel
        {
            return self.ViewModel.GetItem<T>();
        }
        
        /// <summary>
        /// Add a setting
        /// </summary>
        public static void Add(this ISettingsService self, ISettingViewModel viewModel)
        {
            self.ViewModel.Items.Add(viewModel);
        }
        
        /// <summary>
        /// Ensure a setting exists, if not, creates the default value
        /// </summary>
        public static void EnsureDefault<T>(this ISettingsService self) where T : ISettingViewModel, new()
        {
            if (self.Get<T>() == null)
            {
                self.Add(new T());
            }
        }
    }
}
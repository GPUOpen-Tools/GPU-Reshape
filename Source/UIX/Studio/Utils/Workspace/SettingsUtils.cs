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

using System.Linq;
using Runtime.ViewModels.Traits;
using Studio.Services;
using Studio.ViewModels;
using Studio.ViewModels.Setting;

namespace Studio.Utils.Workspace
{
    public static class SettingsUtils
    {
        /// <summary>
        /// Create all relevant workspace setting entries for a set of process names, and open the window
        /// </summary>
        public static void OpenWorkspaceSettingsFor(string[] processes)
        {
            SettingsViewModel settings = new();

            // Must have list
            if (settings.SettingViewModel.GetItem<ApplicationListSettingViewModel>() is not { } listSettings)
            {
                Logging.Error("Failed to find application setting list");
                return;
            }

            // Create for each adapter
            foreach (string process in processes)
            {
                // Try to find existing application with matching process
                var appSettings = listSettings.GetFirstItem<ApplicationSettingViewModel>(x => x.ApplicationName == process);
                if (appSettings == null)
                {
                    // None found, create a new one
                    appSettings = new ApplicationSettingViewModel()
                    {
                        ApplicationName = process
                    };
                    
                    // Add to apps
                    listSettings.Items.Add(appSettings);
                }

                // Select this one
                // If multiple adapters, just selects last
                settings.SelectedSettingViewModel = appSettings;
            }

            // Open window
            ServiceRegistry.Get<IWindowService>()?.OpenFor(settings);
        }

        /// <summary>
        /// Create all relevant workspace setting entries for a set of adapters, and open the window
        /// </summary>
        public static void OpenWorkspaceSettingsFor(IWorkspaceAdapter[] adapters)
        {
            OpenWorkspaceSettingsFor(
                adapters
                    .Select(x => x.GetWorkspace().Connection?.Application?.Process ?? null)
                    .Where(x => x != null)
                    .ToArray()!
            );
        }
    }
}

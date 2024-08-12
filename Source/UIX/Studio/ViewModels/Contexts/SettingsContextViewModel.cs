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

using System.Collections.Generic;
using AvaloniaEdit.Utils;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.Extensions;
using Studio.Services;
using Studio.ViewModels.Setting;

namespace Studio.ViewModels.Contexts
{
    public class SettingsContextViewModel : ReactiveObject, IContextViewModel
    {
        /// <summary>
        /// Install a context against a target view model
        /// </summary>
        /// <param name="itemViewModels">destination items list</param>
        /// <param name="targetViewModels">the targets to install for</param>
        public void Install(IList<IContextMenuItemViewModel> itemViewModels, object[] targetViewModels)
        {
            if (targetViewModels.PromoteOrNull<IWorkspaceAdapter>() is not {} adapters)
            {
                return;
            }
            
            itemViewModels.AddRange(new []
            {
                new ContextMenuItemViewModel()
                {
                    Header = "-",
                },
                new ContextMenuItemViewModel()
                {
                    Header = "Settings",
                    Command = ReactiveCommand.Create(() => OnInvoked(adapters))
                }
            });
        }

        /// <summary>
        /// Command implementation
        /// </summary>
        private void OnInvoked(IWorkspaceAdapter[] adapters)
        {
            SettingsViewModel settings = new();

            // Must have list
            if (settings.SettingViewModel.GetItem<ApplicationListSettingViewModel>() is not { } listSettings)
            {
                Studio.Logging.Error("Failed to find application setting list");
                return;
            }

            // Create for each adapter
            foreach (IWorkspaceAdapter adapter in adapters)
            {
                // Validate the connection
                if (adapter.GetWorkspace().Connection?.Application is not { } application)
                {
                    continue;
                }

                // Try to find existing application with matching process
                var appSettings = listSettings.GetFirstItem<ApplicationSettingViewModel>(x => x.ApplicationName == application.Process);
                if (appSettings == null)
                {
                    // None found, create a new one
                    appSettings = new ApplicationSettingViewModel()
                    {
                        ApplicationName = application.Process
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
    }
}
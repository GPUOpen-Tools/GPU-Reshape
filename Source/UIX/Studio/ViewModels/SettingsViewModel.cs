using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia;
using Avalonia.Threading;
using Bridge.CLR;
using Discovery.CLR;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Setting;
using Studio.ViewModels.Tools;

namespace Studio.ViewModels
{
    public class SettingsViewModel : ReactiveObject
    {
        /// <summary>
        /// All workspaces
        /// </summary>
        public ISettingItemViewModel SettingItemViewModel { get; }

        /// <summary>
        /// Currently selected setting
        /// </summary>
        public ISettingItemViewModel? SelectedSettingItem
        {
            get => _selectedSettingItem;
            set => this.RaiseAndSetIfChanged(ref _selectedSettingItem, value);
        }
        
        public SettingsViewModel()
        {
            // Get service
            var settingsService = App.Locator.GetService<ISettingsService>() ?? throw new InvalidOperationException();

            // Set root view model
            SettingItemViewModel = settingsService.ViewModel;

            // Assume first item
            SelectedSettingItem = settingsService.ViewModel.Items[0];
        }

        /// <summary>
        /// Internal selected setting
        /// </summary>
        private ISettingItemViewModel? _selectedSettingItem;
    }
}

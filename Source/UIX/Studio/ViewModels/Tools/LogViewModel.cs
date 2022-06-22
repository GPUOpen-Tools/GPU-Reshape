using System;
using Avalonia;
using Dock.Model.ReactiveUI.Controls;
using Studio.Services;
using Studio.ViewModels.Logging;

namespace Studio.ViewModels.Tools
{
    public class LogViewModel : Tool
    {
        /// <summary>
        /// Data view model
        /// </summary>
        public ILoggingViewModel? LoggingViewModel
        {
            get
            {
                return App.Locator.GetService<ILoggingService>()?.ViewModel;
            }
        }
    }
}

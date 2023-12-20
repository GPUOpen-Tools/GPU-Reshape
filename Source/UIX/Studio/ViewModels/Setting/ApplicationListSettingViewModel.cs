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

using System.Collections.ObjectModel;
using System.IO;
using System.Windows.Input;
using Message.CLR;
using Newtonsoft.Json;
using ReactiveUI;
using Runtime.Models.Setting;
using Runtime.ViewModels.Traits;
using Studio.Services.Suspension;

namespace Studio.ViewModels.Setting
{
    public class ApplicationListSettingViewModel : ReactiveObject, ISettingViewModel, INotifySuspension
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
        public SettingVisibility Visibility { get; } = SettingVisibility.Tool;

        /// <summary>
        /// Items within
        /// </summary>
        [DataMember(DataMemberContract.SubContracted | DataMemberContract.Populate)]
        public ObservableCollection<ISettingViewModel> Items { get; } = new();

        /// <summary>
        /// Currently selected item?
        /// </summary>
        public ISettingViewModel SelectedItem
        {
            get => _selectedItem;
            set => this.RaiseAndSetIfChanged(ref _selectedItem, value);
        }

        /// <summary>
        /// Add a new item
        /// </summary>
        public ICommand Add { get; }
        
        /// <summary>
        /// Close an item
        /// </summary>
        public ICommand Close { get; }
        
        public ApplicationListSettingViewModel()
        {
            // Create commands
            Add = ReactiveCommand.Create(OnAdd);
            Close = ReactiveCommand.Create<ISettingViewModel>(OnClose);
        }

        /// <summary>
        /// Invoked on adds
        /// </summary>
        private void OnAdd()
        {
            Items.Add(new ApplicationSettingViewModel()
            {
                ApplicationName = "Unnamed"
            });
        }

        /// <summary>
        /// Invoked on closes
        /// </summary>
        /// <param name="sender"></param>
        private void OnClose(ISettingViewModel sender)
        {
            Items.Remove(sender);
        }

        /// <summary>
        /// Invoked on data suspensions
        /// </summary>
        public void Suspending()
        {
            // Ensure startup path exists
            if (System.IO.Path.GetDirectoryName(Path) is { } directory)
            {
                Directory.CreateDirectory(directory);
            }
            
            // Ensure startup path exists
            Directory.CreateDirectory(StartupEnvironmentPath);
            
            try
            {
                // Clean up previous environments
                foreach (FileInfo file in new DirectoryInfo(StartupEnvironmentPath).EnumerateFiles())
                {
                    file.Delete(); 
                }
                
                // Create new config object
                ApplicationStartupConfig config = new();
                
                // Handle all applications
                foreach (ApplicationSettingViewModel applicationCandidateSettingViewModel in this.Where<ApplicationSettingViewModel>())
                {
                    // Create environment view
                    var view = new OrderedMessageView<ReadWriteMessageStream>(new ReadWriteMessageStream());
                    
                    // Commit all startup objects
                    foreach (IStartupEnvironmentSetting startupEnvironmentSetting in applicationCandidateSettingViewModel.Where<IStartupEnvironmentSetting>())
                    {
                        startupEnvironmentSetting.Commit(view);
                    }
                    
                    // Key name used for loads
                    string keyName = applicationCandidateSettingViewModel.ApplicationName + ".env";
                    
                    // Designated path
                    string appEnvPath = System.IO.Path.Combine(StartupEnvironmentPath, keyName);
                    
                    // Write startup environment file for given application
                    File.WriteAllBytes(appEnvPath, view.Storage.Data.GetBuffer());
                    
                    // Add keys
                    config.Applications.Add(applicationCandidateSettingViewModel.ApplicationName, keyName);
                }
                
                // Serialize config
                File.WriteAllText(Path, JsonConvert.SerializeObject(config, Formatting.Indented));
            }
            catch
            {
                Studio.Logging.Error("Failed to serialize application startup environment");
            }
        }

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "Applications";

        /// <summary>
        /// Internal selected item
        /// </summary>
        private ISettingViewModel _selectedItem;

        /// <summary>
        /// Path for startup environment directives
        /// </summary>
        private static readonly string Path = System.IO.Path.Combine("Intermediate", "Settings", "ApplicationStartupEnvironments.json");

        /// <summary>
        /// Path for startup environments
        /// </summary>
        private static readonly string StartupEnvironmentPath = System.IO.Path.Combine("Intermediate", "StartupEnvironment");
    }
}
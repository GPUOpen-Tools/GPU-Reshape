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

using System.Collections.ObjectModel;
using System.Windows.Input;
using ReactiveUI;
using Studio.Services.Suspension;

namespace Studio.ViewModels.Setting
{
    public class SourceSettingViewModel : BaseSettingViewModel, ISourceSettingViewModel
    {
        /// <summary>
        /// All source directories
        /// </summary>
        [DataMember]
        public ObservableCollection<string> SourceDirectories { get; } = new();

        /// <summary>
        /// Index sub directories?
        /// </summary>
        [DataMember]
        public bool IndexSubFolders
        {
            get => _indexSubFolders;
            set => this.RaiseAndSetIfChanged(ref _indexSubFolders, value);
        }

        /// <summary>
        /// Indexing extensions
        /// </summary>
        public string Extensions
        {
            get => _extensions;
            set => this.RaiseAndSetIfChanged(ref _extensions, value);
        }

        /// <summary>
        /// Current selected item?
        /// </summary>
        public string SelectedItem
        {
            get => _selectedItem;
            set => this.RaiseAndSetIfChanged(ref _selectedItem, value);
        }

        /// <summary>
        /// Close command
        /// </summary>
        public ICommand Close { get; }
        
        public SourceSettingViewModel() : base("Source Paths")
        {
            // Only appear in inline views
            Visibility = SettingVisibility.Inline;
            
            // Create commands
            Close = ReactiveCommand.Create<string>(OnClose);
        }

        /// <summary>
        /// Invoked on closes
        /// </summary>
        /// <param name="sender"></param>
        private void OnClose(string sender)
        {
            SourceDirectories.Remove(sender);
        }
       
        /// <summary>
        /// Internal selected item
        /// </summary>
        private string _selectedItem;

        /// <summary>
        /// Internal sub-directories
        /// </summary>
        private bool _indexSubFolders = false;

        /// <summary>
        /// Internal extensions
        /// </summary>
        private string _extensions = "hlsl,hlsli,glsl,glsli,usf,ush";
    }
}
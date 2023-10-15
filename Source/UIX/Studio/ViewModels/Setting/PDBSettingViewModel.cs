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
using System.Windows.Input;
using Message.CLR;
using ReactiveUI;
using Studio.Services.Suspension;

namespace Studio.ViewModels.Setting
{
    public class PDBSettingViewModel : BaseSettingViewModel, IPDBSettingViewModel, IStartupEnvironmentSetting
    {
        /// <summary>
        /// All search directories
        /// </summary>
        [DataMember]
        public ObservableCollection<string> SearchDirectories { get; } = new();

        /// <summary>
        /// Search in sub directories?
        /// </summary>
        [DataMember]
        public bool SearchInSubFolders
        {
            get => _searchInSubFolders;
            set => this.RaiseAndSetIfChanged(ref _searchInSubFolders, value);
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
        
        public PDBSettingViewModel() : base("Symbol Paths")
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
            SearchDirectories.Remove(sender);
        }

        /// <summary>
        /// Commit all startup data
        /// </summary>
        /// <param name="view"></param>
        public void Commit(OrderedMessageView<ReadWriteMessageStream> view)
        {
            // Configuration
            var config = view.Add<SetPDBConfigMessage>();
            config.recursive = _searchInSubFolders ? 1 : 0;
            config.pathCount = (uint)SearchDirectories.Count;
            
            // Add all paths
            for (int i = 0; i < SearchDirectories.Count; i++)
            {
                var path = view.Add<SetPDBPathMessage>(new SetPDBPathMessage.AllocationInfo
                {
                    pathLength = (ulong)SearchDirectories[i].Length
                });
                
                // Assign data
                path.index = (uint)i;
                path.path.SetString(SearchDirectories[i]);
            }
            
            // Request re-indexing
            view.Add<IndexPDPathsMessage>();
        }
       
        /// <summary>
        /// Internal selected item
        /// </summary>
        private string _selectedItem;

        /// <summary>
        /// Internal sub-directories
        /// </summary>
        private bool _searchInSubFolders = false;
    }
}
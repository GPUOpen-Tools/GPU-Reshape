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

using System;
using Avalonia;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace
{
    public class WorkspaceViewModel : ReactiveObject, IWorkspaceViewModel
    {
        /// <summary>
        /// Base property collection
        /// </summary>
        public IPropertyViewModel PropertyCollection => _properties;

        /// <summary>
        /// Active connection
        /// </summary>
        public IConnectionViewModel? Connection
        {
            get => _connection;
            set
            {
                this.RaiseAndSetIfChanged(ref _connection, value);
                
                OnConnectionChanged();
            }
        }

        public WorkspaceViewModel()
        {
            // Subscribe on collections behalf
            _properties.CloseCommand = ReactiveCommand.Create(OnClose);
        }

        /// <summary>
        /// Invoked on close
        /// </summary>
        private void OnClose()
        {
            App.Locator.GetService<IWorkspaceService>()?.Remove(this);
        }

        /// <summary>
        /// Invoked when the connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            CreateProperties();
        }

        /// <summary>
        /// Create all properties within
        /// </summary>
        private void CreateProperties()
        {
            // Set connection
            _properties.ConnectionViewModel = _connection;

            // Create descriptor
            _properties.Descriptor = new WorkspaceOverviewDescriptor()
            {
                Workspace = this
            };
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Destroy all properties and services
            DestructPropertyTree(_properties);
            
            // Destroy the connection
            _connection?.Destruct();
        }

        /// <summary>
        /// Destruct a given property tree hierarchically
        /// </summary>
        private void DestructPropertyTree(IPropertyViewModel propertyViewModel)
        {
            // Children first
            // ... I forgot the name, depth-something-something, this is the opposite of job security.
            foreach (IPropertyViewModel child in propertyViewModel.Properties.Items)
            {
                DestructPropertyTree(child);
            }
            
            // Destroy associated services
            foreach (IPropertyService propertyService in propertyViewModel.Services.Items)
            {
                propertyService.Destruct();
            }

            // Finally destruct the property
            propertyViewModel.Destruct();
        }

        /// <summary>
        /// Internal connection state
        /// </summary>
        private IConnectionViewModel? _connection;

        /// <summary>
        /// Internal property state
        /// </summary>
        private WorkspaceCollectionViewModel _properties = new();
    }
}
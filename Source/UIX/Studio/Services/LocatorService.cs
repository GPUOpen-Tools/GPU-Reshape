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

using System;
using System.Collections.Generic;
using Studio.Extensions;
using Studio.Views;

namespace Studio.Services
{
    public class LocatorService : ILocatorService
    {
        public LocatorService()
        {
            // Standard document types
            AddDerived(typeof(ViewModels.Documents.WelcomeDescriptor), typeof(ViewModels.Documents.WelcomeViewModel));
            AddDerived(typeof(ViewModels.Documents.WhatsNewDescriptor), typeof(ViewModels.Documents.WhatsNewViewModel));
            AddDerived(typeof(ViewModels.Documents.WorkspaceOverviewDescriptor), typeof(ViewModels.Documents.WorkspaceOverviewViewModel));
            AddDerived(typeof(ViewModels.Documents.ShaderDescriptor), typeof(ViewModels.Documents.ShaderViewModel));
            AddDerived(typeof(ViewModels.Documents.CodeFileDescriptor), typeof(ViewModels.Documents.CodeViewModel));
            
            // Setting types
            AddDerived(typeof(ViewModels.Setting.DiscoverySettingViewModel), typeof(Views.Setting.DiscoverySettingView));
            AddDerived(typeof(ViewModels.Setting.FeaturesSettingViewModel), typeof(Views.Setting.FeaturesSettingView));
            AddDerived(typeof(ViewModels.Setting.ApplicationListSettingViewModel), typeof(Views.Setting.ApplicationListSettingView));
            AddDerived(typeof(ViewModels.Setting.ApplicationSettingViewModel), typeof(Views.Setting.ApplicationSettingView));
            AddDerived(typeof(ViewModels.Setting.PDBSettingViewModel), typeof(Views.Setting.PDBSettingView));
            AddDerived(typeof(ViewModels.Setting.SourceSettingViewModel), typeof(Views.Setting.SourceSettingView));
            AddDerived(typeof(ViewModels.Setting.GlobalSettingViewModel), typeof(Views.Setting.GlobalSettingView));
            
            // Window types
            AddDerived(typeof(ViewModels.SettingsViewModel), typeof(Views.SettingsWindow));
            AddDerived(typeof(ViewModels.AboutViewModel), typeof(Views.AboutWindow));
            AddDerived(typeof(ViewModels.ConnectViewModel), typeof(Views.ConnectWindow));
            AddDerived(typeof(ViewModels.LaunchViewModel), typeof(Views.LaunchWindow));
            AddDerived(typeof(ViewModels.PipelineFilterViewModel), typeof(Views.PipelineFilter));
            AddDerived(typeof(ViewModels.DialogViewModel), typeof(Views.DialogWindow));
            
            // Report types
            AddDerived(typeof(ViewModels.Reports.InstrumentationReportViewModel), typeof(Views.Reports.InstrumentationReportWindow));
            
            // Object types
            AddDerived(typeof(ViewModels.Workspace.Objects.MissingDetailViewModel), typeof(Views.Workspace.Objects.MissingDetailView));
            AddDerived(typeof(ViewModels.Workspace.Objects.NoDetailViewModel), typeof(Views.Workspace.Objects.NoDetailView));
            AddDerived(typeof(ViewModels.Workspace.Objects.GenericValidationDetailViewModel), typeof(Views.Workspace.Objects.GenericValidationDetailView));
            AddDerived(typeof(ViewModels.Workspace.Objects.ResourceValidationDetailViewModel), typeof(Views.Workspace.Objects.ResourceValidationDetailView));
        }

        /// <summary>
        /// Add a derived type
        /// </summary>
        /// <param name="type">source type</param>
        /// <param name="derived">derived type</param>
        /// <param name="viewType">type of the view</param>
        public void AddDerived(Type type, Type derived, ViewType viewType = ViewType.Primary)
        {
            _types.GetOrAddDefault(viewType).Locators.Add(type, derived);
        }

        /// <summary>
        /// Get the derived type
        /// </summary>
        /// <param name="type">source type</param>
        /// <param name="viewType">type of the view</param>
        /// <returns></returns>
        public Type? GetDerived(Type type, ViewType viewType)
        {
            _types.GetOrAddDefault(viewType).Locators.TryGetValue(type, out Type? derived);
            return derived;
        }

        /// <summary>
        /// Bucket for a type
        /// </summary>
        private class TypeBucket
        {
            public Dictionary<Type, Type> Locators = new();
        }

        /// <summary>
        /// Internal lookup
        /// </summary>
        private Dictionary<ViewType, TypeBucket> _types = new();
    }
}
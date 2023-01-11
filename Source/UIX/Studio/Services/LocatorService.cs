﻿using System;
using System.Collections.Generic;

namespace Studio.Services
{
    public class LocatorService : ILocatorService
    {
        public LocatorService()
        {
            // Standard document types
            AddDerived(typeof(ViewModels.Documents.WelcomeDescriptor), typeof(ViewModels.Documents.WelcomeViewModel));
            AddDerived(typeof(ViewModels.Documents.WorkspaceOverviewDescriptor), typeof(ViewModels.Documents.WorkspaceOverviewViewModel));
            AddDerived(typeof(ViewModels.Documents.ShaderDescriptor), typeof(ViewModels.Documents.ShaderViewModel));
            
            // Window types
            AddDerived(typeof(ViewModels.SettingsViewModel), typeof(Views.SettingsWindow));
            AddDerived(typeof(ViewModels.AboutViewModel), typeof(Views.AboutWindow));
            AddDerived(typeof(ViewModels.ConnectViewModel), typeof(Views.ConnectWindow));
        }

        /// <summary>
        /// Add a derived type
        /// </summary>
        /// <param name="type">source type</param>
        /// <param name="derived">derived type</param>
        public void AddDerived(Type type, Type derived)
        {
            _locators.Add(type, derived);
        }

        /// <summary>
        /// Get the derived type
        /// </summary>
        /// <param name="type">source type</param>
        /// <returns></returns>
        public Type? GetDerived(Type type)
        {
            _locators.TryGetValue(type, out Type? derived);
            return derived;
        }

        /// <summary>
        /// Internal lookup
        /// </summary>
        private Dictionary<Type, Type> _locators = new();
    }
}
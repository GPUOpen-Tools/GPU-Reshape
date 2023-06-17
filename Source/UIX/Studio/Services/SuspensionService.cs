// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Reflection;
using Avalonia;
using Newtonsoft.Json;
using ReactiveUI;
using Studio.Models.Suspension;
using Studio.Services.Suspension;

namespace Studio.Services
{
    public class SuspensionService : ISuspensionService
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="path">serialization path</param>
        public SuspensionService(string path)
        {
            _path = path;
            
            // Load current data
            LoadFrom(path);
            
            // Serialize at intervals
            _serialize
                .Throttle(TimeSpan.FromSeconds(1))
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(_ => WriteTo(_path));
        }

        /// <summary>
        /// Bind an object for type based suspension, members are bound to the cold storage for the underlying type
        /// </summary>
        /// <param name="obj"></param>
        public void BindTypedSuspension(INotifyPropertyChanged obj)
        {
            // Part of the cold storage?
            if (_coldObjects.TryGetValue(obj.GetType(), out ColdObject? coldObject))
            {
                // Load cold members
                foreach (PropertyInfo info in obj.GetType().GetProperties()
                             .Where(p => p.IsDefined(typeof(DataMember), false))
                             .Where(p => coldObject.Storage.ContainsKey(p.Name)))
                {
                    info.SetValue(obj, coldObject.Storage[info.Name]);
                }
            }
            else
            {
                // Create cold storage for type
                _coldObjects[obj.GetType()] = new();
            }
            
            // Subscribe to changes
            obj.PropertyChanged += OnTypedPropertyChanged;
        }

        /// <summary>
        /// Invoked on typed changes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnTypedPropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            // Validate sender
            if (sender == null || !_coldObjects.TryGetValue(sender.GetType(), out ColdObject? coldObject))
            {
                return;
            }
            
            // Validate property
            if (e.PropertyName == null || sender.GetType().GetProperty(e.PropertyName) is not { } info || !info.IsDefined(typeof(DataMember)))
            {
                return;
            }

            // Store value
            coldObject.Storage[e.PropertyName] = info.GetValue(sender);
            
            // Schedule serialization
            _serialize.OnNext(Unit.Default);
        }

        /// <summary>
        /// Load from path
        /// </summary>
        /// <param name="path">file path</param>
        /// <returns></returns>
        private bool LoadFrom(string path)
        {
            if (!File.Exists(path))
            {
                return false;
            }
            
            // Just try to load it, assume invalid if failed
            try
            {
                JsonConvert.PopulateObject(File.ReadAllText(path), this);
            }
            catch
            {
                return false;
            }

            // OK
            return true;
        }

        /// <summary>
        /// Write to path
        /// </summary>
        /// <param name="path">file path</param>
        /// <returns></returns>
        private void WriteTo(string path)
        {
            try
            {
                File.WriteAllText(path, JsonConvert.SerializeObject(this, Formatting.Indented));
            }
            catch
            {
                // ignored
            }
        }
        
        /// <summary>
        /// Current serialization path
        /// </summary>
        private string _path;

        /// <summary>
        /// Serializer event
        /// </summary>
        private Subject<Unit> _serialize = new();

        /// <summary>
        /// All cold objects
        /// </summary>
        [JsonProperty] 
        private Dictionary<Type, ColdObject> _coldObjects = new();
        
        /// <summary>
        /// Default json settings
        /// </summary>
        private readonly JsonSerializerSettings _settings = new()
        {
            TypeNameHandling = TypeNameHandling.All,
            NullValueHandling = NullValueHandling.Ignore
        };
    }

    public static class SuspensionExtensions
    {
        /// <summary>
        /// Bind object to typed suspension
        /// </summary>
        /// <param name="self">object to be bound</param>
        public static void BindTypedSuspension(this INotifyPropertyChanged self)
        {
            App.Locator.GetService<ISuspensionService>()?.BindTypedSuspension(self);
        }
    }
}
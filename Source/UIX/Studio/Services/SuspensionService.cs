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
using System.Collections;
using System.Collections.Generic;
using System.Collections.Specialized;
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
using Runtime.ViewModels.Traits;
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
            // Already has a cold cache?
            if (!_typedSuspensions.TryGetValue(obj.GetType(), out ObjectSuspension? suspension))
            {
                suspension = new();
                suspension.ColdObject = new ColdObject();
                _typedSuspensions[obj.GetType()] = suspension;
            }

            // Assign current host object
            suspension.HotObject = obj;
            
            // Bind the suspension tree from root
            BindSuspensionTree(obj, suspension.ColdObject!, SuspensionFlag.Attach);
        }

        /// <summary>
        /// Bind a suspension tree recursively
        /// </summary>
        /// <param name="notify">current hot object</param>
        /// <param name="coldObject">current cold object</param>
        /// <param name="flags">suspension flags</param>
        private void BindSuspensionTree(INotifyPropertyChanged notify, ColdObject coldObject, SuspensionFlag flags)
        {
            // Load cold members
            foreach (PropertyInfo info in notify.GetType().GetProperties().Where(p => p.IsDefined(typeof(DataMember), false)))
            {
                var contract = info.GetCustomAttribute<DataMember>();
             
                // Is sub-contracted?
                if (contract!.Contract.HasFlag(DataMemberContract.SubContracted))
                {
                    object? propertyObject = info.GetValue(notify);
                    
                    // Sub-contracted types must be enumerable
                    if (propertyObject is not IEnumerable enumerable)
                    {
                        continue;
                    }
                    
                    // Get the storage object
                    var collectionColdObject = coldObject.GetOrAdd<ColdObject>(info.Name, propertyObject.GetType());
                    _objectAssociations[propertyObject] = collectionColdObject;

                    // Visit all contained elements in hot object
                    var set = new HashSet<string>();
                    foreach (object item in enumerable)
                    {
                        string key = GetSuspensionKeyFor(item);
                        
                        // Get or construct storage
                        var itemColdObject = collectionColdObject.GetOrAdd<ColdObject>(key, item.GetType());

                        // If the child can notify, bind suspension on the object
                        if (item is INotifyPropertyChanged notifyChild)
                        {
                            BindSuspensionTree(notifyChild, itemColdObject, flags);
                        }

                        // Mark as contained
                        set.Add(key);
                    }

                    // Does the contract allow population?
                    if (contract.Contract.HasFlag(DataMemberContract.Populate))
                    {
                        // Must be a list
                        if (propertyObject is not IList list)
                        {
                            continue;
                        }

                        // For all suspended objects not already present
                        foreach ((string? key, ColdValue value) in collectionColdObject.Storage.Where(p => !set.Contains(p.Key)).ToList())
                        {
                            // Instantiate the hot object
                            var item = Activator.CreateInstance(value.Type!);
                            if (item == null)
                            {
                                continue;
                            }
                            
                            // Create child cold object
                            var itemColdObject = collectionColdObject.GetOrAdd<ColdObject>(key, item.GetType());

                            // If the child can notify, bind suspension on the object
                            if (item is INotifyPropertyChanged notifyChild)
                            {
                                BindSuspensionTree(notifyChild, itemColdObject, flags);
                            }
                            
                            // Add to list
                            list.Add(item);
                        }
                    }

                    // Subscribe to changes
                    if (flags.HasFlag(SuspensionFlag.Attach) && propertyObject is INotifyCollectionChanged notifyCollection)
                    {
                        notifyCollection.CollectionChanged += OnCollectionChanged;
                    }
                }
                else
                {
                    // Is cached?
                    if (coldObject.TryGetHotValue(info.Name, info.PropertyType, out object? value))
                    {
                        // Are we populating a valid list?
                        if (info.GetValue(notify) is IList list)
                        {
                            // Cold data must be enumerable
                            if (value is not IEnumerable enumerable)
                            {
                                continue;
                            }
                            
                            // Add all cold items
                            foreach (object item in enumerable)
                            {
                                list.Add(item);
                            }
                        }
                        else
                        {
                            // Assume assignable
                            info.SetValue(notify, value);
                        }
                    }
                    else
                    {
                        // Not in the cold cache, add an entry
                        coldObject.Storage[info.Name] = new ColdValue()
                        {
                            Type = null,
                            Value = info.GetValue(notify)
                        };
                    }
                    
                    // Subscribe to changes
                    if (flags.HasFlag(SuspensionFlag.Attach) && info.GetValue(notify) is INotifyCollectionChanged notifyCollection)
                    {
                        notifyCollection.CollectionChanged += OnCollectionChanged;
                    }
                }
            }
            
            // Subscribe to changes
            if (flags.HasFlag(SuspensionFlag.Attach))
            {
                notify.PropertyChanged += OnPropertyChanged;
            }
            
            // Does the object request notifications?
            if (flags.HasFlag(SuspensionFlag.Notify) && notify is INotifySuspension notifySuspension)
            {
                notifySuspension.Suspending();
            }
        }

        /// <summary>
        /// Invoked on typed changes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnPropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            // Validate sender
            if (sender == null)
            {
                return;
            }
            
            // Validate property
            if (e.PropertyName == null || sender.GetType().GetProperty(e.PropertyName) is not { } info || !info.IsDefined(typeof(DataMember)))
            {
                return;
            }

            // Schedule serialization
            _serialize.OnNext(Unit.Default);
        }

        /// <summary>
        /// Invoked on collection changes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            // Validate sender
            if (sender == null)
            {
                return;
            }

            // Does not need to be sub-contracted, f.x. observable collections of primitive data
            if (_objectAssociations.TryGetValue(sender, out ColdObject? collectionColdObject))
            {
                foreach (object? item in e.NewItems ?? Array.Empty<object?>())
                {
                    // Create cold object on new item
                    var itemColdObject = collectionColdObject.GetOrAdd<ColdObject>(GetSuspensionKeyFor(item), item.GetType());
                
                    // If the child can notify, bind suspension on the object
                    if (item is INotifyPropertyChanged notify)
                    {
                        BindSuspensionTree(notify, itemColdObject, SuspensionFlag.Attach);
                    }
                }
            }
            
            // Schedule serialization
            _serialize.OnNext(Unit.Default);
        }

        /// <summary>
        /// Get the suspension key for an object
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        private string GetSuspensionKeyFor(object obj)
        {
            // Has [SuspensionKey] attribute?
            if (obj.GetType().GetProperties().FirstOrDefault(p => p.IsDefined(typeof(SuspensionKey))) is { } key)
            {
                return key.GetValue(obj)?.ToString() ?? throw new Exception("Suspension keys must be valid");
            }
            
            // Assume type
            return obj.GetType().ToString();
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
                Logging.Error("Failed to deserialize suspension data");
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
            // Remove old associations
            _objectAssociations.Clear();
            
            // Rebind, without attachments, the suspension tree
            // Forces the cold object tree to be recreated
            foreach (var (key, value) in _typedSuspensions)
            {
                // No associated hot object?
                if (value.HotObject == null)
                {
                    continue;
                }
                
                // Remove known storage
                value.ColdObject!.Storage.Clear();
                
                // Rebuild tree
                BindSuspensionTree(value.HotObject, value.ColdObject, SuspensionFlag.Notify);
            }
            
            // Ensure path exists
            if (Path.GetDirectoryName(path) is { } directory)
            {
                Directory.CreateDirectory(directory);
            }
            
            try
            {
                File.WriteAllText(path, JsonConvert.SerializeObject(this, Formatting.Indented));
            }
            catch
            {
                Logging.Error("Failed to serialize suspension data");
            }
        }

        private class ObjectSuspension
        {
            /// <summary>
            /// Root cold object
            /// </summary>
            public ColdObject? ColdObject;
            
            /// <summary>
            /// Current hot object
            /// </summary>
            [JsonIgnore]
            public INotifyPropertyChanged? HotObject;
        }

        [Flags]
        private enum SuspensionFlag
        {
            /// <summary>
            /// No flags
            /// </summary>
            None = 0,
            
            /// <summary>
            /// Attach subscribers
            /// </summary>
            Attach = 1,
            
            /// <summary>
            /// Notify listeners
            /// </summary>
            Notify = 2
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
        private Dictionary<Type, ObjectSuspension> _typedSuspensions = new();

        /// <summary>
        /// Current object associations
        /// </summary>
        private Dictionary<object, ColdObject> _objectAssociations = new();
        
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
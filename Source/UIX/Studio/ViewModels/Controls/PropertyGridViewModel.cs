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
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using Avalonia.Controls;
using ReactiveUI;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Controls
{
    public class PropertyGridViewModel : ReactiveObject
    {
        /// <summary>
        /// All categories
        /// </summary>
        public ObservableCollection<PropertyCategoryViewModel> Categories { get; } = new();

        /// <summary>
        /// Shared name section width
        /// </summary>
        public double NameSectionWidth
        {
            get => _nameSectionWidth;
            set => this.RaiseAndSetIfChanged(ref _nameSectionWidth, value);
        }
        
        /// <summary>
        /// Shared value section width
        /// </summary>
        public double ValueSectionWidth
        {
            get => _valueSectionWidth;
            set => this.RaiseAndSetIfChanged(ref _valueSectionWidth, value);
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public PropertyGridViewModel()
        {
            // Dummy data for design
            if (Design.IsDesignMode)
            {
                Categories.Add(new PropertyCategoryViewModel()
                {
                    Name = "Category A",
                    Properties =
                    {
                        new PropertyFieldViewModel()
                        {
                            Name = "A",
                            Value = true
                        },
                        new PropertyFieldViewModel()
                        {
                            Name = "B",
                            Value = true
                        }
                    }
                });

                Categories.Add(new PropertyCategoryViewModel()
                {
                    Name = "Category B",
                    Properties =
                    {
                        new PropertyFieldViewModel()
                        {
                            Name = "C",
                            Value = 2
                        },
                        new PropertyFieldViewModel()
                        {
                            Name = "D",
                            Value = 2
                        }
                    }
                });
            }
        }
        
        /// <summary>
        /// Construct from a set of objects
        /// </summary>
        public PropertyGridViewModel(ReactiveObject[] objects)
        {
            // Add all objects
            foreach (ReactiveObject reactiveObject in objects)
            {
                AddTagged(reactiveObject);
            }
        }

        /// <summary>
        /// Add a tagged reactive object to this property grid
        /// </summary>
        /// <param name="data">data to be added</param>
        public void AddTagged(ReactiveObject data)
        {
            // Get all properties
            IEnumerable<PropertyInfo> properties = data
                .GetType()
                .GetProperties()
                .Where(p => p.IsDefined(typeof(PropertyField), false));

            // All local fields to data
            // Multiple objects may share the same field names, do not overlap
            List<Tuple<PropertyFieldViewModel, PropertyInfo>> localProperties = new();

            foreach (PropertyInfo propertyInfo in properties)
            {
                // Get category, default to Miscellaneous
                PropertyCategoryViewModel category = GetCategoryOrCreate(propertyInfo.GetCustomAttribute<CategoryAttribute>()?.Category ?? "Miscellaneous");

                // Create property
                PropertyFieldViewModel property = new()
                {
                    Name = propertyInfo.Name,
                    Value = propertyInfo.GetValue(data)!
                };

                // Bind from property VM -> data
                property.WhenAnyValue(x => x.Value).Subscribe(x =>
                {
                    propertyInfo.SetValue(data, Convert.ChangeType(x, propertyInfo.PropertyType));
                });
                
                // Keep track of it
                category.Properties.Add(property);
                localProperties.Add(Tuple.Create(property, propertyInfo));
            }

            // Bind from data -> property VM
            data.Changed.Subscribe(change =>
            {
                if (localProperties.FirstOrDefault(p => p.Item2.Name == change.PropertyName) is { } property)
                {
                    property.Item1.Value = property.Item2.GetValue(data)!;
                }
            });
        }

        /// <summary>
        /// Get an existing category or create one
        /// </summary>
        /// <param name="category">name to query</param>
        public PropertyCategoryViewModel GetCategoryOrCreate(string category)
        {
            // Try to search
            if (Categories.FirstOrDefault(x => x.Name == category) is { } property)
            {
                return property;
            }
            
            // None found, create a new one
            property = new PropertyCategoryViewModel()
            {
                Name = category
            };
            
            Categories.Add(property);
            return property;
        }
        
        /// <summary>
        /// Internal shared name width
        /// </summary>
        private double _nameSectionWidth = 50;
        
        /// <summary>
        /// Internal value name width
        /// </summary>
        private double _valueSectionWidth = 50;
    }
}
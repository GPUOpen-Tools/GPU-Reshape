﻿// 
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
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Views.Tools.Property
{
    public class PropertyCollectionGridDescriptor : PropertyDescriptor
    {
        /// <summary>
        /// Exposed property info on parent
        /// </summary>
        public readonly PropertyInfo PropertyInfo;

        /// <summary>
        /// Exposed property field on parent
        /// </summary>
        public readonly PropertyField PropertyField;
        
        /// <summary>
        /// Owning parent
        /// </summary>
        public readonly IPropertyViewModel Parent;

        /// <summary>
        /// Expected converter
        /// </summary>
        public override TypeConverter Converter => _acceptAllConverter;

        /// <summary>
        /// All attributes within this descriptor
        /// </summary>
        public override AttributeCollection Attributes => _attributes;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="parent">Property view model parent component</param>
        /// <param name="propertyInfo">Property view model field property</param>
        public PropertyCollectionGridDescriptor(IPropertyViewModel parent, PropertyInfo propertyInfo, PropertyField propertyField)
            : base(FormatName(propertyInfo.Name), new Attribute[] { new CategoryAttribute(parent.Name) })
        {
            Parent = parent;
            PropertyInfo = propertyInfo;
            PropertyField = propertyField;
            _attributes = new AttributeCollection(propertyInfo.GetCustomAttributes().ToArray());
            _acceptAllConverter = new AcceptAllConverter()
            {
                TargetType = propertyInfo.PropertyType
            };
        }

        /// <summary>
        /// When overridden in a derived class, indicates whether
        /// resetting the <paramref name="component "/>will change the value of the
        /// <paramref name="component"/>.
        /// </summary>
        public override bool CanResetValue(object component)
        {
            return true;
        }

        /// <summary>
        /// When overridden in a derived class, gets the type of the
        /// component this property is bound to.
        /// </summary>
        public override Type ComponentType => Parent.GetType();

        /// <summary>
        /// When overridden in a derived class, gets the current value of the property on a component.
        /// </summary>
        public override object GetValue(object component)
        {
            return PropertyInfo.GetValue(Parent)!;
        }

        /// <summary>
        /// When overridden in a derived class, gets a value indicating whether this
        /// property is read-only.
        /// </summary>
        public override bool IsReadOnly => false;

        /// <summary>
        /// When overridden in a derived class, gets the type of the property.
        /// </summary>
        public override Type PropertyType => PropertyInfo.PropertyType;

        /// <summary>
        /// When overridden in a derived class, indicates whether
        /// resetting the <paramref name="component "/>will change the value of the
        /// <paramref name="component"/>.
        /// </summary>
        public override void ResetValue(object component)
        {
        }

        /// <summary>
        /// When overridden in a derived class, sets the value of
        /// the component to a different value.
        /// </summary>
        public override void SetValue(object component, object value)
        {
            if (!value.GetType().IsAssignableFrom(PropertyInfo.PropertyType))
            {
                throw new Exception("Invalid type to assign");
            }

            PropertyInfo.SetValue(Parent, value);
        }

        /// <summary>
        /// When overridden in a derived class, indicates whether the
        /// value of this property needs to be persisted.
        /// </summary>
        public override bool ShouldSerializeValue(object component)
        {
            return true;
        }

        /// <summary>
        /// Format a given name
        /// </summary>
        private static string FormatName(string memberName)
        {
            return string.Join(" ", Regex.Split(memberName, @"(?<!^)(?=[A-Z])"));
        }

        /// <summary>
        /// Internal converter
        /// </summary>
        private AcceptAllConverter _acceptAllConverter;

        /// <summary>
        /// Internal attributes
        /// </summary>
        private AttributeCollection _attributes;
    }
}
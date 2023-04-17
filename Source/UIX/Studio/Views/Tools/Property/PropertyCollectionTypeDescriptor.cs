using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using Studio.ViewModels.Workspace.Properties;
using Studio.Views.Shader;

namespace Studio.Views.Tools.Property
{
    public class PropertyCollectionTypeDescriptor : CustomTypeDescriptor
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public PropertyCollectionTypeDescriptor(IEnumerable<IPropertyViewModel> properties)
        {
            List<PropertyDescriptor> descriptors = new();

            // Summarize all properties
            foreach (IPropertyViewModel propertyViewModel in properties)
            {
                // Only pool property fields
                foreach (PropertyInfo propertyInfo in propertyViewModel.GetType().GetProperties().Where(p => p.IsDefined(typeof(PropertyField), false)))
                {
                    descriptors.Add(new PropertyCollectionGridDescriptor(propertyViewModel, propertyInfo));
                }
            }
            
            _properties = descriptors.ToArray();
        }
        
        /// <summary>
        /// The GetConverter method returns a type converter for the type this type
        /// descriptor is representing.
        /// </summary>
        public override TypeConverter GetConverter()
        {
            return new AcceptAllConverter();
        }
        
        /// <summary>
        /// The GetProperties method returns a collection of property descriptors
        /// for the object this type descriptor is representing. An optional
        /// attribute array may be provided to filter the collection that is returned.
        /// If no parent is provided,this will return an empty
        /// property collection.
        /// </summary>
        public override PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            return GetProperties();
        }

        /// <summary>
        /// The GetProperties method returns a collection of property descriptors
        /// for the object this type descriptor is representing. An optional
        /// attribute array may be provided to filter the collection that is returned.
        /// If no parent is provided,this will return an empty
        /// property collection.
        /// </summary>
        public override PropertyDescriptorCollection GetProperties()
        {
            return new PropertyDescriptorCollection(base.GetProperties().Cast<PropertyDescriptor>().Concat(_properties).ToArray());
        }

        /// <summary>
        /// The GetPropertyOwner method returns an instance of an object that
        /// owns the given property for the object this type descriptor is representing.
        /// An optional attribute array may be provided to filter the collection that is
        /// returned. Returning null from this method causes the TypeDescriptor object
        /// to use its default type description services.
        /// </summary>
        public override object GetPropertyOwner(PropertyDescriptor pd)
        {
            if (pd is PropertyCollectionGridDescriptor gridDescriptor)
            {
                return gridDescriptor.Parent;
            }

            return this;
        }
        
        /// <summary>
        /// Set of expected properties
        /// </summary>
        private PropertyDescriptor[] _properties;
    }
}
using System;
using System.ComponentModel;
using System.Globalization;
using AvaloniaEdit;

namespace Studio.Views.Tools.Property
{
    public class AcceptAllConverter : TypeConverter
    {
        /// <summary>
        /// Gets a value indicating whether this converter can convert an object in the
        /// given source type to the native type of the converter.
        /// </summary>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            return true;
        }

        /// <summary>
        /// Gets a value indicating whether the given value object is valid for this type.
        /// </summary>
        public override bool IsValid(ITypeDescriptorContext context, object value)
        {
            return true;
        }

        /// <summary>
        /// Gets a collection of standard values for the data type this type converter is designed for.
        /// </summary>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
        {
            return new StandardValuesCollection(new object[] { });
        }

        /// <summary>
        /// Gets a value indicating whether the collection of standard values returned from
        /// <see cref='System.ComponentModel.TypeConverter.GetStandardValues()'/> is an exclusive list.
        /// </summary>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) => true;

        /// <summary>
        /// Gets a value indicating whether this object supports a standard set of values
        /// that can be picked from a list.
        /// </summary>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) => true;
    }
}
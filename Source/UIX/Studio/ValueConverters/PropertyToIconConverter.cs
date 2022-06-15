﻿using System;
using System.Globalization;
using Avalonia.Data.Converters;
using Studio.ViewModels.Tools;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ValueConverters
{
    public class PropertyToIconConverter : IValueConverter
    {
        /// <summary>
        /// Convert source data
        /// </summary>
        /// <param name="value">inbound value</param>
        /// <param name="targetType">expected type</param>
        /// <param name="parameter">originating parameter</param>
        /// <param name="language">culture</param>
        /// <returns>converted value</returns>
        /// <exception cref="NotSupportedException"></exception>
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            IPropertyViewModel property = value as IPropertyViewModel ?? throw new NotSupportedException("Expected property view model in conversion");
            
            switch (property)
            {
                default:
                    return null;
                case WorkspaceCollectionViewModel:
                    return ResourceLocator.GetIcon("StreamOn");
                case PipelineCollectionViewModel:
                case ShaderCollectionViewModel:
                    return ResourceLocator.GetIcon("DotsGrid");
            }
        }

        /// <summary>
        /// Convert destination data
        /// </summary>
        /// <param name="value">inbound value</param>
        /// <param name="targetType">expected type</param>
        /// <param name="parameter">originating parameter</param>
        /// <param name="language">culture</param>
        /// <returns>converted value</returns>
        /// <exception cref="NotSupportedException"></exception>
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
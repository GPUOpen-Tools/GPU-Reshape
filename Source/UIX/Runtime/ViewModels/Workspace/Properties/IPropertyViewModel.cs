using System;
using DynamicData;
using ReactiveUI;

namespace Studio.ViewModels.Workspace.Properties
{
    public interface IPropertyViewModel
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name { get; }
        
        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility { get; }
        
        /// <summary>
        /// Parent property
        /// </summary>
        public IPropertyViewModel? Parent { get; set; }
        
        /// <summary>
        /// Property collection within
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; }
        
        /// <summary>
        /// Service collection within
        /// </summary>
        public ISourceList<IPropertyService> Services { get; }
        
        /// <summary>
        /// Workspace connection view model
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel { get; set; }
    }

    public static class PropertyViewModelExtensions
    {
        /// <summary>
        /// Get a property from a given type
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetProperty<T>(this IPropertyViewModel self) where T : IPropertyViewModel
        {
            foreach (IPropertyViewModel propertyViewModel in self.Properties.Items)
            {
                if (propertyViewModel is T typed)
                {
                    return typed;
                }
            }

            return default;
        }
        
        /// <summary>
        /// Get a service from a given type
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetService<T>(this IPropertyViewModel self) where T : IPropertyService
        {
            foreach (IPropertyService extension in self.Services.Items)
            {
                if (extension is T typed)
                {
                    return typed;
                }
            }

            return default;
        }
    }
}
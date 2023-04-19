using System;
using System.Collections.Generic;
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
        /// Check if a property of type exists
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static bool HasProperty<T>(this IPropertyViewModel self)
        {
            foreach (IPropertyViewModel propertyViewModel in self.Properties.Items)
            {
                if (propertyViewModel is T typed)
                {
                    return true;
                }
            }

            return false;
        }
        
        /// <summary>
        /// Get a property from a given type
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetProperty<T>(this IPropertyViewModel self)
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
        /// Get a property from a given type with a matching predecate
        /// </summary>
        /// <param name="self"></param>
        /// <param name="predecate"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetPropertyWhere<T>(this IPropertyViewModel self, Func<T, bool> predecate)
        {
            foreach (IPropertyViewModel propertyViewModel in self.Properties.Items)
            {
                if (propertyViewModel is T typed && predecate.Invoke(typed))
                {
                    return typed;
                }
            }

            return default;
        }
        
        /// <summary>
        /// Get a set of properties with type
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static IEnumerable<T> GetProperties<T>(this IPropertyViewModel? self)
        {
            foreach (IPropertyViewModel propertyViewModel in self.Properties.Items)
            {
                if (propertyViewModel is T typed)
                {
                    yield return typed;
                }
            }
        }
        
        /// <summary>
        /// Get a service from a given type
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetService<T>(this IPropertyViewModel self)
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
        
        /// <summary>
        /// Get a set of services with type
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static IEnumerable<T> GetServices<T>(this IPropertyViewModel self)
        {
            foreach (IPropertyService extension in self.Services.Items)
            {
                if (extension is T typed)
                {
                    yield return typed;
                }
            }
        }

        /// <summary>
        /// Get the root property
        /// </summary>
        /// <param name="self"></param>
        /// <returns></returns>
        public static IPropertyViewModel GetRoot(this IPropertyViewModel self)
        {
            IPropertyViewModel vm = self;

            // Keep rolling back
            while (vm.Parent != null)
            {
                vm = vm.Parent;
            }

            return vm;
        }

        /// <summary>
        /// Detach a given property from its parent
        /// </summary>
        /// <param name="self"></param>
        public static void DetachFromParent(this IPropertyViewModel self)
        {
            self.Parent?.Properties.Remove(self);
        }
    }
}
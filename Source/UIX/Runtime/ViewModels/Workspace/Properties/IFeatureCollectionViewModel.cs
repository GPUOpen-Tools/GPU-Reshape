using System.Collections.ObjectModel;
using Studio.Models.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Runtime.ViewModels.Workspace.Properties
{
    public interface IFeatureCollectionViewModel : IPropertyViewModel
    {
        /// <summary>
        /// All pooled features
        /// </summary>
        public ObservableCollection<FeatureInfo> Features { get; }
    }

    public static class FeatureCollectionViewModelExtensions
    {
        /// <summary>
        /// Check if a feature is present
        /// </summary>
        /// <param name="self"></param>
        /// <param name="name"></param>
        /// <returns>false if not found</returns>
        public static bool HasFeature(this IFeatureCollectionViewModel self, string name)
        {
            foreach (FeatureInfo info in self.Features)
            {
                if (info.Name == name)
                {
                    return true;
                }
            }

            return false;
        }
        
        /// <summary>
        /// Get a feature
        /// </summary>
        /// <param name="self"></param>
        /// <param name="name"></param>
        /// <returns>null if not found</returns>
        public static FeatureInfo? GetFeature(this IFeatureCollectionViewModel self, string name)
        {
            foreach (FeatureInfo info in self.Features)
            {
                if (info.Name == name)
                {
                    return info;
                }
            }

            return null;
        }
    }
}
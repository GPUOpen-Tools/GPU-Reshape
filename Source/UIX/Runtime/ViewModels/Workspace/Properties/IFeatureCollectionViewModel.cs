// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
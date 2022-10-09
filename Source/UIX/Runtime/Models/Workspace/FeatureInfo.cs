using System;

namespace Studio.Models.Workspace
{
    public struct FeatureInfo
    {
        /// <summary>
        /// Name of the feature
        /// </summary>
        public string Name { get; set; }
        
        /// <summary>
        /// High level information of the feature
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Assigned bit set pattern for this feature
        /// </summary>
        public UInt64 FeatureBit { get; set; }
    }
}
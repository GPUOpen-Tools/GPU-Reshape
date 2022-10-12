using System;

namespace Runtime.Models.Objects
{
    public struct PipelineIdentifier
    {
        /// <summary>
        /// Global UID
        /// </summary>
        public UInt64 GUID { get; set; }

        /// <summary>
        /// Readable descriptor
        /// </summary>
        public string Descriptor { get; set; }
    }
}

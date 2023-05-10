namespace Studio.Models.Workspace.Objects
{
    public struct Resource
    {
        /// <summary>
        /// Physical unique identifier
        /// </summary>
        public uint PUID { get; set; }

        /// <summary>
        /// Version of this resource
        /// </summary>
        public uint Version { get; set; }

        /// <summary>
        /// Lookup key
        /// </summary>
        public ulong Key => (ulong)PUID | ((ulong)Version << 32);

        /// <summary>
        /// Name of this resource
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Width of this resource
        /// </summary>
        public uint Width { get; set; }

        /// <summary>
        /// Height of this resource
        /// </summary>
        public uint Height { get; set; }

        /// <summary>
        /// Depth of this resource
        /// </summary>
        public uint Depth { get; set; }

        /// <summary>
        /// Is this resource a stand-in for a future streamed resource?
        /// </summary>
        public bool IsUnknown { get; set; }
    }
}
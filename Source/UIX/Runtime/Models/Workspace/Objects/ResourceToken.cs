namespace Studio.Models.Workspace.Objects
{
    public struct ResourceToken
    {
        /// <summary>
        /// Packed token
        /// </summary>
        public uint Token { get; set; }

        /// <summary>
        /// Physical unique identifier
        /// </summary>
        public uint PUID => Token & ((1 << 22) - 1);

        /// <summary>
        /// Resource type
        /// </summary>
        public uint Type => (Token >> 22) & ((1 << 2) - 1);

        /// <summary>
        /// Sub-resource base
        /// </summary>
        public uint SRB => (Token >> 24) & ((1 << 8) - 1);
    }
}
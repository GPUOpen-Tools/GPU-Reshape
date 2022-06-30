using System;

namespace Studio.Models.Workspace.Objects
{
    public class CondensedMessage
    {
        /// <summary>
        /// Number of messages
        /// </summary>
        public uint Count;

        /// <summary>
        /// Shader extract
        /// </summary>
        public string Extract;

        /// <summary>
        /// General contents
        /// </summary>
        public string Content;
    }
}
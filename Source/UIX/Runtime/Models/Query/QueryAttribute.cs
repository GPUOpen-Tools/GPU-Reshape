using Avalonia.Media;

namespace Runtime.Models.Query
{
    public struct QueryAttribute
    {
        /// <summary>
        /// Offset of the given query
        /// </summary>
        public int Offset { get; set; }
        
        /// <summary>
        /// Length of the given query
        /// </summary>
        public int Length { get; set; }

        /// <summary>
        /// Attribute key
        /// </summary>
        public string Key { get; set; }

        /// <summary>
        /// Attribute value
        /// </summary>
        public string Value { get; set; }
    }
}
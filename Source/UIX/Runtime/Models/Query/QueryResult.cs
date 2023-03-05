namespace Runtime.Models.Query
{
    public enum QueryResult
    {
        /// <summary>
        /// Parsing successful
        /// </summary>
        OK,
        
        /// <summary>
        /// Invalid syntax
        /// </summary>
        Invalid,
        
        /// <summary>
        /// A duplicate key was found
        /// </summary>
        DuplicateKey
    }
}
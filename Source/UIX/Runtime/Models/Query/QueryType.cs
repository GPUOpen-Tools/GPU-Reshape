using System;
using System.Collections.Generic;

namespace Runtime.Models.Query
{
    public class QueryType
    {
        /// <summary>
        /// All variables within a query
        /// </summary>
        public Dictionary<string, Type> Variables = new();
    }
}
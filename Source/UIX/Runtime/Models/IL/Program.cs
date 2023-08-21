using System.Collections.Generic;

namespace Studio.Models.IL
{
    public class Program
    {
        /// <summary>
        /// Number of allocated identifiers
        /// </summary>
        public int AllocatedIdentifiers;

        /// <summary>
        /// Entry point of the program
        /// </summary>
        public uint EntryPoint;

        /// <summary>
        /// Backend GUID
        /// </summary>
        public int GUID;
        
        /// <summary>
        /// All types
        /// </summary>
        public Type[] Types;
        
        /// <summary>
        /// All constants
        /// </summary>
        public Constant[] Constants;
        
        /// <summary>
        /// All variables
        /// </summary>
        public Variable[] Variables;
        
        /// <summary>
        /// All functions
        /// </summary>
        public Function[] Functions;

        /// <summary>
        /// Identifier lookup
        /// </summary>
        public Dictionary<uint, object> Lookup = new();
    }
}
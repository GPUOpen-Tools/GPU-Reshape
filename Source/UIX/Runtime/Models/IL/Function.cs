namespace Studio.Models.IL
{
    public class Function
    {
        /// <summary>
        /// Identifier of this function
        /// </summary>
        public uint ID;

        /// <summary>
        /// All parameter identifiers of this function
        /// </summary>
        public uint[] Parameters;

        /// <summary>
        /// Underlying type of the function
        /// </summary>
        public FunctionType Type;
        
        /// <summary>
        /// All basic blocks of the function
        /// </summary>
        public BasicBlock[] BasicBlocks;
    }
}
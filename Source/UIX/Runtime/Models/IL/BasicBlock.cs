namespace Studio.Models.IL
{
    public struct BasicBlock
    {
        /// <summary>
        /// Identifier of this basic block
        /// </summary>
        public uint ID;
        
        /// <summary>
        /// All instructions
        /// </summary>
        public Instruction[] Instructions;
    }
}
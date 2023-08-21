namespace Studio.Models.IL
{
    public class Variable
    {
        /// <summary>
        /// Identifier of this variable
        /// </summary>
        public uint ID;

        /// <summary>
        /// Type of this variable
        /// </summary>
        public Type Type;

        /// <summary>
        /// Given address space of this variable
        /// </summary>
        public AddressSpace AddressSpace;
    }
}
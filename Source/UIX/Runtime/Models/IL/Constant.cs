using System;

namespace Studio.Models.IL
{
    public class Constant
    {
        /// <summary>
        /// Identifier of this constant
        /// </summary>
        public uint ID;

        /// <summary>
        /// Kind of this constant
        /// </summary>
        public ConstantKind Kind;
        
        /// <summary>
        /// Type of this constant
        /// </summary>
        public Type Type;
    }
    
    public class UnexposedConstant : Constant
    {
        
    }
    
    public class BoolConstant : Constant
    {
        /// <summary>
        /// Boolean value
        /// </summary>
        public bool Value;
    }
    
    public class IntConstant : Constant
    {
        /// <summary>
        /// Integral value
        /// </summary>
        public Int64 Value;
    }
    
    public class FPConstant : Constant
    {
        /// <summary>
        /// Floating point value
        /// </summary>
        public double Value;
    }

    public class StructConstant : Constant
    {
        /// <summary>
        /// All constant members
        /// </summary>
        public Constant[] Members;
    }

    public class NullConstant : Constant
    {
        
    }

    public class UndefConstant : Constant
    {
        
    }
}
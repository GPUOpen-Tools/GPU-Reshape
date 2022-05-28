#pragma once

namespace IL {
    enum class OpCode {
        /// Invalid op code
        None,

        /// Unexposed by the abstraction layer
        Unexposed,

        /// Literal value
        Literal,

        /// Logical
        Any,
        All,

        /// Math
        Add,
        Sub,
        Div,
        Mul,
        Rem,
        Trunc,

        /// Comparison
        Or,
        And,
        Equal,
        NotEqual,
        LessThan,
        LessThanEqual,
        GreaterThan,
        GreaterThanEqual,

        /// Flow control
        Branch,
        BranchConditional,
        Switch,
        Phi,
        Return,

        /// Bit manipulation
        BitOr,
        BitXOr,
        BitAnd,
        BitShiftLeft,
        BitShiftRight,

        /// Structural
        // ExtractElement,

        /// Casting
        FloatToInt,
        IntToFloat,
        BitCast,

        /// Messages
        Export,

        /// SSA
        Alloca,
        Load,
        Store,
        StoreOutput,

        /// Resource
        StoreTexture,
        LoadTexture,
        StoreBuffer,
        LoadBuffer,
        ResourceSize
    };
}

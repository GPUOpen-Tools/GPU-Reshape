#pragma once

namespace IL {
    enum class OpCode {
        /// Invalid op code
        None,

        /// Unexposed by the abstraction layer
        Unexposed,

        /// Types
        IntType,
        FPType,

        /// Literal value
        Literal,

        /// Math
        Add,
        Sub,
        Div,
        Mul,

        /// Comparison
        Or,
        And,
        Equal,
        NotEqual,
        LessThan,
        LessThanEqual,
        GreaterThan,
        GreaterThanEqual,

        /// Branching
        Branch,
        BranchConditional,

        /// Bit manipulation
        BitOr,
        BitAnd,
        BitShiftLeft,
        BitShiftRight,

        /// Structural
        // ExtractElement,

        /// Messages
        Export,

        /// SSA
        Alloca,
        Load,
        Store,

        /// Texture
        StoreTexture,
        LoadTexture,

        /// Buffer
        StoreBuffer,
    };
}

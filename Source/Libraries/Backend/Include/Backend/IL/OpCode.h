#pragma once

namespace IL {
    enum class OpCode {
        /// Invalid op code
        None,

        /// Unexposed by the abstraction layer
        Unexposed,

        /// Debug
        SourceAssociation,

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

        /// Resource
        StoreTexture,
        LoadTexture,
        StoreBuffer,
        LoadBuffer,
        ResourceSize
    };
}

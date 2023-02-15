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

        /// Special
        IsInf,
        IsNaN,

        /// Flow control
        Select,
        Branch,
        BranchConditional,
        Switch,
        Phi,
        Return,

        /// Atomic
        AtomicOr,
        AtomicXOr,
        AtomicAnd,
        AtomicAdd,
        AtomicMin,
        AtomicMax,
        AtomicExchange,
        AtomicCompareExchange,

        /// Bit manipulation
        BitOr,
        BitXOr,
        BitAnd,
        BitShiftLeft,
        BitShiftRight,

        /// Structural
        AddressChain,
        Extract,
        Insert,

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

        /// Program
        StoreOutput,

        /// Resource
        SampleTexture,
        StoreTexture,
        LoadTexture,
        StoreBuffer,
        LoadBuffer,

        /// Resource queries
        ResourceToken,
        ResourceSize
    };
}

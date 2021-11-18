#pragma once

namespace IL {
    enum class OpCode {
        /// Invalid op code
        None,

        /// Unexposed by the abstraction layer
        Unexposed,

        /// Literal value
        Literal,

        /// Math
        Add,
        // Sub,
        // Div,
        // Mul,

        /// Bit manipulation
        // BitShiftLeft,
        // BitShiftRight,

        /// SSA
        Load,
        Store,

        /// Texture
        LoadTexture,

        /// Buffer
        StoreBuffer,
    };
}

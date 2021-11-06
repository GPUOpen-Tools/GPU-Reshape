#pragma once

namespace IL {
    enum class OpCode {
        /// Invalid op code
        None,

        /// Unexposed by the abstraction layer
        Unexposed,

        /// SSA
        Load,
        Store,

        // Texture
        LoadTexture,
    };
}

#pragma once

// Backend
#include <Backend/IL/Emitter.h>

namespace Backend::IL {
    /// Scalarized coordinates
    struct TexelCoordinateScalar {
        IL::ID x{InvalidID};
        IL::ID y{InvalidID};
        IL::ID z{InvalidID};
    };

    /// Convert a linear index to a 3d coordinate
    /// \param emitter target emitter
    /// \param index linear index
    /// \param width total width of the grid
    /// \param height total height of the grid
    /// \param depth total depth of the grid
    /// \return 3d coordinate
    template<typename T>
    static TexelCoordinateScalar TexelIndexTo3D(IL::Emitter<T>& emitter, IL::ID index, IL::ID width, IL::ID height, IL::ID depth) {
        TexelCoordinateScalar out{};
        out.x = emitter.Rem(index, width);
        out.y = emitter.Rem(emitter.Div(index, width), height);
        out.z = emitter.Div(index, emitter.Mul(width, height));
        return out;
    }
}

#pragma once

namespace Backend::IL {
    enum class AddressSpace {
        Texture,
        Buffer,
        Function,
        Resource,
        Unexposed
    };
}

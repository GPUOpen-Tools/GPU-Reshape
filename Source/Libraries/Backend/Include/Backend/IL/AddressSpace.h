#pragma once

namespace Backend::IL {
    enum class AddressSpace {
        Constant,
        Texture,
        Buffer,
        Function,
        Resource,
        GroupShared,
        RootConstant,
        Output,
        Unexposed
    };
}

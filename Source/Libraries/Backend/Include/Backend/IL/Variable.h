#pragma once

// Backend
#include "ID.h"
#include "AddressSpace.h"

namespace Backend::IL {
    struct Type;

    struct Variable {
        /// Identifier of the argument
        ::IL::ID id{::IL::InvalidID};

        /// Address space of the variable
        AddressSpace addressSpace{AddressSpace::Unexposed};

        /// Argument type
        const Type* type{nullptr};
    };
}

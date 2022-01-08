#pragma once

// Common
#include "CRC.h"
#include "Allocators.h"

// Std
#include <numeric>

// Backwards reference
class Registry;

/// Component id
using ComponentID = uint32_t;

/// ID implementation
#define CLASS_ID(CLASS) \
    static constexpr ComponentID kID = std::integral_constant<ComponentID, crcdetail::compute(#CLASS, sizeof(#CLASS)-1)>::value

/// Interface implementation
#define COMPONENT(CLASS) \
    CLASS_ID(CLASS)

/// Registry component
class IComponent {
public:
    COMPONENT(IComponent);

    /// Query an interface from this component
    /// \param id the interface component id
    /// \return nullptr if not found
    virtual void* QueryInterface(ComponentID id) {
        if (id == kID) {
            return this;
        }

        return nullptr;
    }

    /// Query an interface from this component
    /// \tparam T interface type
    /// \return nullptr if not found
    template<typename T>
    T* QueryInterface() {
        return static_cast<T*>(QueryInterface(T::kID));
    }

    ComponentID componentID;
    Allocators allocators{};
    Registry* registry{nullptr};
};

/// Destruction
inline void destroy(IComponent* object) {
    if (!object)
        return;

    // Copy allocators
    Allocators allocators = object->allocators;

    object->~IComponent();
    allocators.free(object);
}

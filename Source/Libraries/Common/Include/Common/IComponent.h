#pragma once

// Common
#include "CRC.h"
#include "Allocators.h"
#include "Containers/ReferenceObject.h"

// Std
#include <numeric>

// Backwards reference
class Registry;

/// Component id
using ComponentID = uint32_t;

/// Component name
struct ComponentName {
    operator uint32_t() const {
        return id;
    }

    /// Sorting
    bool operator<(const ComponentName &rhs) const {
        return id < rhs.id;
    }

    /// Equality
    bool operator==(const ComponentName &rhs) const {
        return id == rhs.id;
    }

    /// Identifier of the component
    ComponentID id{0};

    /// Name of the component
    const char *name{nullptr};
};

/// ID implementation
#define CLASS_ID(CLASS) \
    static constexpr ComponentID kID = std::integral_constant<ComponentID, crcdetail::compute(#CLASS, sizeof(#CLASS)-1)>::value; \
    static constexpr ComponentName kName = {kID, #CLASS}

/// Interface implementation
#define COMPONENT(CLASS) \
    CLASS_ID(CLASS)

/// Base class for all components and interfaces
class IComponentBase : public ReferenceObject {
public:
    /// Id of this component
    ComponentID componentId;

    /// Name of this component
    ComponentName componentName;

    /// Allotted allocators
    Allocators allocators{};

    /// Creator registry
    Registry *registry{nullptr};

    /// Top address of this component
    void *address{nullptr};
};

/// Interface base class
class IInterface : public virtual IComponentBase {
public:
    /* */
};

/// Component base class
class IComponent : public virtual IComponentBase {
public:
    COMPONENT(IComponent);

    /// Query an interface from this component
    /// \param id the interface component id
    /// \return nullptr if not found
    virtual void *QueryInterface(ComponentID id) {
        if (id == kID) {
            return this;
        }

        // Unknown interface
        return nullptr;
    }
};

/// Typed component base class, provides interface querying support
template<typename T>
class TComponent : public IComponent {
public:
    void *QueryInterface(ComponentID id) override {
        switch (id) {
            case IComponent::kID:
                return static_cast<IComponent*>(this);
            case T::kID:
                return static_cast<T*>(this);
        }

        // Unknown interface
        return nullptr;
    }
};

/// Cast a component
template<typename T, typename U>
T* Cast(U* object) {
    return static_cast<T*>(object->QueryInterface(T::kID));
}

/// Destruction
inline bool destroyRef(IComponentBase *object) {
    if (!object->ReleaseUserNoDestruct()) {
        return false;
    }

    // Copy allocators
    Allocators allocators = object->allocators;

    object->~IComponentBase();
    allocators.free(allocators.userData, object->address, kDefaultAlign);
    return true;
}

// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
    static constexpr ComponentID kID = std::integral_constant<ComponentID, StringCRC32Short(#CLASS, sizeof(#CLASS)-1)>::value; \
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

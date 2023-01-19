#pragma once

// Common
#include "IComponent.h"

/// Component reference, thread safe
template<typename T = IComponent>
struct ComRef {
    /// Null constructor
    ComRef(std::nullptr_t = nullptr) : component(nullptr) {
        /* */
    }

    /// Construct from raw component
    ComRef(T* component) : component(static_cast<IComponentBase*>(component)), object(component) {
        if (component) {
            component->AddUser();
        }
    }

    /// Copy constructor
    ComRef(const ComRef& rhs) : component(rhs.component), object(rhs.object) {
        if (component) {
            component->AddUser();
        }
    }

    /// Move constructor
    ComRef(ComRef&& rhs) : component(rhs.component), object(rhs.object) {
        rhs.component = nullptr;
        rhs.object = nullptr;
    }

    /// Deconstructor
    ~ComRef() {
        Release();
    }

    /// Copy assign
    ComRef& operator=(const ComRef& rhs) {
        if (component) {
            destroyRef(component);
        }

        component = rhs.component;
        object = rhs.object;

        if (component) {
            component->AddUser();
        }

        return *this;
    }

    /// Move assign
    ComRef& operator=(ComRef&& rhs) {
        if (component) {
            destroyRef(component);
        }

        component = rhs.component;
        object = rhs.object;

        rhs.component = nullptr;
        rhs.object = nullptr;
        return *this;
    }

    /// Component reference casting
    template<typename U>
    operator ComRef<U>() const {
        return ComRef<U>(static_cast<U*>(object));
    }

    /// Release this reference
    void Release() {
        if (!component) {
            return;
        }

        destroyRef(component);
        component = nullptr;
        object = nullptr;
    }

    /// Get the unsafe pointer
    T* GetUnsafe() const {
        return object;
    }

    /// Get the unsafe pointer
    T* GetUnsafeAddUser() const {
        component->AddUser();
        return object;
    }

    /// Access component
    T* operator->() const {
        return GetUnsafe();
    }

    /// Check if this reference is valid
    operator bool() const {
        return IsValid();
    }

    /// Check if this reference is valid
    bool IsValid() const {
        return component != nullptr;
    }

private:
    /// Top level object
    T* object{nullptr};

    /// Base component for reference handling, kept separate to avoid downcast need
    IComponentBase* component{nullptr};
};

/// Component reference casting
template<typename T, typename U>
ComRef<T> Cast(const ComRef<U>& object) {
    return Cast<T>(object.GetUnsafe());
}

// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include "IComponent.h"

/// Component reference, thread safe
template<typename T = IComponent>
struct ComRef {
    /// Null constructor
    ComRef(std::nullptr_t = nullptr) : component(nullptr) {
        /* */
    }

    /// Construct from raw component
    ComRef(T* component) : object(component), component(static_cast<IComponentBase*>(component)) {
        if (component) {
            component->AddUser();
        }
    }

    /// Copy constructor
    ComRef(const ComRef& rhs) : object(rhs.object), component(rhs.component) {
        if (component) {
            component->AddUser();
        }
    }

    /// Move constructor
    ComRef(ComRef&& rhs) : object(rhs.object), component(rhs.component) {
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

    /// Equality check
    bool operator==(const ComRef<T>& rhs) const {
        return object == rhs.object;
    }

    /// Inequality check
    bool operator!=(const ComRef<T>& rhs) const {
        return object != rhs.object;
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

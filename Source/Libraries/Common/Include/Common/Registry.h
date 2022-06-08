#pragma once

// Common
#include "Assert.h"
#include "Allocators.h"
#include "IComponent.h"
#include "ComRef.h"
#include "Dispatcher/Mutex.h"

// Std
#include <map>
#include <vector>

/// Component registry
class Registry {
public:
    Registry() {
        allocators.alloc = AllocateDefault;
        allocators.free = FreeDefault;
    }

    Registry(Registry* parent) : parent(parent), allocators(parent->allocators) {

    }

    ~Registry() {
        Release();
    }

    /// No copy
    Registry(const Registry&) = delete;
    Registry(Registry&&) = delete;

    /// No assignment
    Registry& operator=(const Registry&) = delete;
    Registry& operator=(Registry&&) = delete;

    /// Add a component
    /// \param id the id of the component
    /// \param component the component to be added
    template<typename T>
    ComRef<T> Add(T* component) {
        MutexGuard guard(mutex);
        ASSERT(!component->registry, "Component belongs to another registry");

        // Set the registry
        component->allocators    = allocators;
        component->registry      = this;
        component->componentName = T::kName;
        component->address       = component;

        // Add registry user
        component->AddUser();

        // Append
        ASSERT(!components.count(T::kName), "Component already registered");
        components[T::kName] = component;
        linear.push_back(component);

        return component;
    }

    /// Add a component
    /// \param id the id of the component
    /// \param component the component to be added
    template<typename T, typename... A>
    ComRef<T> AddNew(A&&... args) {
        return Add(new (allocators) T(args...));
    }

    /// Add a component
    /// \param id the id of the component
    /// \param component the component to be added
    template<typename T, typename... A>
    ComRef<T> New(A&&... args) {
        auto* component = new (allocators) T(args...);

        // Set the registry
        component->allocators    = allocators;
        component->registry      = this;
        component->componentName = T::kName;
        component->address       = component;

        return component;
    }

    /// Remove a component from this registry
    /// \param component component to be removed
    void Remove(const ComRef<>& component) {
        Remove(component.GetUnsafe());
    }

    /// Remove a component from this registry
    /// \param component component to be removed
    void Remove(IComponent* component) {
        MutexGuard guard(mutex);

        // Remove
        ASSERT(components.count(component->componentName), "Component not registered");
        components.erase(component->componentName);
        linear.erase(std::find(linear.begin(), linear.end(), component));

        // Release reference
        destroyRef(component);
    }

    /// Get a component
    /// \return the component, nullptr if not found
    template<typename T>
    ComRef<T> Get() {
        MutexGuard guard(mutex);
        auto it = components.find(T::kName);
        if (it == components.end()) {
            if (parent) {
                return parent->Get<T>();
            }

            return nullptr;
        }

        return static_cast<T*>(it->second);
    }

    /// Get a component
    /// \param id the id of the component
    /// \return the component, nullptr if not found
    ComRef<> Get(uint32_t id) {
        MutexGuard guard(mutex);
        auto it = components.find({id});
        if (it == components.end()) {
            if (parent) {
                return parent->Get(id);
            }

            return nullptr;
        }

        return it->second;
    }

    void Release() {
        // Release all components, reverse order
        for (auto it = linear.rbegin(); it != linear.rend(); it++) {
            // No longer associated with this registry
            (*it)->registry = nullptr;

            // Copy name
            ComponentName name = (*it)->componentName;

            // Release reference
            destroyRef(*it);

            // Remove from component map
            components.erase(name);
        }
    }

    /// Set the allocators
    void SetAllocators(const Allocators& value) {
        allocators = value;
    }

    /// Get the allocators
    Allocators GetAllocators() const {
        return allocators;
    }

    /// Set the parent
    void SetParent(Registry* value) {
        parent = value;
    }

    /// Get the parent
    Registry* GetParent() const {
        return parent;
    }

private:
    Allocators allocators;

    /// Parent registry
    Registry* parent{nullptr};

    /// Component lookup
    std::map<ComponentName, IComponent*> components;

    /// All components, linear layout
    std::vector<IComponent*> linear;

    /// CLR compliant mutex
    Mutex mutex;
};

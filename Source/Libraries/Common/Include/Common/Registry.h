#pragma once

// Common
#include "Assert.h"
#include "Allocators.h"
#include "IComponent.h"

// Std
#include <map>
#include <mutex>

/// Component registry
class Registry {
public:
    Registry() {
        allocators.alloc = AllocateDefault;
        allocators.free = FreeDefault;
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
    T* Add(T* component) {
        std::lock_guard<std::mutex> guard(mutex);

        // Set the registry
        component->allocators  = allocators;
        component->registry    = this;
        component->componentID = T::kID;

        // Append
        ASSERT(!components.count(T::kID), "Component already registered");
        components[T::kID] = component;
        return component;
    }

    /// Add a component
    /// \param id the id of the component
    /// \param component the component to be added
    template<typename T, typename... A>
    T* AddNew(A&&... args) {
        return Add(new (allocators) T(args...));
    }

    /// Add a component
    /// \param id the id of the component
    /// \param component the component to be added
    template<typename T, typename... A>
    T* New(A&&... args) {
        auto* component = new (allocators) T(args...);

        // Set the registry
        component->allocators  = allocators;
        component->registry    = this;
        component->componentID = T::kID;

        return component;
    }

    /// Add a component
    /// \param id the id of the component
    /// \param component the component to be added
    IComponent* Add(uint32_t id, IComponent* component) {
        std::lock_guard<std::mutex> guard(mutex);

        // Set the state
        component->allocators  = allocators;
        component->registry    = this;
        component->componentID = id;

        // Append
        ASSERT(!components.count(id), "Component already registered");
        components[id] = component;
        return component;
    }

    /// Remove a component from this registry
    /// \param component component to be removed
    void Remove(IComponent* component) {
        std::lock_guard<std::mutex> guard(mutex);

        // Remove
        ASSERT(components.count(component->componentID), "Component not registered");
        components.erase(component->componentID);
    }

    /// Remove a component from this registry
    /// \param component component to be removed
    void RemoveDestroy(IComponent* component) {
        Remove(component);
        destroy(component);
    }

    /// Get a component
    /// \return the component, nullptr if not found
    template<typename T>
    T* Get() {
        return static_cast<T*>(Get(T::kID));
    }

    /// Get a component
    /// \param id the id of the component
    /// \return the component, nullptr if not found
    IComponent* Get(uint32_t id) {
        std::lock_guard<std::mutex> guard(mutex);
        auto it = components.find(id);
        if (it == components.end()) {
            return nullptr;
        }

        return it->second;
    }

    /// Set the allocators
    void SetAllocators(const Allocators& value) {
        allocators = value;
    }

    /// Get the allocators
    Allocators GetAllocators() const {
        return allocators;
    }

private:
    Allocators allocators;

    std::map<uint32_t, IComponent*> components;
    std::mutex                      mutex;
};

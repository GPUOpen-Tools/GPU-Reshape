#pragma once

// Common
#include <Common/Allocators.h>
#include <Common/Assert.h>

// Std
#include <vector>

/// Stack based container, with optional heap fallback
template<typename T, size_t STACK_LENGTH>
struct TrivialStackVector {
    /// Initialize to size
    TrivialStackVector(size_t size = 0) : data(stack) {
        Resize(size);
    }

    /// Copy from other
    TrivialStackVector(const TrivialStackVector& other) : TrivialStackVector(other.size) {
        std::memcpy(data, other.Data(), sizeof(T) * size);
    }

    /// Move from other
    TrivialStackVector(TrivialStackVector&& other) : fallback(std::move(other.fallback)), size(other.size) {
        if (other.data != other.stack) {
            data = fallback.data();
        } else {
            std::memcpy(stack, other.stack, sizeof(T) * other.size);
            data = stack;
        }
    }

    /// Swap this container
    /// \param other swap destination
    void Swap(TrivialStackVector& other) {
        // TODO: Do a pass on how safe this is... The handling of "data" smells incredibly unsafe.

        const bool isLhsFallback = data == fallback.data();
        const bool isRhsFallback = other.data == other.fallback.data();

        std::swap(size, other.size);
        std::swap(stack, other.stack);
        fallback.swap(other.fallback);

        data = isRhsFallback ? fallback.data() : stack;
        other.data = isLhsFallback ? other.fallback.data() : other.stack;
    }

    /// Assign copy from other
    TrivialStackVector& operator=(const TrivialStackVector& other) {
        Resize(other.Size());
        std::memcpy(data, other.Data(), sizeof(T) * size);
        return *this;
    }

    /// Assign move from other
    TrivialStackVector& operator=(TrivialStackVector&& other) {
        size = other.size;
        fallback = std::move(other.fallback);

        if (other.data != other.stack) {
            data = fallback.data();
        } else {
            std::memcpy(stack, other.stack, sizeof(T) * other.size);
            data = stack;
        }

        return *this;
    }

    /// Create a detached allocation
    /// \param allocators the allocators
    /// \return new allocation, lifetime up to caller
    T* DetachAllocation(const Allocators& allocators) {
        T* items = new (allocators) T[size];
        std::memcpy(items, data, sizeof(T) * size);
        return items;
    }

    /// Resize this container
    /// \param length the desired length
    void Resize(size_t length) {
        if (data != stack || length > STACK_LENGTH) {
            fallback.resize(length);
            data = fallback.data();
        }

        size = length;
    }

    /// Reserve this container
    /// \param length the desired allocated length
    void Reserve(size_t length) {
        if (size > STACK_LENGTH) {
            fallback.reserve(length);
            data = fallback.data();
        }
    }

    /// Add a value to this container
    /// \param value the value to be added
    T& Add(const T& value = {}) {
        if (data != stack || size >= STACK_LENGTH) {
            if (data == stack) {
                fallback.insert(fallback.end(), data, data + size);
            }

            fallback.push_back(value);
            data = fallback.data();
        } else {
            data[size] = value;
        }

        size++;

        return data[size - 1];
    }

    /// Add a value to this container
    /// \param value the value to be added
    T& Insert(const T* location, const T& value = {}) {
        size_t offset = location - data;

        if (data != stack || size >= STACK_LENGTH) {
            if (data == stack) {
                fallback.insert(fallback.end(), data, data + size);
            }

            fallback.insert(fallback.begin() + offset, value);
            data = fallback.data();
        } else {
            if (offset != size) {
                std::memmove(data + offset + 1, data + offset, sizeof(T) * size - offset);
            }
            data[offset] = value;
        }

        size++;

        return data[offset];
    }

    /// Size of this container
    size_t Size() const {
        return size;
    }

    /// Get the element at a given index
    T& operator[](uint32_t i) {
        ASSERT(i < size, "Index out of bounds");
        return data[i];
    }

    /// Get the element at a given index
    const T& operator[](uint32_t i) const {
        ASSERT(i < size, "Index out of bounds");
        return data[i];
    }

    /// Get the data address of this container
    T* Data() {
        return data;
    }

    /// Get the data address of this container
    const T* Data() const {
        return data;
    }

    T* begin() {
        return data;
    }

    const T* begin() const {
        return data;
    }

    T* end() {
        return data + size;
    }

    const T* end() const {
        return data + size;
    }

private:
    /// Current size of this container
    size_t size{0};

    /// Current base address of the data
    T* data{nullptr};

    /// Initial stack data
    T stack[STACK_LENGTH];

    /// Heap fallback
    std::vector<T> fallback;
};
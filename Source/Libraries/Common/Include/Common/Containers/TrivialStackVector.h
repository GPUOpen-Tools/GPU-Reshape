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
#include <Common/Allocator/Vector.h>
#include <Common/Assert.h>

// Std
#include <vector>

/// Stack based container, with optional heap fallback
template<typename T, size_t STACK_LENGTH>
struct TrivialStackVector {
    /// Initialize to size
    TrivialStackVector(const Allocators& allocators = {}) : data(stack), fallback(allocators) {
        /** */
    }

    /// Copy from other
    TrivialStackVector(const TrivialStackVector& other) : data(stack), fallback(other.fallback.get_allocator()) {
        Resize(other.size);
        std::memcpy(data, other.Data(), sizeof(T) * size);
    }

    /// Move from other
    TrivialStackVector(TrivialStackVector&& other) noexcept : size(other.size), fallback(other.fallback) {
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

            if (data == stack) {
                std::memcpy(fallback.data(), stack, sizeof(T) * std::min(size, length));
            }
            
            data = fallback.data();
        }

        size = length;
    }

    /// Clear all elements
    void Clear() {
        size = 0;
    }

    /// Reserve this container
    /// \param length the desired allocated length
    void Reserve(size_t length) {
        if (size > STACK_LENGTH && length > fallback.capacity()) {
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
                std::memmove(data + offset + 1, data + offset, sizeof(T) * (size - offset));
            }
            data[offset] = value;
        }

        size++;

        return data[offset];
    }

    /// Pop the last value
    /// \return 
    T PopBack() {
        ASSERT(size > 0, "Out of bounds");
        return data[--size];
    }

    /// Size of this container
    size_t Size() const {
        return size;
    }

    /// Get the element at a given index
    T& operator[](size_t i) {
        ASSERT(i < size, "Index out of bounds");
        return data[i];
    }

    /// Get the element at a given index
    const T& operator[](size_t i) const {
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
    Vector<T> fallback;
};
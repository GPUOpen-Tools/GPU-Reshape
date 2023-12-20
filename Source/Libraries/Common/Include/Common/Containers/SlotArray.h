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

// Std
#include <vector>
#include <type_traits>

template<typename T>
struct SlotArray {
    static constexpr bool kIsIndirect = std::is_pointer_v<std::remove_const_t<T>>;

    /// Add an element to this array
    /// \param value
    void Add(const T& value) {
        uint64_t id = array.size();
        auto&& emplaced = array.emplace_back(value);
        IdOf(emplaced) = id;
    }

    /// Remove an element from this array
    /// \param value
    void Remove(const T& value) {
        size_t id = IdOf(value);

        // Swap last element with slot
        if (array.size() > 1) {
            std::swap(array[id], array.back());
            IdOf(array[id]) = id;
        }

        // Remove element
        array.pop_back();
    }

    /// Remove all elements passing a condition
    /// \param functor called functor per element
    template<typename F>
    void RemoveIf(F&& functor) {
        for (size_t i = 0; i < array.size(); i++) {
            if (functor(array[i])) {
                Remove(array[i]);
            }
        }
    }

    /// Get element at index
    /// \param i index
    /// \return value
    T& operator[](size_t i) {
        return array.at(i);
    }

    /// Get element at index
    /// \param i index
    /// \return value
    const T& operator[](size_t i) const {
        return array.at(i);
    }

    /// Size of this container
    /// \return
    uint64_t Size() const {
        return array.size();
    }

    typename std::vector<T>::iterator begin() {
        return array.begin();
    }

    typename std::vector<T>::const_iterator begin() const {
        return array.begin();
    }

    typename std::vector<T>::iterator end() {
        return array.end();
    }

    typename std::vector<T>::const_iterator end() const {
        return array.end();
    }

private:
    /// Get id of a given value
    uint64_t& IdOf(T& value) {
        if constexpr(kIsIndirect) {
            return value->id;
        } else {
            return value.id;
        }
    }

    /// Get id of a given value
    const uint64_t& IdOf(const T& value) {
        if constexpr(kIsIndirect) {
            return value->id;
        } else {
            return value.id;
        }
    }

private:
    /// Underlying container
    std::vector<T> array;
};
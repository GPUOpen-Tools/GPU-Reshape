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
#pragma once

#include <cstdint>

template<typename T, typename STAMP>
struct UniqueID {
    using UnderlyingType = T;

    constexpr UniqueID() : valid(false) {

    }

    constexpr explicit UniqueID(T value) : valid(true), value(value) {

    }

    constexpr operator T() const {
        return value;
    }

    constexpr bool IsValid() const {
        return valid;
    }

    static constexpr UniqueID Invalid() {
        return UniqueID{};
    }

    bool valid;
    T value;
};

#define UNIQUE_ID(TYPE, NAME) \
    struct UniqueID##NAME##Stamp; \
    using NAME = UniqueID<TYPE, UniqueID##NAME##Stamp>

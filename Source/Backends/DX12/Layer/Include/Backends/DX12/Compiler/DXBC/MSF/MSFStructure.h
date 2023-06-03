#pragma once

// Common
#include <Common/Containers/TrivialStackVector.h>

struct MSFFile {
    MSFFile(const Allocators& allocators) : data(allocators) {

    }

    /// Cast this file
    template<typename T>
    const T* As() const {
        ASSERT(sizeof(T) == data.size(), "MSF size mismatch");
        return reinterpret_cast<const T*>(data.data());
    }

    /// All contained data
    Vector<uint8_t> data;
};

struct MSFDirectory {
    MSFDirectory(const Allocators& allocators) : files(allocators) {

    }

    /// All files within this directory
    Vector<MSFFile> files;
};

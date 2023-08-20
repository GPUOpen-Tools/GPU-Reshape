#pragma once

// System
#include <handleapi.h>

/// Object wrapper for releasing handles
template<typename T>
struct Win32Object {
    /// Constructor
    Win32Object(T handle) : handle(handle) {
        
    }
    
    /// Destructor
    ~Win32Object() {
        if (handle) {
            CloseHandle(handle);
        }
    }

    /// No copy/move
    Win32Object(const Win32Object&) = delete;
    Win32Object(Win32Object&&) = delete;

    /// No copy/move assignment
    Win32Object& operator=(const Win32Object&) = delete;
    Win32Object& operator=(Win32Object&&) = delete;

    /// Implicit object
    operator T() const {
        return handle;
    }

    /// Underlying object
    T handle;
};

/// Standard objects
using Win32Handle = Win32Object<HANDLE>;
using Win32Module = Win32Object<HMODULE>;

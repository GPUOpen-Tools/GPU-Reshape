#pragma once

// Platform
#ifdef _WIN32
#include <Windows.h>
#else
#error Not implemented
#endif

// Std
#include <string>

/// Simple library loading
struct Library {
    /// Load a library
    /// \param path path of the library
    /// \return success state
    bool Load(const std::string& path) {
#ifdef _WIN32
        sourcePath = path + ".dll";
#else
        sourcePath = path + ".a";
#endif

        handle = LoadLibrary(sourcePath.c_str());
        return handle != nullptr;
    }

    /// Free a library
    void Free() {
        FreeLibrary(handle);
        handle = nullptr;
    }

    /// Get a library function
    /// \tparam T function type
    /// \param name name of the function
    /// \return nullptr if not found
    template<typename T>
    T GetFn(const char* name) const {
        return reinterpret_cast<T>(GetProcAddress(handle, name));
    }

    /// Get the path of this library
    /// \return
    const std::string& Path() const {
        return sourcePath;
    }

    /// Is this library in a good state
    bool IsGood() const {
        return handle != nullptr;
    }

private:
    std::string sourcePath;
    HINSTANCE handle{nullptr};
};

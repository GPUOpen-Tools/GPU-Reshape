// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

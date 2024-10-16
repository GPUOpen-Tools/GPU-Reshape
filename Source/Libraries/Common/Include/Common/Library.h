// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
#include <string>

/// Simple library loading
struct Library {
    /// Load a library
    /// \param path path of the library
    /// \return success state
    bool Load(const std::string& path);

    /// Free a library
    void Free();

    /// Get a library function
    /// \tparam T function type
    /// \param name name of the function
    /// \return nullptr if not found
    template<typename T>
    T GetProcAddr(const char* name) const {
        return reinterpret_cast<T>(GetProcAddr(name));
    }

    /// Get a library function
    /// \param name name of the function
    /// \return nullptr if not found
    void* GetProcAddr(const char* name) const;

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

    /// Platform specific handle
    void* handle{nullptr};
};

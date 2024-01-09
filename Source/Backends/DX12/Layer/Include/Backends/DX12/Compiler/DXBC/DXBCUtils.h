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
#include <cstdint>

// Forward declarations
struct DXILDigest;

/// Scan for the DXBC shader digest
/// \param byteCode shader code
/// \param byteLength byte length of shader code
/// \param digest given digest
/// \return false if failed
bool ScanDXBCShaderDigest(const void* byteCode, uint64_t byteLength, DXILDigest& digest);

/// Get the dxbc shader hash or compute one
/// \param byteCode shader code
/// \param byteLength byte length of shader code
/// \return shader hash
uint32_t GetOrComputeDXBCShaderHash(const void* byteCode, uint64_t byteLength);

/// Check if a given byte code is DXBC native
/// \param byteCode shader code
/// \param byteLength byte length of shader code
/// \return true if pure DXBC
bool IsDXBCNative(const void* byteCode, uint64_t byteLength);

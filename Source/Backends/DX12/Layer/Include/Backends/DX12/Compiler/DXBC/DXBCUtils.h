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

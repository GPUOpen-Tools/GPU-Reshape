﻿#include <Backends/DX12/RelFunTBL.h>

// Common
#include <Common/FileSystem.h>

// System
#include <Windows.h>

// Std
#include <cstdint>
#include <fstream>

int main(int32_t argc, const char *const *argv) {
    /**
        Note for readers.

        While I like to reinvent the wheel at times, this proved to be the simplest solution. The problem is that the addresses of
        LoadLibraryA, W, ... differ between x64 and Wow64, determining the x64 offsets is easy, Wow64 not so much. I had multiple
        implementations that inspected the binary headers, export tables, and whatnot, while they *get* there, they are frankly
        overcomplicated compared to the below. Given that this is called so infrequently, I believe the IO overhead to be justified.
    */

    // Open stream
    std::ofstream stream(GetIntermediatePath("Interop") / "X86RelFunTbl.dat", std::ios_base::binary);
    if (!stream.good()) {
        return 1u;
    }

    // Local table
    X86RelFunTBL table;
    table.kernel32LoadLibraryA = reinterpret_cast<uint32_t>(LoadLibraryA);
    table.kernel32FreeLibrary  = reinterpret_cast<uint32_t>(FreeLibrary);

    // Write table
    stream.write(reinterpret_cast<const char*>(&table), sizeof(table));

    // OK
    return 0u;
}
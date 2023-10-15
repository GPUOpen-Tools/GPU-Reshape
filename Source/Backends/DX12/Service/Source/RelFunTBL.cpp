// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

#include <Backends/DX12/RelFunTBL.h>

// Common
#include <Common/FileSystem.h>

// System
#include <Windows.h>

// Std
#include <cstdint>
#include <fstream>

// Validation
static_assert(sizeof(void*) == 4, "Unexpected architecture");

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
    table.kernel32LoadLibraryA = reinterpret_cast<uint32_t>(reinterpret_cast<void*>(LoadLibraryA));
    table.kernel32FreeLibrary  = reinterpret_cast<uint32_t>(reinterpret_cast<void*>(FreeLibrary));

    // Write table
    stream.write(reinterpret_cast<const char*>(&table), sizeof(table));

    // OK
    return 0u;
}

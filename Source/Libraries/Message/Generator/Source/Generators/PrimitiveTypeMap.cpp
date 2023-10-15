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

#include <Generators/PrimitiveTypeMap.h>

PrimitiveTypeMap::PrimitiveTypeMap() {
    types = {
        {"uint64", {"uint64_t", "UInt64", 8}},
        {"uint32", {"uint32_t", "UInt32", 4}},
        {"uint16", {"uint16_t", "UInt16", 2}},
        {"uint8",  {"uint8_t",  "UInt8",  1}},
        {"int64",  {"int64_t",  "Int64",  8}},
        {"int32",  {"int32_t",  "Int32",  4}},
        {"int16",  {"int16_t",  "Int16",  2}},
        {"int8",   {"int8_t",   "Int8",   1}},
        {"float",  {"float",    "float",  4}},
        {"double", {"double",   "double", 8}},
        {"bool",   {"int32_t",  "Int32",  4}},
    };
}

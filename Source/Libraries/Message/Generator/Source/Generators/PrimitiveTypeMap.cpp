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

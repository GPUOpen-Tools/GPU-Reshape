#include <Generators/PrimitiveTypeMap.h>

PrimitiveTypeMap::PrimitiveTypeMap() {
    types = {
        {"uint64", {"uint64_t", 8}},
        {"uint32", {"uint32_t", 4}},
        {"uint16", {"uint16_t", 2}},
        {"uint8",  {"uint8_t",  1}},
        {"int64",  {"int64_t",  8}},
        {"int32",  {"int32_t",  4}},
        {"int16",  {"int16_t",  2}},
        {"int8",   {"int8_t",   1}},
        {"float",  {"float",    4}},
        {"double", {"double",   8}},
        {"bool",   {"int32_t",  4}},
    };
}

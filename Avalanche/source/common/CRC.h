#pragma once

#include "Detail/CRC16.h"
#include "Detail/CRC64.h"

/**
 * Compute the 16bit CRC hash
 * @param[in] str the string to hash, null terminated
 */
constexpr std::uint16_t ComputeCRC16(const char *str, std::uint16_t crc = ~static_cast<std::uint16_t>(0)) 
{
    return *str == 0 ? ~crc : ComputeCRC16(str + 1, Detail::kCRC16[static_cast<unsigned char>(crc) ^ *str] ^ (crc >> 8));
}

/**
 * Compute the 64bit CRC hash
 * @param[in] str the string to hash, null terminated
 */
constexpr std::uint64_t ComputeCRC64(const char *str, std::uint64_t crc = ~static_cast<std::uint64_t>(0)) 
{
    return *str == 0 ? ~crc : ComputeCRC64(str + 1, Detail::kCRC64[static_cast<unsigned char>(crc) ^ *str] ^ (crc >> 8));
}

/**
 * Compute the 64bit CRC hash
 * @param[in] str the beginning of the string to hash
 * @param[in] str the end of the string to hash
 */
constexpr std::uint64_t ComputeCRC64(const int8_t *start, const int8_t * end, std::uint64_t crc = ~static_cast<std::uint64_t>(0)) 
{
	return start == end ? ~crc : ComputeCRC64(start + 1, end, Detail::kCRC64[static_cast<unsigned char>(crc) ^ *start] ^ (crc >> 8));
}

/**
 * Compute the 64bit CRC hash of a buffer
 * @param[in] start the beginning of the buffer
 * @param[in] end the end of the buffer
 */
constexpr std::uint64_t ComputeCRC64Buffer(const uint8_t *start, const uint8_t * end)
{
	std::uint64_t crc = ~static_cast<std::uint64_t>(0);
	for (; start != end; start++)
	{
		crc = Detail::kCRC64[static_cast<unsigned char>(crc) ^ *start] ^ (crc >> 8);
	}
	return ~crc;
}

/**
 * Compute the 64bit CRC hash of an object
 * @param[in] data the object to compute the hash of
 */
template<typename T>
constexpr std::uint64_t ComputeCRC64Buffer(const T& data)
{
	return ComputeCRC64Buffer(reinterpret_cast<const uint8_t*>(&data), reinterpret_cast<const uint8_t*>(&data) + sizeof(T));
}

/**
 * 16bit CRC string literal
 */
constexpr std::uint16_t operator "" _crc16(const char *str, size_t) 
{
    return ComputeCRC16(str);
}

/**
 * 64bit CRC string literal
 */
constexpr std::uint64_t operator "" _crc64(const char *str, size_t) 
{
    return ComputeCRC64(str);
}

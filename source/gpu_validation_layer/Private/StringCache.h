#pragma once

#include <CRC.h>
#include <map>
#include <string>

struct SStringCache
{
    template<size_t LENGTH>
    const char* Get(const char (&buffer)[LENGTH])
    {
        uint64_t crc = ComputeCRC64(reinterpret_cast<const int8_t*>(buffer), reinterpret_cast<const int8_t*>(buffer + LENGTH));

        auto it = m_StringCache.find(crc);
        if (it == m_StringCache.end())
        {
            return (m_StringCache[crc] = buffer).c_str();
        }

        return (*it).second.c_str();
    }

    std::map<uint64_t, std::string> m_StringCache;
};
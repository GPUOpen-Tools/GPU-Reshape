#pragma once

// Serialization write helper
template<typename T>
inline void Write(std::ofstream& stream, const T& value)
{
    stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

// Serialization read helper
template<typename T>
inline void Read(std::ifstream& stream, T& value)
{
    stream.read(reinterpret_cast<char*>(&value), sizeof(T));
}
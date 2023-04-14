#include <Backend/StartupEnvironment.h>

// Common
#include <Common/Alloca.h>
#include <Common/FileSystem.h>
#include <Common/GlobalUID.h>

// Std
#include <filesystem>
#include <fstream>

using namespace Backend;

bool StartupEnvironment::LoadFromEnvironment(MessageStream& stream) {
    // Try to get length
    uint64_t length{0};
    if (getenv_s(&length, nullptr, 0u, kEnvKey) || !length) {
        return true;
    }

    // Get path data
    auto* path = ALLOCA_ARRAY(char, length);
    if (getenv_s(&length, path, length, kEnvKey)) {
        return false;
    }

    // Invalid state if doesn't exist
    if (!std::filesystem::exists(path)) {
        return false;
    }

    // Open file and get size
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Assign schema
    stream.ValidateOrSetSchema(OrderedMessageSchema::GetSchema());
    
    // Try to read stream
    if (!file.read(reinterpret_cast<char*>(stream.ResizeData(size)), size)) {
        return false;
    }

    // OK
    return true;
}

std::string StartupEnvironment::WriteEnvironment(const MessageStream& stream) {
    // Create new environment path
    std::filesystem::path path = GetIntermediatePath("StartupEnvironment") / (GlobalUID::New().ToString() + ".env");

    // Write to output
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(stream.GetDataBegin()), stream.GetByteSize());

    // OK
    return path.string();
}

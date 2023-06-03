#include <Common/FileSystem.h>

// Std
#include <fstream>

// System
#if defined(_MSC_VER)
#include <Windows.h>
#else
#include <unistd.h>
#endif

std::filesystem::path GetCurrentExecutableDirectory() {
#if defined(_MSC_VER)
    wchar_t buffer[FILENAME_MAX]{};
    GetModuleFileNameW(nullptr, buffer, FILENAME_MAX);
    if (!*buffer) {
        return {};
    }
#else
    char buffer[FILENAME_MAX];
    const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (length <= 0) {
        return {};
    }
#endif

    return std::filesystem::path(buffer).parent_path();
}

std::string GetCurrentExecutableName() {
#if defined(_MSC_VER)
    wchar_t buffer[FILENAME_MAX]{};
    GetModuleFileNameW(nullptr, buffer, FILENAME_MAX);
    if (!*buffer) {
        return {};
    }
#else
    char buffer[FILENAME_MAX];
    const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (length <= 0) {
        return {};
    }
#endif

    return std::filesystem::path(buffer).filename().string();
}

#if defined(_MSC_VER)
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif

std::filesystem::path GetCurrentModuleDirectory() {
#if defined(_MSC_VER)
    wchar_t buffer[FILENAME_MAX]{};

    GetModuleFileNameW((HINSTANCE)&__ImageBase, buffer, FILENAME_MAX);
    if (!*buffer) {
        return {};
    }

    return std::filesystem::path(buffer).parent_path();
#else
#    error Not implemented
#endif
}

void CreateDirectoryTree(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
}

std::filesystem::path GetBaseModuleDirectory() {
    std::filesystem::path path = GetCurrentModuleDirectory();

    // Walk back until root
    while (!std::filesystem::exists(path / "Plugins")) {
        path = path.parent_path();
    }

    return path;
}

std::filesystem::path GetIntermediatePath(const std::string_view &category) {
    std::filesystem::path path = GetBaseModuleDirectory();

    // Append
    path /= "Intermediate";
    path /= category;

    // Make sure it exists
    CreateDirectoryTree(path);
    return path;
}

std::filesystem::path GetIntermediateDebugPath() {
    return GetIntermediatePath("Debug");
}

std::filesystem::path GetIntermediateCachePath() {
    return GetIntermediatePath("Cache");
}

std::string ReadAllText(const std::filesystem::path &path) {
    std::ifstream stream(path);

    // Determine size
    stream.seekg(0, std::ios::end);
    size_t size = stream.tellg();

    // Read contents
    std::string str(size, ' ');
    stream.seekg(0);
    stream.read(&str[0], size);
    return str;
}

static bool IsPathDelim(char _char) {
    switch (_char) {
        default:
            return false;
        case '\\':
        case '/':
            return true;
    }
}

std::string SanitizePath(const std::string_view &view) {
    std::string path;
    path.reserve(view.length());

    // Was the last character a delimiter?
    bool wasDelim = false;

    // Construct new string
    for (size_t i = 0; i < view.length(); i++) {
        bool isDelim = IsPathDelim(view[i]);
        
        // Skip if the last was a delimiter
        if (wasDelim && isDelim) {
            continue;
        }

        // Set state
        wasDelim = isDelim;
        
        // Sanitize delims
        char _char;
        switch (view[i]) {
            default:
                _char = view[i];
                break;
            case '/':
                _char = '\\';
                break;
        }

        // Add character
        path.insert(path.end(), _char);
    }

    // OK
    return path;
}

bool PathExists(const std::string& view) {
    struct stat buffer;
    return (stat (view.c_str(), &buffer) == 0); 
}

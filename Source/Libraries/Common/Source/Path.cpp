#include <Common/Path.h>

// System
#if defined(_MSC_VER)
#include <Windows.h>
#else
#include <unistd.h>
#endif

std::filesystem::path CurrentExecutableDirectory() {
#if defined(_MSC_VER)
    wchar_t buffer[FILENAME_MAX]{};
    GetModuleFileNameW(nullptr, buffer, FILENAME_MAX);
    if (!*buffer) {
        return {};
    }

    return std::filesystem::path(buffer).parent_path().string();
#else
    char buffer[FILENAME_MAX];
    const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if (length <= 0) {
        return {};
    }

    return std::filesystem::path(std::string(path, length)).parent_path().string();
#endif
}

#if defined(_MSC_VER)
extern "C" IMAGE_DOS_HEADER __ImageBase;
#endif

std::filesystem::path CurrentModuleDirectory() {
#if defined(_MSC_VER)
    wchar_t buffer[FILENAME_MAX]{};
    char path[MAX_PATH]{};
    HMODULE hm = NULL;

    GetModuleFileNameW((HINSTANCE)&__ImageBase, buffer, FILENAME_MAX);
    if (!*buffer) {
        return {};
    }

    return std::filesystem::path(buffer).parent_path().string();
#else
#    error Not implemented
#endif
}

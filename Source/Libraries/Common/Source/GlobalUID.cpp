#include <Common/GlobalUID.h>

#ifdef _WIN64

#include <objbase.h>

#else
#include <uuid/uuid.h>
#endif

#ifdef _WIN64
using LPFN_GUIDFromString = BOOL (WINAPI *)(LPCTSTR, LPGUID);

LPFN_GUIDFromString GetGUIDFromStringFPTR() {
    LPFN_GUIDFromString pGUIDFromString = nullptr;

    HINSTANCE hInst = LoadLibrary(TEXT("shell32.dll"));
    if (hInst) {
        pGUIDFromString = (LPFN_GUIDFromString) GetProcAddress(hInst, MAKEINTRESOURCEA(703));
        if (pGUIDFromString) {
            return pGUIDFromString;
        }
    }

    if (!pGUIDFromString) {
        hInst = LoadLibrary(TEXT("Shlwapi.dll"));
        if (hInst) {
            pGUIDFromString = (LPFN_GUIDFromString) GetProcAddress(hInst, MAKEINTRESOURCEA(269));
            if (pGUIDFromString) {
                return pGUIDFromString;
            }
        }
    }

    return nullptr;
}
#endif

GlobalUID GlobalUID::New() {
#ifdef _WIN64
    static_assert(sizeof(GUID) == sizeof(UUID), "Unexpected size");

    union {
        GlobalUID uuid;
        GUID guid;
    } u{};

    // Create GUID
    CoCreateGuid(&u.guid);
    return u.uuid;
#else
    UUID uuid;
    uuid_generate(uuid.uuid);
    return uuid;
#endif
}

GlobalUID GlobalUID::FromString(const std::string_view &view) {
#ifdef _WIN64
    union {
        GlobalUID uuid;
        GUID guid;
    } u{};

    // Attempt to get the converter
    static LPFN_GUIDFromString fptr = GetGUIDFromStringFPTR();
    if (!fptr) {
        return GlobalUID();
    }

    // Convert
    if (!fptr(view.data(), &u.guid)) {
        std::memset(u.uuid.uuid, 0x0, sizeof(uuid));
    }

    return u.uuid;
#else
    UUID uuid;
    uuid_generate(uuid.uuid);
    return uuid;
#endif
}

std::string GlobalUID::ToString() const {
#ifdef _WIN64
    GUID guid;
    std::memcpy(&guid, uuid, sizeof(guid));

    // To string (wchar_t)
    OLECHAR *guidString;
    StringFromCLSID(guid, &guidString);

    // Get the converted length
    int32_t wideLength = static_cast<int32_t>(std::wcslen(guidString));

    // Get the UTF8 length
    int32_t length = WideCharToMultiByte(CP_UTF8, 0, guidString, wideLength, nullptr, 0, nullptr, nullptr);

    // To UTF8
    std::string str(length, 0);
    WideCharToMultiByte(CP_UTF8, 0, guidString, wideLength, &str[0], length, nullptr, nullptr);
    
    // OK
    CoTaskMemFree(guidString);
    return str;
#else
#    error Not implemented
#endif
}

GUID GlobalUID::AsPlatformGUID() const {
    GUID guid;
    std::memcpy(&guid, uuid, sizeof(guid));

    return guid;
}

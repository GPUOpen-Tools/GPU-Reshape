// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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

#include <Common/GlobalUID.h>

#ifdef GRS_WINDOWS
#include <objbase.h>
#else // GRS_WINDOWS
#include <uuid/uuid.h>
#endif // GRS_WINDOWS

#ifdef GRS_WINDOWS
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
#ifdef GRS_WINDOWS
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
#ifdef GRS_WINDOWS
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
#ifdef GRS_WINDOWS
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

GlobalUID GlobalUID::FromPlatformGUID(const GUID &value) {
    GlobalUID uid;
    std::memcpy(&uid.uuid, &value, sizeof(value));
    
    return uid;
}

GUID GlobalUID::AsPlatformGUID() const {
    GUID guid;
    std::memcpy(&guid, uuid, sizeof(guid));

    return guid;
}

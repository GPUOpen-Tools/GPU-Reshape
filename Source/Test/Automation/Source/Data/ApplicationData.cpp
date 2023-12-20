#include <Test/Automation/Data/ApplicationData.h>

#if _WIN32
#include <Windows.h>
#endif // _WIN32

bool ApplicationData::IsAlive() const {
#if _WIN32
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, processID);

    // Wait with zero time
    DWORD result = WaitForSingleObject(process, 0);
    CloseHandle(process);

    // Check if it was successfully signalled
    return result == WAIT_TIMEOUT;
#endif // _WIN32
}

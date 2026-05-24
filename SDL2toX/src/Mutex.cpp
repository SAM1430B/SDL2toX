#include "Mutex.h"
#include <stdio.h>

namespace SDL2toX {

    HANDLE g_hMutex = nullptr;

    bool CreateSingleInstanceMutex() {
        char mutexName[256];
        snprintf(mutexName, sizeof(mutexName), "SDL2toX_Mutex_PID_%lu", GetCurrentProcessId());

        g_hMutex = CreateMutexA(NULL, FALSE, mutexName);

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            if (g_hMutex) {
                CloseHandle(g_hMutex);
                g_hMutex = nullptr;
            }
            return false;
        }

        return true;
    }

    void ReleaseSingleInstanceMutex() {
        if (g_hMutex) {
            CloseHandle(g_hMutex);
            g_hMutex = nullptr;
        }
    }
}
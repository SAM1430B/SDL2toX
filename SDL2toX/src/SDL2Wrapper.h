#pragma once
#include <windows.h>

namespace SDL2Wrapper
{
    bool Initialize();
    void Quit();
    DWORD WINAPI SDL2toX_SDL2GetState(DWORD dwUserIndex, void* pState, bool extended);
    DWORD WINAPI SDL2toX_SDL2SetState(DWORD dwUserIndex, void* pVibration);
    DWORD WINAPI SDL2toX_SDL2GetCapabilities(DWORD dwUserIndex, DWORD dwFlags, void* pCapabilities);
}
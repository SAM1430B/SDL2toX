#include <Windows.h>
#include <Xinput.h>
#include "../XInputExports.h"

bool WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    static HMODULE hSystem = nullptr;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        // Load dll
        char path[MAX_PATH];
        if (GetFileAttributesA("xinput1_4.Chained.dll") != INVALID_FILE_ATTRIBUTES)
        {
            // If xinput1_4.Chained.dll exists, load it
            strcpy_s(path, "xinput1_4.Chained.dll");
        }
        else
        {
            // Load system xinput1_4.dll
            GetSystemDirectoryA(path, MAX_PATH);
            strcat_s(path, "\\xinput1_4.dll");
        }
        hSystem = LoadLibraryA(path);

        if (hSystem)
        {
            SystemXInputGetState = (FPXGetState)GetProcAddress(hSystem, "XInputGetState");
            SystemXInputSetState = (FPXSetState)GetProcAddress(hSystem, "XInputSetState");
            SystemXInputGetCapabilities = (FPXGetCapabilities)GetProcAddress(hSystem, "XInputGetCapabilities");
            SystemXInputEnable = (FPXEnable)GetProcAddress(hSystem, "XInputEnable");
            SystemXInputGetBatteryInformation = (FPXGetBatteryInformation)GetProcAddress(hSystem, "XInputGetBatteryInformation");
            SystemXInputGetKeystroke = (FPXGetKeystroke)GetProcAddress(hSystem, "XInputGetKeystroke");
            SystemXInputGetDSoundAudioDeviceGuids = (FPXGetDSoundAudioDeviceGuids)GetProcAddress(hSystem, "XInputGetDSoundAudioDeviceGuids");
            SystemXInputGetAudioDeviceIds = (FPXGetAudioDeviceIds)GetProcAddress(hSystem, "XInputGetAudioDeviceIds");
			SystemXInputGetStateEx = (FPXGetStateEx)GetProcAddress(hSystem, (LPCSTR)100);
			SystemXInputWaitForGuideButton = (FPXWaitForGuideButton)GetProcAddress(hSystem, (LPCSTR)101);
			SystemXInputCancelGuideButtonWait = (FPXCancelGuideButtonWait)GetProcAddress(hSystem, (LPCSTR)102);
			SystemXInputPowerOffController = (FPXPowerOffController)GetProcAddress(hSystem, (LPCSTR)103);
        }
        else
        {
            //Log() << "Failed to load xinput1_4 library!";
        }

        // Load SDL2toX dll
#ifdef _WIN64
        if (LoadLibraryA("SDL2toX.64.dll"));
        //else Log() << "Failed to load SDL2toX.64.dll. Error code: " << GetLastError();
#else
        if (LoadLibraryA("SDL2toX.32.dll"));
        //else Log() << "Failed to load SDL2toX.32.dll. Error code: " << GetLastError();
#endif
        break;

    case DLL_PROCESS_DETACH:
        if (hSystem)
        {
            FreeLibrary(hSystem);
            hSystem = nullptr;
        }
        break;
    }

    return true;
}
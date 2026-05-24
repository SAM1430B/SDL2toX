#include "INISettings.h"
#include <windows.h>

namespace SDL2toX
{
    unsigned int INISettings::controllerIndex = 1;
    unsigned int INISettings::controllerIndex2 = 2;
    unsigned int INISettings::controllerIndex3 = 3;
    unsigned int INISettings::controllerIndex4 = 4;

    void INISettings::LoadSettings()
    {
        char iniPath[MAX_PATH];
        GetModuleFileNameA(NULL, iniPath, MAX_PATH);
        char* lastSlash = strrchr(iniPath, '\\');
        if (lastSlash != nullptr)
        {
            *(lastSlash + 1) = '\0';
            strcat_s(iniPath, MAX_PATH, "SDL2toX.ini");
        }

        controllerIndex = GetPrivateProfileIntA("ControllerIndex", "XInputController1", 1, iniPath);
        controllerIndex2 = GetPrivateProfileIntA("ControllerIndex", "XInputController2", 2, iniPath);
        controllerIndex3 = GetPrivateProfileIntA("ControllerIndex", "XInputController3", 3, iniPath);
        controllerIndex4 = GetPrivateProfileIntA("ControllerIndex", "XInputController4", 4, iniPath);
    }
}
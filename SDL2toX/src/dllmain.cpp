#include <windows.h>
#include "Mutex.h"
#include "XinputHook.h"

DWORD WINAPI InjectionThread(LPVOID lpParameter)
{
    SDL2toX::XinputHook::InstallImpl();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:

        if (!SDL2toX::CreateSingleInstanceMutex()) {
            return FALSE;
        }

        DisableThreadLibraryCalls(hModule);

        CreateThread(nullptr, 0, InjectionThread, hModule, 0, nullptr);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        SDL2toX::XinputHook::UninstallImpl();
        break;
    }

    return TRUE;
}
/*
Copyright (c) 2021 Ilyaki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "XinputHook.h"
#include "SDL2Wrapper.h"
#include "INISettings.h"
#include <Xinput.h>
#include <detours.h>
#include <stdio.h>
#include <utility>

namespace SDL2toX
{
    typedef DWORD(WINAPI* t_XInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState);
    typedef DWORD(WINAPI* t_XInputGetStateEx)(DWORD dwUserIndex, void* pState);
    typedef DWORD(WINAPI* t_XInputSetState)(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
    typedef DWORD(WINAPI* t_XInputGetCapabilities)(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);

    t_XInputGetState TrueXInputGetState = nullptr;
    t_XInputGetStateEx TrueXInputGetStateEx = nullptr;
    t_XInputSetState TrueXInputSetState = nullptr;
    t_XInputGetCapabilities TrueXInputGetCapabilities = nullptr;

    t_XInputGetState DetouredGetState[5] = { nullptr };
    t_XInputGetStateEx DetouredGetStateEx[5] = { nullptr };
    t_XInputSetState DetouredSetState[5] = { nullptr };
    t_XInputGetCapabilities DetouredGetCapabilities[5] = { nullptr };

    static thread_local bool isSdlInternalCall = false;

    // Translate XInput User Index to the Custom SDL Index via INISettings
    // INISettings::controllerIndex 0 means user wants no controller on this game
    inline std::pair<bool, unsigned int> GetTargetControllerIndex(DWORD dwUserIndex)
    {
        if (dwUserIndex == 0 || dwUserIndex == XUSER_INDEX_ANY)
            return { INISettings::controllerIndex != 0, INISettings::controllerIndex - 1 };

        if (dwUserIndex == 1)
            return { INISettings::controllerIndex2 != 0, INISettings::controllerIndex2 - 1 };

        if (dwUserIndex == 2)
            return { INISettings::controllerIndex3 != 0, INISettings::controllerIndex3 - 1 };

        if (dwUserIndex == 3)
            return { INISettings::controllerIndex4 != 0, INISettings::controllerIndex4 - 1 };

        return { false, 0 };
    }

    DWORD WINAPI Hook_XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState)
    {
        if (dwUserIndex >= 4 && dwUserIndex != XUSER_INDEX_ANY)
            return ERROR_BAD_ARGUMENTS;

        if (isSdlInternalCall)
        {
            if (TrueXInputGetState) return TrueXInputGetState(dwUserIndex, pState);
            return ERROR_DEVICE_NOT_CONNECTED;
        }

        auto target = GetTargetControllerIndex(dwUserIndex);
        if (!target.first) return ERROR_DEVICE_NOT_CONNECTED;

        isSdlInternalCall = true;
        DWORD result = SDL2Wrapper::SDL2toX_SDL2GetState(target.second, pState, false);
        isSdlInternalCall = false;
        return result;
    }

    DWORD WINAPI Hook_XInputGetStateEx(DWORD dwUserIndex, void* pState)
    {
        if (dwUserIndex >= 4 && dwUserIndex != XUSER_INDEX_ANY)
            return ERROR_BAD_ARGUMENTS;

        if (isSdlInternalCall)
        {
            if (TrueXInputGetStateEx) return TrueXInputGetStateEx(dwUserIndex, pState);
            return ERROR_DEVICE_NOT_CONNECTED;
        }

        auto target = GetTargetControllerIndex(dwUserIndex);
        if (!target.first) return ERROR_DEVICE_NOT_CONNECTED;

        isSdlInternalCall = true;
        DWORD result = SDL2Wrapper::SDL2toX_SDL2GetState(target.second, pState, true);
        isSdlInternalCall = false;
        return result;
    }

    DWORD WINAPI Hook_XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
    {
        if (dwUserIndex >= 4 && dwUserIndex != XUSER_INDEX_ANY)
            return ERROR_BAD_ARGUMENTS;

        if (isSdlInternalCall)
        {
            if (TrueXInputSetState) return TrueXInputSetState(dwUserIndex, pVibration);
            return ERROR_DEVICE_NOT_CONNECTED;
        }

        auto target = GetTargetControllerIndex(dwUserIndex);
        if (!target.first) return ERROR_DEVICE_NOT_CONNECTED;

        isSdlInternalCall = true;
        DWORD result = SDL2Wrapper::SDL2toX_SDL2SetState(target.second, pVibration);
        isSdlInternalCall = false;
        return result;
    }

    DWORD WINAPI Hook_XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities)
    {
        if (dwUserIndex >= 4 && dwUserIndex != XUSER_INDEX_ANY)
            return ERROR_BAD_ARGUMENTS;

        if (isSdlInternalCall)
        {
            if (TrueXInputGetCapabilities) return TrueXInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);
            return ERROR_DEVICE_NOT_CONNECTED;
        }

        auto target = GetTargetControllerIndex(dwUserIndex);
        if (!target.first) return ERROR_DEVICE_NOT_CONNECTED;

        isSdlInternalCall = true;
        DWORD result = SDL2Wrapper::SDL2toX_SDL2GetCapabilities(target.second, dwFlags, pCapabilities);
        isSdlInternalCall = false;
        return result;
    }

    void XinputHook::InstallImpl()
    {
        // Load the INI settings first
        INISettings::LoadSettings();

        // Throw a warning message if SDL2.dll is not found for non-static build.
#ifndef SDL2toX_Static
        HMODULE hSDL = LoadLibraryA("SDL2.dll");
        if (!hSDL)
        {
            char exePath[MAX_PATH];
            GetModuleFileNameA(NULL, exePath, MAX_PATH);

            char* exeName = strrchr(exePath, '\\');
            if (exeName != nullptr) {exeName++;}
            else {exeName = exePath;}

            char boxTitle[MAX_PATH + 50];
            snprintf(boxTitle, sizeof(boxTitle), "%s - SDL2toX Initialization Failed", exeName);
            MessageBoxA(NULL,
                "SDL2toX cannot be used because SDL2.dll is missing from the application directory.\n\n",
                boxTitle,
                MB_ICONWARNING | MB_OK | MB_TOPMOST | MB_SETFOREGROUND);

            return;
        }
#endif
      
        const wchar_t* xinputModules[] = {
            L"xinput1_4.dll", L"xinput1_3.dll", L"xinput1_2.dll", L"xinput1_1.dll", L"xinput9_1_0.dll"
        };

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        for (int i = 0; i < 5; i++)
        {
            // Force load the DLL so it gets hooked even if the game loads it dynamically later
            HMODULE hXInput = GetModuleHandleW(xinputModules[i]);
            if (!hXInput) hXInput = LoadLibraryW(xinputModules[i]);

            if (hXInput)
            {
                DetouredGetState[i] = (t_XInputGetState)GetProcAddress(hXInput, "XInputGetState");
                DetouredSetState[i] = (t_XInputSetState)GetProcAddress(hXInput, "XInputSetState");
                DetouredGetCapabilities[i] = (t_XInputGetCapabilities)GetProcAddress(hXInput, "XInputGetCapabilities");
                DetouredGetStateEx[i] = (t_XInputGetStateEx)GetProcAddress(hXInput, (LPCSTR)100);

                if (DetouredGetState[i]) DetourAttach(&(PVOID&)DetouredGetState[i], Hook_XInputGetState);
                if (DetouredSetState[i]) DetourAttach(&(PVOID&)DetouredSetState[i], Hook_XInputSetState);
                if (DetouredGetCapabilities[i]) DetourAttach(&(PVOID&)DetouredGetCapabilities[i], Hook_XInputGetCapabilities);
                if (DetouredGetStateEx[i]) DetourAttach(&(PVOID&)DetouredGetStateEx[i], Hook_XInputGetStateEx);

            }
        }

        LONG error = DetourTransactionCommit();
        if (error != NO_ERROR)
        {
            fprintf(stderr, "Detours Hooking Failed: %ld\n", error);
        }

        // Copy them safely for SDL to use internally.
        for (int i = 0; i < 5; i++)
        {
            if (!TrueXInputGetState && DetouredGetState[i]) TrueXInputGetState = DetouredGetState[i];
            if (!TrueXInputSetState && DetouredSetState[i]) TrueXInputSetState = DetouredSetState[i];
            if (!TrueXInputGetCapabilities && DetouredGetCapabilities[i]) TrueXInputGetCapabilities = DetouredGetCapabilities[i];
            if (!TrueXInputGetStateEx && DetouredGetStateEx[i]) TrueXInputGetStateEx = DetouredGetStateEx[i];
        }
    }

    void XinputHook::UninstallImpl()
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        for (int i = 0; i < 5; i++)
        {
            if (DetouredGetState[i]) DetourDetach(&(PVOID&)DetouredGetState[i], Hook_XInputGetState);
            if (DetouredSetState[i]) DetourDetach(&(PVOID&)DetouredSetState[i], Hook_XInputSetState);
            if (DetouredGetCapabilities[i]) DetourDetach(&(PVOID&)DetouredGetCapabilities[i], Hook_XInputGetCapabilities);
            if (DetouredGetStateEx[i]) DetourDetach(&(PVOID&)DetouredGetStateEx[i], Hook_XInputGetStateEx);
        }

        DetourTransactionCommit();

        SDL2Wrapper::Quit();
    }
}
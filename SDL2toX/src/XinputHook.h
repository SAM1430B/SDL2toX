#pragma once
#include <windows.h>

namespace SDL2toX
{
    class XinputHook
    {
    public:
        static void InstallImpl();
        static void UninstallImpl();
    };
}
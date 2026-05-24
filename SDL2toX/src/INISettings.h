#pragma once

namespace SDL2toX
{
    class INISettings
    {
    public:
        static unsigned int controllerIndex;
        static unsigned int controllerIndex2;
        static unsigned int controllerIndex3;
        static unsigned int controllerIndex4;

        static void LoadSettings();
    };
}
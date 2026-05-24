#pragma once
#include <windows.h>

namespace SDL2toX {

    bool CreateSingleInstanceMutex();
    void ReleaseSingleInstanceMutex();
}
#include "SDL2Wrapper.h"
#include <SDL.h>
#include <Xinput.h>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace SDL2Wrapper
{
    // Map XInput user index to SDL_GameController objects
    static std::unordered_map<DWORD, SDL_GameController*> connectedControllers;

    static std::once_flag initFlag;
    static bool initSuccess = false;

    typedef struct _XINPUT_GAMEPAD_EX
    {
        WORD  wButtons;
        BYTE  bLeftTrigger;
        BYTE  bRightTrigger;
        SHORT sThumbLX;
        SHORT sThumbLY;
        SHORT sThumbRX;
        SHORT sThumbRY;
        DWORD dwUnknown;
    } XINPUT_GAMEPAD_EX, * PXINPUT_GAMEPAD_EX;

    bool Initialize()
    {
        std::call_once(initFlag, []() {
            SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
            SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");

            /*SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STADIA, "1");
            SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4, "1");
            SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH, "1");*/

            if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0)
            {
                // Optionally load community mappings for generic controllers
                // https://github.com/mdqinc/SDL_GameControllerDB/blob/master/gamecontrollerdb.txt
                SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
                initSuccess = true;
            }
            });

        return initSuccess;
    }

    void Quit()
    {
        for (auto const& [index, controller] : connectedControllers)
        {
            if (controller)
                SDL_GameControllerClose(controller);
        }
        connectedControllers.clear();
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);
    }

    SDL_GameController* GetOrOpenController(DWORD dwUserIndex)
    {
        if (connectedControllers.count(dwUserIndex) && connectedControllers[dwUserIndex] != nullptr)
        {
            if (SDL_GameControllerGetAttached(connectedControllers[dwUserIndex]))
                return connectedControllers[dwUserIndex];
            else
            {
                SDL_GameControllerClose(connectedControllers[dwUserIndex]);
                connectedControllers.erase(dwUserIndex);
            }
        }

        int numJoysticks = SDL_NumJoysticks();
        DWORD validControllerCount = 0;

        for (int i = 0; i < numJoysticks; ++i)
        {
            if (SDL_IsGameController(i))
            {
                if (validControllerCount == dwUserIndex)
                {
                    SDL_GameController* controller = SDL_GameControllerOpen(i);
                    if (controller)
                    {
                        connectedControllers[dwUserIndex] = controller;
                        return controller;
                    }
                }
                validControllerCount++;
            }
        }

        return nullptr;
    }

    DWORD WINAPI SDL2toX_SDL2GetState(DWORD dwUserIndex, void* pState, bool extended)
    {
        if (!Initialize()) return ERROR_DEVICE_NOT_CONNECTED;

        // Pump events to update controller connection statuses and states
        SDL_GameControllerUpdate();

        SDL_GameController* controller = GetOrOpenController(dwUserIndex);
        if (!controller)
            return ERROR_DEVICE_NOT_CONNECTED;

        static DWORD packetNumber = 0;

        if (extended)
        {
            XINPUT_STATE* state = (XINPUT_STATE*)pState;
            state->dwPacketNumber = packetNumber++;
            memset(&(state->Gamepad), 0, sizeof(XINPUT_GAMEPAD_EX));
        }
        else
        {
            XINPUT_STATE* state = (XINPUT_STATE*)pState;
            state->dwPacketNumber = packetNumber++;
            memset(&(state->Gamepad), 0, sizeof(XINPUT_GAMEPAD));
        }

        XINPUT_GAMEPAD* gamepadState = &((XINPUT_STATE*)pState)->Gamepad;

        // Buttons
#define MAP_BTN(sdl_btn, xinput_btn) if (SDL_GameControllerGetButton(controller, sdl_btn)) gamepadState->wButtons |= xinput_btn
        MAP_BTN(SDL_CONTROLLER_BUTTON_A, XINPUT_GAMEPAD_A);
        MAP_BTN(SDL_CONTROLLER_BUTTON_B, XINPUT_GAMEPAD_B);
        MAP_BTN(SDL_CONTROLLER_BUTTON_X, XINPUT_GAMEPAD_X);
        MAP_BTN(SDL_CONTROLLER_BUTTON_Y, XINPUT_GAMEPAD_Y);
        MAP_BTN(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, XINPUT_GAMEPAD_LEFT_SHOULDER);
        MAP_BTN(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER);
        MAP_BTN(SDL_CONTROLLER_BUTTON_BACK, XINPUT_GAMEPAD_BACK);
        MAP_BTN(SDL_CONTROLLER_BUTTON_START, XINPUT_GAMEPAD_START);
        MAP_BTN(SDL_CONTROLLER_BUTTON_LEFTSTICK, XINPUT_GAMEPAD_LEFT_THUMB);
        MAP_BTN(SDL_CONTROLLER_BUTTON_RIGHTSTICK, XINPUT_GAMEPAD_RIGHT_THUMB);
        MAP_BTN(SDL_CONTROLLER_BUTTON_DPAD_UP, XINPUT_GAMEPAD_DPAD_UP);
        MAP_BTN(SDL_CONTROLLER_BUTTON_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_DOWN);
        MAP_BTN(SDL_CONTROLLER_BUTTON_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_LEFT);
        MAP_BTN(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT);
#undef MAP_BTN

        // Axes (SDL2 range is -32768 to 32767 for sticks)
        gamepadState->sThumbLX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
        gamepadState->sThumbLY = ~SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
        gamepadState->sThumbRX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
        gamepadState->sThumbRY = ~SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);

        // Triggers 
        auto mapTrigger = [](Sint16 sdlAxis) -> BYTE {
            if (sdlAxis < 0) return 0; // Failsafe
            return (BYTE)((sdlAxis * 255) / 32767);
            };

        gamepadState->bLeftTrigger = mapTrigger(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
        gamepadState->bRightTrigger = mapTrigger(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));

        return ERROR_SUCCESS;
    }

    DWORD WINAPI SDL2toX_SDL2SetState(DWORD dwUserIndex, void* pVibration)
    {
        if (!Initialize()) return ERROR_DEVICE_NOT_CONNECTED;

        SDL_GameController* controller = GetOrOpenController(dwUserIndex);
        if (!controller)
            return ERROR_DEVICE_NOT_CONNECTED;

        XINPUT_VIBRATION* vib = (XINPUT_VIBRATION*)pVibration;
        SDL_GameControllerRumble(controller, vib->wLeftMotorSpeed, vib->wRightMotorSpeed, 0xFFFFFFFF);

        return ERROR_SUCCESS;
    }

    DWORD WINAPI SDL2toX_SDL2GetCapabilities(DWORD dwUserIndex, DWORD dwFlags, void* pCapabilities)
    {
        if (!Initialize()) return ERROR_DEVICE_NOT_CONNECTED;

        SDL_GameController* controller = GetOrOpenController(dwUserIndex);
        if (!controller)
            return ERROR_DEVICE_NOT_CONNECTED;

        XINPUT_CAPABILITIES* caps = (XINPUT_CAPABILITIES*)pCapabilities;
        memset(caps, 0, sizeof(XINPUT_CAPABILITIES));

        caps->Type = XINPUT_DEVTYPE_GAMEPAD;
        caps->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;

        caps->Gamepad.wButtons = 0xFFFF;
        caps->Gamepad.bLeftTrigger = 255;
        caps->Gamepad.bRightTrigger = 255;
        caps->Gamepad.sThumbLX = 32767;
        caps->Gamepad.sThumbLY = 32767;
        caps->Gamepad.sThumbRX = 32767;
        caps->Gamepad.sThumbRY = 32767;

        caps->Vibration.wLeftMotorSpeed = 65535;
        caps->Vibration.wRightMotorSpeed = 65535;

        return ERROR_SUCCESS;
    }
}
#pragma once

#include "raylib.h"
#include "constants.h"

// Runtime settings shared by the pause menu and gameplay systems.
// Defaults match the previous hardcoded values in main.cpp / player.cpp.
struct GameSettings {
    float mouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;
    int   renderDistance   = DEFAULT_RENDER_DISTANCE;
    float cameraFov        = DEFAULT_CAMERA_FOV;

    int keyForward = KEY_W;
    int keyBack    = KEY_S;
    int keyLeft    = KEY_A;
    int keyRight   = KEY_D;
    int keyJump    = KEY_SPACE;
    int keySprint  = KEY_LEFT_SHIFT;

    void Clamp() {
        if (mouseSensitivity < 0.01f) mouseSensitivity = 0.01f;
        if (mouseSensitivity > 1.0f)  mouseSensitivity = 1.0f;
        if (renderDistance < 1)  renderDistance = 1;
        if (renderDistance > 16) renderDistance = 16;
        if (cameraFov < 40.0f)  cameraFov = 40.0f;
        if (cameraFov > 110.0f) cameraFov = 110.0f;
    }
};

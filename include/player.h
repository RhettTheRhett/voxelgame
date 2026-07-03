#pragma once

#include "raylib.h"
#include <cstdint>

struct PlayerData {
    uint8_t slot;
    int direction;

};

class Player {
    public:
    void SelectSlot(uint8_t slot);
    void ScrollSlot(int direction);
    
    private:
};
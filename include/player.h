#pragma once

#include "raylib.h"
#include "block.h"
#include "constants.h"
#include <cstdint>
#include <array>

struct PlayerData {
    uint8_t currentSlot;
    std::array<Block,HOTBAR_SIZE> blockSlot;
};

class Player {
    public:
    void SelectSlot(uint8_t slot);
    void ScrollSlot(int direction);

    Block GetHeldItem() const;
    const PlayerData& GetData() const;
    
    private:
    PlayerData playerData = {0,  {Block::DIRT, Block::GRASS, Block::STONE, Block::LIGHT_STONE, Block::WOOD, Block::PLANKS, Block::BRICK, Block::SAND, Block::BEDROCK}};
};

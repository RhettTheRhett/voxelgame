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

    Vector3 GetPosition() const;
    Vector3 GetEyePosition() const;
    BoundingBox GetBounds() const;

    void SetPositionFromEye(Vector3 eye);

    
    private:
    PlayerData playerData = {0,  {Block::DIRT, Block::GRASS, Block::STONE, Block::LIGHT_STONE, Block::WOOD, Block::PLANKS, Block::BRICK, Block::SAND, Block::BEDROCK}};
    
    Vector3 position = { 0.0f, 80.0f, 0.0f };
    Vector3 velocity;

    bool onGround;
};

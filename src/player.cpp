#include "raylib.h"
#include "player.h"


void Player::SelectSlot(uint8_t slot){
    playerData.currentSlot = slot % HOTBAR_SIZE;
}
void Player::ScrollSlot(int direction)
{
    int slot = static_cast<int>(playerData.currentSlot);
    slot = (slot + direction) % HOTBAR_SIZE;

    if (slot < 0)
        slot += HOTBAR_SIZE;

    SelectSlot(static_cast<uint8_t>(slot));
}

Block Player::GetHeldItem() const {
    return playerData.blockSlot[playerData.currentSlot];
}
const PlayerData& Player::GetData() const {
    return playerData;
}


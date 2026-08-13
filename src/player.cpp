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

Vector3 Player::GetPosition() const {
    return position;
}

Vector3 Player::GetEyePosition() const {
    return { position.x, position.y + PLAYER_EYE_HEIGHT, position.z };
}
BoundingBox Player::GetBounds() const {
    float half = PLAYER_WIDTH * 0.5f;

    BoundingBox box;
    box.min = { position.x - half,  position.y, position.z - half  };
    box.max = { position.x + half,  position.y + PLAYER_HEIGHT, position.z + half };
    return box;
}

void Player::SetPositionFromEye(Vector3 eye) {
    position.x = eye.x;
    position.y = eye.y - PLAYER_EYE_HEIGHT;
    position.z = eye.z;
}

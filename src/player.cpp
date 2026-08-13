#include "raylib.h"
#include "raymath.h"
#include "player.h"
#include <cmath>

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

void Player::ApplyInput(float yaw, float deltaTime) {
    float yawRad = yaw * DEG2RAD;
    Vector3 moveForward = { cosf(yawRad), 0.0f, sinf(yawRad) };
    Vector3 moveSide    = { cosf(yawRad + 90.0f * DEG2RAD), 0.0f, sinf(yawRad + 90.0f * DEG2RAD) };

    velocity.x = 0.0f;
    velocity.z = 0.0f;

    if (IsKeyDown(KEY_W)) { velocity.x += moveForward.x * PLAYER_MOVE_SPEED; velocity.z += moveForward.z * PLAYER_MOVE_SPEED; }
    if (IsKeyDown(KEY_S)) { velocity.x -= moveForward.x * PLAYER_MOVE_SPEED; velocity.z -= moveForward.z * PLAYER_MOVE_SPEED; }
    if (IsKeyDown(KEY_A)) { velocity.x -= moveSide.x    * PLAYER_MOVE_SPEED; velocity.z -= moveSide.z    * PLAYER_MOVE_SPEED; }
    if (IsKeyDown(KEY_D)) { velocity.x += moveSide.x    * PLAYER_MOVE_SPEED; velocity.z += moveSide.z    * PLAYER_MOVE_SPEED; }
    if (IsKeyPressed(KEY_SPACE) ) { // && onGround
        velocity.y = PLAYER_JUMP_SPEED;
        //onGround = false;
    }   

    (void)deltaTime;
}

void Player::ApplyDebugInput() {
    if (IsKeyPressed(KEY_PAGE_UP)) {
        Vector3 eye = GetEyePosition();
        SetPositionFromEye({ eye.x, PLAYER_DEBUG_EYE_Y, eye.z });
        velocity.y = 0.0f;
    }
    if (IsKeyPressed(KEY_PAGE_DOWN)) {
        gravityPaused = !gravityPaused;
        if (gravityPaused) velocity.y = 0.0f;
    }
}

void Player::UpdatePhysics(World& world, float deltaTime) {
    (void)world;

    if (!gravityPaused) {
        velocity.y -= PLAYER_GRAVITY * deltaTime;
        velocity.y = Clamp(velocity.y, -PLAYER_TERMINAL_VELOCITY, PLAYER_TERMINAL_VELOCITY);
    } else {
        velocity.y = 0.0f;
    }

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z += velocity.z * deltaTime;
}

void Player::SyncCamera(Camera3D& camera, float yaw, float pitch) const {
    float pitchRad = pitch * DEG2RAD;
    float yawRad   = yaw   * DEG2RAD;

    Vector3 forward = {
        cosf(pitchRad) * cosf(yawRad),
        sinf(pitchRad),
        cosf(pitchRad) * sinf(yawRad)
    };

    camera.position = GetEyePosition();
    camera.target   = Vector3Add(camera.position, forward);
}
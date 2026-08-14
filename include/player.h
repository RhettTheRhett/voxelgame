#pragma once

#include "raylib.h"
#include "block.h"
#include "constants.h"

#include <cstdint>
#include <array>

struct World;

struct PlayerData {
    uint8_t currentSlot;
    std::array<BlockId, HOTBAR_SIZE> blockSlot;
};

class Player {
    public:
    Player();

    void SelectSlot(uint8_t slot);
    void ScrollSlot(int direction);

    BlockId GetHeldItem() const;
    const PlayerData& GetData() const;

    Vector3 GetPosition() const;
    Vector3 GetEyePosition() const;
    BoundingBox GetBounds() const;

    void SetPositionFromEye(Vector3 eye);
    void ApplyInput(float yaw, float deltaTime);
    void ApplyDebugInput();
    void UpdatePhysics(World& world, float deltaTime);
    void SyncCamera(Camera3D& camera, float yaw, float pitch) const;

    bool IsGravityPaused() const { return gravityPaused; }
    bool IsOnGround() const { return onGround; }

    float GetStepHeight() const { return stepHeight; }
    void SetStepHeight(float height) { stepHeight = height; }

    private:
    void InitDefaultHotbar();
    void TryConsumeJumpBuffer();
    bool TryStepUp(const World& world, float desiredStep);

    void ResolveCollisionsY(const World& world);
    void ResolveCollisionsX(const World& world);
    void ResolveCollisionsZ(const World& world);

    PlayerData playerData{};

    Vector3 position = { 0.0f, 80.0f, 0.0f };
    Vector3 velocity{};

    bool onGround = false;
    bool wasOnGround = false;
    bool gravityPaused = false;
    float jumpBufferTimer = 0.0f;
    float stepHeight = PLAYER_STEP_HEIGHT;
};

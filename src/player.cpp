#include "raylib.h"
#include "raymath.h"
#include "player.h"
#include "collision.h"
#include "chunk.h"
#include "world.h"
#include <cmath>
#include <cstdio>

//PUBLIC

Player::Player() {
    InitDefaultHotbar();
}

void Player::InitDefaultHotbar() {
    // Resolve via content ids so hotbar tracks the registry, not hard-coded ordinals.
    static const char* kDefaultHotbarIds[HOTBAR_SIZE] = {
        "game:dirt",
        "game:grass",
        "game:stone",
        "game:stone_slab",
        "game:light_stone",
        "game:wood",
        "game:planks",
        "game:brick",
        "game:sand",
    };

    playerData.currentSlot = 0;
    for (uint8_t i = 0; i < HOTBAR_SIZE; i++) {
        BlockId id = Block::AIR;
        if (!TryGetBlockId(kDefaultHotbarIds[i], id)) {
            printf("Player::InitDefaultHotbar: missing '%s'\n", kDefaultHotbarIds[i]);
            id = Block::AIR;
        }
        playerData.blockSlot[i] = id;
    }
}

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

BlockId Player::GetHeldItem() const {
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

    bool moving = IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D);
    float speed = PLAYER_MOVE_SPEED;
    if (moving && IsKeyDown(KEY_LEFT_SHIFT)) {
        speed *= PLAYER_RUN_MULTIPLIER;
    }

    if (IsKeyDown(KEY_W)) { velocity.x += moveForward.x * speed; velocity.z += moveForward.z * speed; }
    if (IsKeyDown(KEY_S)) { velocity.x -= moveForward.x * speed; velocity.z -= moveForward.z * speed; }
    if (IsKeyDown(KEY_A)) { velocity.x -= moveSide.x    * speed; velocity.z -= moveSide.z    * speed; }
    if (IsKeyDown(KEY_D)) { velocity.x += moveSide.x    * speed; velocity.z += moveSide.z    * speed; }

    // Minecraft-style jump buffer: holding/tapping Space queues a jump until grounded.
    if (IsKeyDown(KEY_SPACE)) {
        jumpBufferTimer = PLAYER_JUMP_BUFFER;
    } else {
        jumpBufferTimer -= deltaTime;
        if (jumpBufferTimer < 0.0f) jumpBufferTimer = 0.0f;
    }
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

void Player::TryConsumeJumpBuffer() {
    if (jumpBufferTimer <= 0.0f || !onGround) return;

    velocity.y = PLAYER_JUMP_SPEED;
    onGround = false;
    wasOnGround = false;
    jumpBufferTimer = 0.0f;
}

void Player::UpdatePhysics(World& world, float deltaTime) {
    wasOnGround = onGround;
    onGround = false;

    if (!gravityPaused) {
        velocity.y -= PLAYER_GRAVITY * deltaTime;
        velocity.y = Clamp(velocity.y, -PLAYER_TERMINAL_VELOCITY, PLAYER_TERMINAL_VELOCITY);
    } else {
        velocity.y = 0.0f;
    }

    // Axis-separated: walls first (with step-up), ground last
    position.x += velocity.x * deltaTime;
    ResolveCollisionsX(world);
    position.z += velocity.z * deltaTime;
    ResolveCollisionsZ(world);
    position.y += velocity.y * deltaTime;
    ResolveCollisionsY(world);

    // Consume buffered jump after landing so hold-space auto-jumps without a frame of delay.
    TryConsumeJumpBuffer();
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

//PRIVATE

static void CollectOverlapRange(const BoundingBox& box, int& minBX, int& maxBX, int& minBY, int& maxBY, int& minBZ, int& maxBZ) {
    minBX = (int)floorf(box.min.x);
    maxBX = (int)floorf(box.max.x);
    minBY = (int)floorf(box.min.y);
    maxBY = (int)floorf(box.max.y);
    minBZ = (int)floorf(box.min.z);
    maxBZ = (int)floorf(box.max.z);
}

// True when the player body substantially overlaps the block on Y (a wall),
// not when we are merely standing on top of / under a ceiling.
static bool IsWallOverlap(const BoundingBox& playerBox, const BoundingBox& blockBox) {
    float yOverlap = fminf(playerBox.max.y, blockBox.max.y) - fmaxf(playerBox.min.y, blockBox.min.y);
    return yOverlap > 0.05f;
}

static bool BodyHitsSolid(const World& world, const BoundingBox& box) {
    int minBX, maxBX, minBY, maxBY, minBZ, maxBZ;
    CollectOverlapRange(box, minBX, maxBX, minBY, maxBY, minBZ, maxBZ);

    for (int by = minBY; by <= maxBY; by++) {
        for (int bx = minBX; bx <= maxBX; bx++) {
            for (int bz = minBZ; bz <= maxBZ; bz++) {
                if (!IsSolid(world, bx, by, bz)) continue;
                BoundingBox blockBox = GetBlockCollisionBounds(world, bx, by, bz);
                if (!Overlaps(box, blockBox)) continue;
                if (IsWallOverlap(box, blockBox)) return true;
            }
        }
    }
    return false;
}

bool Player::TryStepUp(const World& world, float desiredStep) {
    // desiredStep = how far feet must rise to stand on blockTop (not the block's own height).
    if (desiredStep <= 0.01f || desiredStep > stepHeight) return false;

    float savedY = position.y;
    position.y = savedY + desiredStep;

    // Headroom / body must be clear after the step (ignore floor graze).
    if (BodyHitsSolid(world, GetBounds())) {
        position.y = savedY;
        return false;
    }
    return true;
}

void Player::ResolveCollisionsX(const World& world) {
    float half = PLAYER_WIDTH * 0.5f;

    int minBX, maxBX, minBY, maxBY, minBZ, maxBZ;
    CollectOverlapRange(GetBounds(), minBX, maxBX, minBY, maxBY, minBZ, maxBZ);

    for (int by = minBY; by <= maxBY; by++) {
        for (int bx = minBX; bx <= maxBX; bx++) {
            for (int bz = minBZ; bz <= maxBZ; bz++) {
                if (!IsSolid(world, bx, by, bz)) continue;

                BoundingBox blockBox = GetBlockCollisionBounds(world, bx, by, bz);
                BoundingBox box = GetBounds();
                if (!Overlaps(box, blockBox)) continue;
                if (!IsWallOverlap(box, blockBox)) continue;

                // Climb distance from current feet to this surface — works slab→full (0.5)
                // and for inefficient staircases of many sub-step-height ledges.
                if (wasOnGround && velocity.y <= 0.0f) {
                    float step = blockBox.max.y - position.y;
                    if (TryStepUp(world, step)) {
                        box = GetBounds();
                        if (!Overlaps(box, blockBox) || !IsWallOverlap(box, blockBox)) {
                            continue;
                        }
                    }
                }

                float blockCenterX = (float)bx + 0.5f;
                if (position.x < blockCenterX) {
                    position.x = blockBox.min.x - half;
                } else {
                    position.x = blockBox.max.x + half;
                }
                velocity.x = 0.0f;
            }
        }
    }
}

void Player::ResolveCollisionsZ(const World& world) {
    float half = PLAYER_WIDTH * 0.5f;

    int minBX, maxBX, minBY, maxBY, minBZ, maxBZ;
    CollectOverlapRange(GetBounds(), minBX, maxBX, minBY, maxBY, minBZ, maxBZ);

    for (int by = minBY; by <= maxBY; by++) {
        for (int bx = minBX; bx <= maxBX; bx++) {
            for (int bz = minBZ; bz <= maxBZ; bz++) {
                if (!IsSolid(world, bx, by, bz)) continue;

                BoundingBox blockBox = GetBlockCollisionBounds(world, bx, by, bz);
                BoundingBox box = GetBounds();
                if (!Overlaps(box, blockBox)) continue;
                if (!IsWallOverlap(box, blockBox)) continue;

                if (wasOnGround && velocity.y <= 0.0f) {
                    float step = blockBox.max.y - position.y;
                    if (TryStepUp(world, step)) {
                        box = GetBounds();
                        if (!Overlaps(box, blockBox) || !IsWallOverlap(box, blockBox)) {
                            continue;
                        }
                    }
                }

                float blockCenterZ = (float)bz + 0.5f;
                if (position.z < blockCenterZ) {
                    position.z = blockBox.min.z - half;
                } else {
                    position.z = blockBox.max.z + half;
                }
                velocity.z = 0.0f;
            }
        }
    }
}

void Player::ResolveCollisionsY(const World& world) {
    BoundingBox box = GetBounds();

    int minBX, maxBX, minBY, maxBY, minBZ, maxBZ;
    CollectOverlapRange(box, minBX, maxBX, minBY, maxBY, minBZ, maxBZ);

    // Penetration tolerance for landing / head bumps (not for side-of-wall snaps).
    float maxPen = fmaxf(0.51f, fabsf(velocity.y) * (1.0f / 30.0f) + 0.05f);
    if (maxPen > 0.9f) maxPen = 0.9f;

    float bestFloorY = -INFINITY;
    float bestCeilY  =  INFINITY;

    for (int by = minBY; by <= maxBY; by++) {
        for (int bx = minBX; bx <= maxBX; bx++) {
            for (int bz = minBZ; bz <= maxBZ; bz++) {
                if (!IsSolid(world, bx, by, bz)) continue;

                BoundingBox blockBox = GetBlockCollisionBounds(world, bx, by, bz);
                if (!Overlaps(box, blockBox)) continue;

                float blockH = blockBox.max.y - blockBox.min.y;
                if (blockH < 0.001f) continue;

                if (velocity.y <= 0.0f) {
                    float feetY    = box.min.y;
                    float blockTop = blockBox.max.y;
                    float pen      = blockTop - feetY;

                    // Floor: feet near the TOP of this shape only (not mid-wall overlap).
                    bool nearTop = feetY >= (blockBox.min.y + blockH * 0.5f - 0.01f);
                    bool isFloor = nearTop && pen >= 0.0f && pen <= maxPen;

                    if (isFloor && blockTop > bestFloorY) {
                        bestFloorY = blockTop;
                    }
                } else {
                    float headY       = box.max.y;
                    float blockBottom = blockBox.min.y;
                    float pen         = headY - blockBottom;

                    // Ceiling: head near the BOTTOM of this shape only.
                    // Prevents wall-side overlaps while jumping from snapping to
                    // (blockMin.y - PLAYER_HEIGHT) and burying the player underground.
                    bool nearBottom = headY <= (blockBox.min.y + blockH * 0.5f + 0.01f);
                    bool isCeiling  = nearBottom && pen >= 0.0f && pen <= maxPen;

                    if (isCeiling && blockBottom < bestCeilY) {
                        bestCeilY = blockBottom;
                    }
                }
            }
        }
    }

    if (velocity.y <= 0.0f && bestFloorY > -INFINITY) {
        position.y = bestFloorY;
        velocity.y = 0.0f;
        onGround = true;
    } else if (velocity.y > 0.0f && bestCeilY < INFINITY) {
        position.y = bestCeilY - PLAYER_HEIGHT;
        velocity.y = 0.0f;
    }
}

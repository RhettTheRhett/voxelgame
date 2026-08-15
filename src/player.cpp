#include "raylib.h"
#include "raymath.h"
#include "player.h"
#include "settings.h"
#include "collision.h"
#include "chunk.h"
#include "world.h"
#include "water.h"
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
        "game:glass",
        "game:brick",
        "game:water",
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

void Player::ApplyInput(float yaw, float deltaTime, const GameSettings& settings) {
    float yawRad = yaw * DEG2RAD;
    Vector3 moveForward = { cosf(yawRad), 0.0f, sinf(yawRad) };
    Vector3 moveSide    = { cosf(yawRad + 90.0f * DEG2RAD), 0.0f, sinf(yawRad + 90.0f * DEG2RAD) };

    velocity.x = 0.0f;
    velocity.z = 0.0f;

    bool moving =
        IsKeyDown(settings.keyForward) ||
        IsKeyDown(settings.keyBack) ||
        IsKeyDown(settings.keyLeft) ||
        IsKeyDown(settings.keyRight);

    float speed = PLAYER_MOVE_SPEED;
    if (moving && IsKeyDown(settings.keySprint)) {
        speed *= PLAYER_RUN_MULTIPLIER;
    }

    if (IsKeyDown(settings.keyForward)) { velocity.x += moveForward.x * speed; velocity.z += moveForward.z * speed; }
    if (IsKeyDown(settings.keyBack))    { velocity.x -= moveForward.x * speed; velocity.z -= moveForward.z * speed; }
    if (IsKeyDown(settings.keyLeft))    { velocity.x -= moveSide.x    * speed; velocity.z -= moveSide.z    * speed; }
    if (IsKeyDown(settings.keyRight))   { velocity.x += moveSide.x    * speed; velocity.z += moveSide.z    * speed; }

    // Minecraft-style jump buffer: holding/tapping jump queues until grounded.
    if (IsKeyDown(settings.keyJump)) {
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
    if (jumpBufferTimer <= 0.0f) return;

    if (inWater) {
        // Hold-jump while submerged = swim up (buffer stays refreshed while key down).
        velocity.y = PLAYER_WATER_SWIM_UP;
        return;
    }

    if (!onGround) return;

    velocity.y = PLAYER_JUMP_SPEED;
    onGround = false;
    wasOnGround = false;
    jumpBufferTimer = 0.0f;
}

static bool BoundsTouchFluid(const World& world, const BoundingBox& box) {
    int minBX = (int)floorf(box.min.x);
    int maxBX = (int)floorf(box.max.x);
    int minBY = (int)floorf(box.min.y);
    int maxBY = (int)floorf(box.max.y);
    int minBZ = (int)floorf(box.min.z);
    int maxBZ = (int)floorf(box.max.z);

    for (int by = minBY; by <= maxBY; by++)
    for (int bx = minBX; bx <= maxBX; bx++)
    for (int bz = minBZ; bz <= maxBZ; bz++) {
        if (IsFluid(GetWorldBlock(world, bx, by, bz))) return true;
    }
    return false;
}

void Player::UpdatePhysics(World& world, float deltaTime) {
    wasOnGround = onGround;
    onGround = false;

    constexpr float kMaxPhysicsDt = 1.0f / 20.0f;
    if (deltaTime > kMaxPhysicsDt) deltaTime = kMaxPhysicsDt;

    inWater = BoundsTouchFluid(world, GetBounds());

    if (inWater) {
        // Slower strafe — ApplyInput already wrote dry-land speeds into velocity.xz.
        velocity.x *= PLAYER_WATER_MOVE_MULT;
        velocity.z *= PLAYER_WATER_MOVE_MULT;

        if (!gravityPaused) {
            // Sink by default (reduced gravity). Jump/swim handled in TryConsumeJumpBuffer.
            velocity.y -= PLAYER_GRAVITY * PLAYER_WATER_GRAVITY_MULT * deltaTime;
            velocity.y = Clamp(velocity.y, -PLAYER_WATER_TERMINAL, PLAYER_WATER_SWIM_UP);
        } else {
            velocity.y = 0.0f;
        }
    } else if (!gravityPaused) {
        velocity.y -= PLAYER_GRAVITY * deltaTime;
        velocity.y = Clamp(velocity.y, -PLAYER_TERMINAL_VELOCITY, PLAYER_TERMINAL_VELOCITY);
    } else {
        velocity.y = 0.0f;
    }

    float prevFeetY = position.y;
    position.y += velocity.y * deltaTime;
    ResolveCollisionsY(world, prevFeetY);

    position.x += velocity.x * deltaTime;
    ResolveCollisionsX(world);
    position.z += velocity.z * deltaTime;
    ResolveCollisionsZ(world);

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

static float AxisOverlapX(const BoundingBox& a, const BoundingBox& b) {
    return Overlap1D(a.min.x, a.max.x, b.min.x, b.max.x);
}
static float AxisOverlapZ(const BoundingBox& a, const BoundingBox& b) {
    return Overlap1D(a.min.z, a.max.z, b.min.z, b.max.z);
}

void Player::ResolveCollisionsX(const World& world) {
    float half = PLAYER_WIDTH * 0.5f;
    constexpr float kAxisEps = 0.001f;

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

                // Orthogonal walls (in front / behind) overlap XZ but are Z-hits.
                // Resolving them on X shoves you sideways into the other wall at
                // chunk corners. Only handle the shallower-X (primary X) face.
                float ox = AxisOverlapX(box, blockBox);
                float oz = AxisOverlapZ(box, blockBox);
                if (ox > oz + kAxisEps) continue;

                // Climb distance from current feet to this surface — works slab→full (0.5)
                // and for inefficient staircases of many sub-step-height ledges.
                if (onGround && velocity.y <= 0.0f) {
                    float step = blockBox.max.y - position.y;
                    if (TryStepUp(world, step)) {
                        box = GetBounds();
                        if (!Overlaps(box, blockBox) || !IsWallOverlap(box, blockBox)) {
                            continue;
                        }
                    }
                }

                float penNeg = box.max.x - blockBox.min.x;
                float penPos = blockBox.max.x - box.min.x;
                if (penNeg < penPos) {
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
    constexpr float kAxisEps = 0.001f;

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

                float ox = AxisOverlapX(box, blockBox);
                float oz = AxisOverlapZ(box, blockBox);
                if (oz > ox + kAxisEps) continue;

                if (onGround && velocity.y <= 0.0f) {
                    float step = blockBox.max.y - position.y;
                    if (TryStepUp(world, step)) {
                        box = GetBounds();
                        if (!Overlaps(box, blockBox) || !IsWallOverlap(box, blockBox)) {
                            continue;
                        }
                    }
                }

                float penNeg = box.max.z - blockBox.min.z;
                float penPos = blockBox.max.z - box.min.z;
                if (penNeg < penPos) {
                    position.z = blockBox.min.z - half;
                } else {
                    position.z = blockBox.max.z + half;
                }
                velocity.z = 0.0f;
            }
        }
    }
}

void Player::ResolveCollisionsY(const World& world, float prevFeetY) {
    BoundingBox box = GetBounds();

    int minBX, maxBX, minBY, maxBY, minBZ, maxBZ;
    CollectOverlapRange(box, minBX, maxBX, minBY, maxBY, minBZ, maxBZ);

    float feetY     = box.min.y;
    float headY     = box.max.y;
    float prevHeadY = prevFeetY + PLAYER_HEIGHT;

    // Swept range: include cells we traveled through this frame, not just
    // the destination AABB (a hitch can skip past the ground cell).
    minBY = (int)floorf(fminf(feetY, prevFeetY)) - 1;
    maxBY = (int)floorf(fmaxf(headY, prevHeadY));

    constexpr float kGroundStick = 0.08f;
    constexpr float kSurfaceEps  = 0.001f;

    float bestFloorY = -INFINITY;
    float bestCeilY  =  INFINITY;

    for (int by = minBY; by <= maxBY; by++) {
        for (int bx = minBX; bx <= maxBX; bx++) {
            for (int bz = minBZ; bz <= maxBZ; bz++) {
                if (!IsSolid(world, bx, by, bz)) continue;

                BoundingBox blockBox = GetBlockCollisionBounds(world, bx, by, bz);
                if (!OverlapsXZ(box, blockBox)) continue;

                float blockH = blockBox.max.y - blockBox.min.y;
                if (blockH < 0.001f) continue;

                if (velocity.y <= 0.0f) {
                    float blockTop = blockBox.max.y;
                    // Started at/above this surface, now at/below it (with stick).
                    // Body-height walls fail prevFeetY >= blockTop, so they
                    // are not treated as floors.
                    bool isFloor = (prevFeetY >= blockTop - kSurfaceEps) &&
                                   (feetY <= blockTop + kGroundStick);

                    if (isFloor && blockTop > bestFloorY) {
                        bestFloorY = blockTop;
                    }
                } else {
                    float blockBottom = blockBox.min.y;
                    bool isCeiling = (prevHeadY <= blockBottom + kSurfaceEps) &&
                                     (headY >= blockBottom);

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

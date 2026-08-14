#pragma once
#include "raylib.h"
#include <cstdint>

 static const float FACE_VERTS[6][12] = {
    // +Y top
    { 1,1,1,  0,1,1,  0,1,0,  1,1,0 },
    // -Y bottom
    { 1,0,0,  0,0,0,  0,0,1,  1,0,1 },
    // +X right
    { 1,1,1,  1,1,0,  1,0,0,  1,0,1 },
    // -X left
    { 0,1,0,  0,1,1,  0,0,1,  0,0,0 },
    // +Z front
    { 0,1,1,  1,1,1,  1,0,1,  0,0,1 },
    // -Z back
    { 1,1,0,  0,1,0,  0,0,0,  1,0,0 },
    };

    static const int FACE_DIRS[6][3] = {
    {  0,  1,  0 },  // +Y
    {  0, -1,  0 },  // -Y
    {  1,  0,  0 },  // +X
    { -1,  0,  0 },  // -X
    {  0,  0,  1 },  // +Z
    {  0,  0, -1 },  // -Z
    };

// Numeric id stored in chunks / hotbar. Same values as registry indices for builtins.
using BlockId = uint16_t;

// Thin aliases for call-site convenience. Source of truth for *identity* is contentId
// ("game:stone"); these ordinals are remappable later via a save palette (deferred).
enum Block : BlockId {
    AIR = 0,
    BEDROCK,
    GRASS,
    DIRT,
    STONE,
    LIGHT_STONE,
    WOOD,
    PLANKS,
    BRICK,
    SAND,
    STONE_SLAB,
};

struct BlockDefinition {
    const char* contentId;   // stable namespaced id, e.g. "game:stone"
    Vector2 FACE_TEX[6];
    bool isLightSource;
    uint8_t lightLevel;
    // Local-space collision AABB within the unit cell (0..1). Full block = {0,0,0}..{1,1,1}.
    Vector3 collisionMin;
    Vector3 collisionMax;
    bool blocksMotion;       // participates in player collision when true
};

// Call once at startup before any GetBlockDef / TryGetBlockId use.
void InitBlockRegistry();

// Hot path: O(1) array index by numeric id. Mesh/light/UI should use this.
const BlockDefinition& GetBlockDef(BlockId id);

// Cold path: string lookup (load/register/commands). Not per-voxel.
bool TryGetBlockId(const char* contentId, BlockId& outId);

BlockId GetBlockCount();

// Growable registration. contentId must outlive the registry (string literal OK).
BlockId RegisterBlock(const BlockDefinition& def);

// True if neighbor's collision fully covers the shared face (face index matches FACE_DIRS).
// Used for mesh face culling so slabs don't hide adjacent full-block faces.
bool OccludesNeighborFace(BlockId neighborId, int face);

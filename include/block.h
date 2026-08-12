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
};

struct BlockDefinition {
    const char* contentId;   // stable namespaced id, e.g. "game:stone"
    Vector2 FACE_TEX[6];
    bool isLightSource;
    uint8_t lightLevel;
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

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
    GLASS,
    STONE_SLAB_TOP, // upper half-slab; append-only so old saves keep glass id
    // Water: source + flowing depths (no block-state metadata yet).
    WATER,   // source, level 0
    WATER_1,
    WATER_2,
    WATER_3,
    WATER_4,
    WATER_5,
    WATER_6,
    WATER_7,
    // Decorative / cutout — append-only for save id stability.
    LEAVES,
    RED_MUSHROOM,
};

enum class BlockMeshShape : uint8_t {
    Cube = 0,
    Cross = 1, // two diagonal quads (flowers, mushrooms)
};

struct BlockDefinition {
    const char* contentId;   // stable namespaced id, e.g. "game:stone"
    Vector2 FACE_TEX[6];
    bool isLightSource;
    uint8_t lightLevel;
    // Local-space collision AABB within the unit cell (0..1). Full block = {0,0,0}..{1,1,1}.
    // Also used for raycast pick bounds (even when blocksMotion is false).
    Vector3 collisionMin;
    Vector3 collisionMax;
    bool blocksMotion;       // participates in player collision when true
    // Opaque: hides neighbor faces + stops sky/block light from traveling through.
    // Independent of blocksMotion — glass collides but is not opaque.
    bool opaque;
    // Translucent: alpha-blended mesh pass (glass, water). Cutout uses opaque pass + discard.
    bool translucent;
    BlockMeshShape meshShape;
    // Extra light levels lost when light enters this cell (on top of the usual -1).
    // 0 = air/glass; leaves use a small value so canopy darkens without fully blocking.
    uint8_t lightOpacity;
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

// True if neighbor fully covers the face this block actually emits (partial shapes included).
// face index matches FACE_DIRS. Non-opaque neighbors never occlude.
bool OccludesNeighborFace(BlockId selfId, BlockId neighborId, int face);

// Helpers for slab placement / merging.
bool IsBottomSlab(BlockId id);
bool IsTopSlab(BlockId id);
bool IsSlab(BlockId id);
// Matching bottom+top stone slabs combine into full stone (Minecraft double-slab).
// TECH DEBT: hardcodes Block::STONE — wrong once wood/brick/etc. slabs exist.
BlockId SlabDoubleResult(BlockId a, BlockId b);

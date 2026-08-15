#include "block.h"

#include <cstdio>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

std::vector<BlockDefinition> g_defs;
std::unordered_map<std::string, BlockId> g_byContentId;

constexpr Vector3 kFullMin = {0.0f, 0.0f, 0.0f};
constexpr Vector3 kFullMax = {1.0f, 1.0f, 1.0f};
constexpr Vector3 kSlabMin = {0.0f, 0.0f, 0.0f};
constexpr Vector3 kSlabMax = {1.0f, 0.5f, 1.0f};
constexpr Vector3 kSlabTopMin = {0.0f, 0.5f, 0.0f};
constexpr Vector3 kSlabTopMax = {1.0f, 1.0f, 1.0f};

static BlockDefinition MakeWaterDef(const char* id, float height) {
    Vector3 mn = {0.0f, 0.0f, 0.0f};
    Vector3 mx = {1.0f, height, 1.0f};
    Vector2 tex = {2, 3}; // atlas tile for water
    return { id, {tex,tex,tex,tex,tex,tex}, false, 0, mn, mx, false, false, true };
}

// Builtin table — registration order MUST match enum Block ordinals.
// Fields: contentId, FACE_TEX[6], isLightSource, lightLevel, collMin, collMax, blocksMotion, opaque, translucent
const BlockDefinition kBuiltinBlocks[] = {
    {"game:air",         {{},{},{},{},{},{}}, false, 0, kFullMin, kFullMax, false, false, false},
    {"game:bedrock",     {{15,15},{15,15},{15,15},{15,15},{15,15},{15,15}}, false, 0, kFullMin, kFullMax, true, true, false},
    {"game:grass",       {{0,0}, {1,0}, {0,1}, {0,1}, {0,1}, {0,1}}, false, 0, kFullMin, kFullMax, true, true, false},
    {"game:dirt",        {{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}}, false, 0, kFullMin, kFullMax, true, true, false},
    {"game:stone",       {{2,0},{2,0},{2,0},{2,0},{2,0},{2,0}}, false, 0, kFullMin, kFullMax, true, true, false},
    {"game:light_stone", {{1,1},{1,1},{1,1},{1,1},{1,1},{1,1}}, true, 15, kFullMin, kFullMax, true, true, false},
    {"game:wood",        {{0,2},{0,2},{1,2},{1,2},{1,2},{1,2}}, false, 0, kFullMin, kFullMax, true, true, false},
    {"game:planks",      {{0,3},{0,3},{0,3},{0,3},{0,3},{0,3}}, false, 0, kFullMin, kFullMax, true, true, false},
    {"game:brick",       {{2,1},{2,1},{2,1},{2,1},{2,1},{2,1}}, false, 0, kFullMin, kFullMax, true, true, false},
    {"game:sand",        {{2,2},{2,2},{2,2},{2,2},{2,2},{2,2}}, false, 0, kFullMin, kFullMax, true, true, false},
    {"game:stone_slab",  {{2,0},{2,0},{2,0},{2,0},{2,0},{2,0}}, false, 0, kSlabMin, kSlabMax, true, true, false},
    {"game:glass",       {{1,3},{1,3},{1,3},{1,3},{1,3},{1,3}}, false, 0, kFullMin, kFullMax, true, false, true},
    {"game:stone_slab_top", {{2,0},{2,0},{2,0},{2,0},{2,0},{2,0}}, false, 0, kSlabTopMin, kSlabTopMax, true, true, false},
    // Fluids: no blocksMotion, not opaque, translucent. Height encodes flow level for mesh/raycast.
    MakeWaterDef("game:water",   1.0f),
    MakeWaterDef("game:water_1", 7.0f / 8.0f),
    MakeWaterDef("game:water_2", 6.0f / 8.0f),
    MakeWaterDef("game:water_3", 5.0f / 8.0f),
    MakeWaterDef("game:water_4", 4.0f / 8.0f),
    MakeWaterDef("game:water_5", 3.0f / 8.0f),
    MakeWaterDef("game:water_6", 2.0f / 8.0f),
    MakeWaterDef("game:water_7", 1.0f / 8.0f),
};

bool Covers1D(float n0, float n1, float s0, float s1, float eps) {
    return n0 <= s0 + eps && n1 >= s1 - eps;
}

} // namespace

BlockId RegisterBlock(const BlockDefinition& def) {
    if (def.contentId == nullptr || def.contentId[0] == '\0') {
        printf("RegisterBlock: missing contentId\n");
        return Block::AIR;
    }

    auto existing = g_byContentId.find(def.contentId);
    if (existing != g_byContentId.end()) {
        printf("RegisterBlock: duplicate contentId '%s'\n", def.contentId);
        return existing->second;
    }

    if (g_defs.size() >= 0xFFFFu) {
        printf("RegisterBlock: block id space exhausted\n");
        return Block::AIR;
    }

    BlockId id = static_cast<BlockId>(g_defs.size());
    g_defs.push_back(def);
    g_byContentId.emplace(def.contentId, id);
    return id;
}

void InitBlockRegistry() {
    if (!g_defs.empty()) {
        return;
    }

    for (const BlockDefinition& def : kBuiltinBlocks) {
        RegisterBlock(def);
    }

    BlockId stoneId = Block::AIR;
    if (!TryGetBlockId("game:stone", stoneId) || stoneId != Block::STONE) {
        printf("InitBlockRegistry: builtin id mismatch (game:stone)\n");
    }
    BlockId slabId = Block::AIR;
    if (!TryGetBlockId("game:stone_slab", slabId) || slabId != Block::STONE_SLAB) {
        printf("InitBlockRegistry: builtin id mismatch (game:stone_slab)\n");
    }
    BlockId glassId = Block::AIR;
    if (!TryGetBlockId("game:glass", glassId) || glassId != Block::GLASS) {
        printf("InitBlockRegistry: builtin id mismatch (game:glass)\n");
    }
    BlockId slabTopId = Block::AIR;
    if (!TryGetBlockId("game:stone_slab_top", slabTopId) || slabTopId != Block::STONE_SLAB_TOP) {
        printf("InitBlockRegistry: builtin id mismatch (game:stone_slab_top)\n");
    }
    BlockId waterId = Block::AIR;
    if (!TryGetBlockId("game:water", waterId) || waterId != Block::WATER) {
        printf("InitBlockRegistry: builtin id mismatch (game:water)\n");
    }
}

const BlockDefinition& GetBlockDef(BlockId id) {
    if (g_defs.empty()) {
        InitBlockRegistry();
    }
    if (id >= g_defs.size()) {
        return g_defs[Block::AIR];
    }
    return g_defs[id];
}

bool TryGetBlockId(const char* contentId, BlockId& outId) {
    if (g_defs.empty()) {
        InitBlockRegistry();
    }
    if (contentId == nullptr) {
        return false;
    }
    auto it = g_byContentId.find(contentId);
    if (it == g_byContentId.end()) {
        return false;
    }
    outId = it->second;
    return true;
}

BlockId GetBlockCount() {
    if (g_defs.empty()) {
        InitBlockRegistry();
    }
    return static_cast<BlockId>(g_defs.size());
}

bool IsBottomSlab(BlockId id) {
    return id == Block::STONE_SLAB;
}

bool IsTopSlab(BlockId id) {
    return id == Block::STONE_SLAB_TOP;
}

bool IsSlab(BlockId id) {
    return IsBottomSlab(id) || IsTopSlab(id);
}

BlockId SlabDoubleResult(BlockId a, BlockId b) {
    // Matching stone half-slabs combine into full stone.
    if ((IsBottomSlab(a) && IsTopSlab(b)) || (IsTopSlab(a) && IsBottomSlab(b))) {
        return Block::STONE;
    }
    if (IsBottomSlab(a) && IsBottomSlab(b)) return Block::STONE;
    if (IsTopSlab(a) && IsTopSlab(b)) return Block::STONE;
    return Block::AIR;
}

bool OccludesNeighborFace(BlockId selfId, BlockId neighborId, int face) {
    if (neighborId == Block::AIR) return false;
    const BlockDefinition& self = GetBlockDef(selfId);
    const BlockDefinition& n = GetBlockDef(neighborId);
    if (!n.opaque) return false;

    constexpr float eps = 0.001f;
    const float ox = (float)FACE_DIRS[face][0];
    const float oy = (float)FACE_DIRS[face][1];
    const float oz = (float)FACE_DIRS[face][2];

    // Neighbor collision expressed in *this* cell's local space.
    const float nMinX = n.collisionMin.x + ox;
    const float nMaxX = n.collisionMax.x + ox;
    const float nMinY = n.collisionMin.y + oy;
    const float nMaxY = n.collisionMax.y + oy;
    const float nMinZ = n.collisionMin.z + oz;
    const float nMaxZ = n.collisionMax.z + oz;

    switch (face) {
        case 0: // +Y face of self at y = collisionMax.y
            return fabsf(nMinY - self.collisionMax.y) <= eps &&
                   Covers1D(nMinX, nMaxX, self.collisionMin.x, self.collisionMax.x, eps) &&
                   Covers1D(nMinZ, nMaxZ, self.collisionMin.z, self.collisionMax.z, eps);
        case 1: // -Y face of self at y = collisionMin.y
            return fabsf(nMaxY - self.collisionMin.y) <= eps &&
                   Covers1D(nMinX, nMaxX, self.collisionMin.x, self.collisionMax.x, eps) &&
                   Covers1D(nMinZ, nMaxZ, self.collisionMin.z, self.collisionMax.z, eps);
        case 2: // +X
            return fabsf(nMinX - self.collisionMax.x) <= eps &&
                   Covers1D(nMinY, nMaxY, self.collisionMin.y, self.collisionMax.y, eps) &&
                   Covers1D(nMinZ, nMaxZ, self.collisionMin.z, self.collisionMax.z, eps);
        case 3: // -X
            return fabsf(nMaxX - self.collisionMin.x) <= eps &&
                   Covers1D(nMinY, nMaxY, self.collisionMin.y, self.collisionMax.y, eps) &&
                   Covers1D(nMinZ, nMaxZ, self.collisionMin.z, self.collisionMax.z, eps);
        case 4: // +Z
            return fabsf(nMinZ - self.collisionMax.z) <= eps &&
                   Covers1D(nMinX, nMaxX, self.collisionMin.x, self.collisionMax.x, eps) &&
                   Covers1D(nMinY, nMaxY, self.collisionMin.y, self.collisionMax.y, eps);
        case 5: // -Z
            return fabsf(nMaxZ - self.collisionMin.z) <= eps &&
                   Covers1D(nMinX, nMaxX, self.collisionMin.x, self.collisionMax.x, eps) &&
                   Covers1D(nMinY, nMaxY, self.collisionMin.y, self.collisionMax.y, eps);
        default:
            return false;
    }
}

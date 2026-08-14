#include "block.h"

#include <cstdio>
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

// Builtin table — registration order MUST match enum Block ordinals.
const BlockDefinition kBuiltinBlocks[] = {
    {"game:air",         {{},{},{},{},{},{}}, false, 0, kFullMin, kFullMax, false},
    {"game:bedrock",     {{15,15},{15,15},{15,15},{15,15},{15,15},{15,15}}, false, 0, kFullMin, kFullMax, true},
    {"game:grass",       {{0,0}, {1,0}, {0,1}, {0,1}, {0,1}, {0,1}}, false, 0, kFullMin, kFullMax, true},
    {"game:dirt",        {{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}}, false, 0, kFullMin, kFullMax, true},
    {"game:stone",       {{2,0},{2,0},{2,0},{2,0},{2,0},{2,0}}, false, 0, kFullMin, kFullMax, true},
    {"game:light_stone", {{1,1},{1,1},{1,1},{1,1},{1,1},{1,1}}, true, 15, kFullMin, kFullMax, true},
    {"game:wood",        {{0,2},{0,2},{1,2},{1,2},{1,2},{1,2}}, false, 0, kFullMin, kFullMax, true},
    {"game:planks",      {{0,3},{0,3},{0,3},{0,3},{0,3},{0,3}}, false, 0, kFullMin, kFullMax, true},
    {"game:brick",       {{2,1},{2,1},{2,1},{2,1},{2,1},{2,1}}, false, 0, kFullMin, kFullMax, true},
    {"game:sand",        {{2,2},{2,2},{2,2},{2,2},{2,2},{2,2}}, false, 0, kFullMin, kFullMax, true},
    // Bottom slab — same stone texture; half-height collision for step-up.
    {"game:stone_slab",  {{2,0},{2,0},{2,0},{2,0},{2,0},{2,0}}, false, 0, kSlabMin, kSlabMax, true},
};

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

    // Guard: enum aliases must still match registration order.
    BlockId stoneId = Block::AIR;
    if (!TryGetBlockId("game:stone", stoneId) || stoneId != Block::STONE) {
        printf("InitBlockRegistry: builtin id mismatch (game:stone)\n");
    }
    BlockId slabId = Block::AIR;
    if (!TryGetBlockId("game:stone_slab", slabId) || slabId != Block::STONE_SLAB) {
        printf("InitBlockRegistry: builtin id mismatch (game:stone_slab)\n");
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

bool OccludesNeighborFace(BlockId neighborId, int face) {
    if (neighborId == Block::AIR) return false;
    const BlockDefinition& n = GetBlockDef(neighborId);
    if (!n.blocksMotion) return false;

    constexpr float eps = 0.001f;
    const bool fullX = n.collisionMin.x <= eps && n.collisionMax.x >= 1.0f - eps;
    const bool fullY = n.collisionMin.y <= eps && n.collisionMax.y >= 1.0f - eps;
    const bool fullZ = n.collisionMin.z <= eps && n.collisionMax.z >= 1.0f - eps;

    switch (face) {
        case 0: // +Y face: neighbor above must sit on the bottom of its cell and cover XZ
            return n.collisionMin.y <= eps && fullX && fullZ;
        case 1: // -Y face: neighbor below must reach the top of its cell and cover XZ
            return n.collisionMax.y >= 1.0f - eps && fullX && fullZ;
        case 2: // +X face: neighbor +X must touch its -X side and cover YZ
            return n.collisionMin.x <= eps && fullY && fullZ;
        case 3: // -X face: neighbor -X must touch its +X side and cover YZ
            return n.collisionMax.x >= 1.0f - eps && fullY && fullZ;
        case 4: // +Z face
            return n.collisionMin.z <= eps && fullX && fullY;
        case 5: // -Z face
            return n.collisionMax.z >= 1.0f - eps && fullX && fullY;
        default:
            return false;
    }
}

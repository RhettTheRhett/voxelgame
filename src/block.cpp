#include "block.h"

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

std::vector<BlockDefinition> g_defs;
std::unordered_map<std::string, BlockId> g_byContentId;

// Builtin table — registration order MUST match enum Block ordinals.
const BlockDefinition kBuiltinBlocks[] = {
    {"game:air",         {{},{},{},{},{},{}}, false, 0},
    {"game:bedrock",     {{15,15},{15,15},{15,15},{15,15},{15,15},{15,15}}, false, 0},
    {"game:grass",       {{0,0}, {1,0}, {0,1}, {0,1}, {0,1}, {0,1}}, false, 0},
    {"game:dirt",        {{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}}, false, 0},
    {"game:stone",       {{2,0},{2,0},{2,0},{2,0},{2,0},{2,0}}, false, 0},
    {"game:light_stone", {{1,1},{1,1},{1,1},{1,1},{1,1},{1,1}}, true, 15},
    {"game:wood",        {{0,2},{0,2},{1,2},{1,2},{1,2},{1,2}}, false, 0},
    {"game:planks",      {{0,3},{0,3},{0,3},{0,3},{0,3},{0,3}}, false, 0},
    {"game:brick",       {{2,1},{2,1},{2,1},{2,1},{2,1},{2,1}}, false, 0},
    {"game:sand",        {{2,2},{2,2},{2,2},{2,2},{2,2},{2,2}}, false, 0},
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
}

const BlockDefinition& GetBlockDef(BlockId id) {
    if (g_defs.empty()) {
        // Defensive: allow late init if a call site forgot InitBlockRegistry.
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

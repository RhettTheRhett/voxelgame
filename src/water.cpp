#include "water.h"
#include "world.h"
#include "chunk.h"
#include "block.h"
#include "constants.h"

#include <queue>
#include <unordered_set>
#include <cstdint>

namespace {

struct WaterPos {
    int x, y, z;
    bool operator==(const WaterPos& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct WaterPosHash {
    size_t operator()(const WaterPos& p) const {
        const size_t x = static_cast<size_t>(static_cast<uint32_t>(p.x));
        const size_t y = static_cast<size_t>(static_cast<uint32_t>(p.y));
        const size_t z = static_cast<size_t>(static_cast<uint32_t>(p.z));
        return x * 73856093u ^ y * 19349663u ^ z * 83492791u;
    }
};

struct WaterJob {
    float readyAt;
    int x, y, z;
    bool operator<(const WaterJob& o) const { return readyAt > o.readyAt; }
};

float g_waterTime = 0.0f;
std::priority_queue<WaterJob> g_waterHeap;
std::unordered_set<WaterPos, WaterPosHash> g_waterQueued;

void Enqueue(int x, int y, int z, float delaySec) {
    if (y < 0 || y >= CHUNK_HEIGHT) return;
    WaterPos p{ x, y, z };
    if (g_waterQueued.count(p)) return;
    g_waterQueued.insert(p);
    g_waterHeap.push({ g_waterTime + delaySec, x, y, z });
}

bool CanWaterOccupy(BlockId id) {
    return id == Block::AIR || IsFluid(id);
}

bool BlocksWater(BlockId id) {
    return GetBlockDef(id).blocksMotion;
}

// Air or flowing — never wake neighboring sources (that ping-ponged forever).
bool ShouldWakeForFlow(BlockId id) {
    return id == Block::AIR || (IsFluid(id) && !IsWaterSource(id));
}

void SetWaterBlock(World& world, int x, int y, int z, BlockId id) {
    ChunkCoord coord{};
    int lx = 0, lz = 0;
    if (!WorldToChunkLocal(x, y, z, coord, lx, lz)) return;
    if (world.chunks.count(coord) == 0) return;

    Chunk& chunk = world.chunks.at(coord);
    if (chunk.blocks[lx][y][lz] == id) return;

    chunk.blocks[lx][y][lz] = id;
    chunk.needsSaving = true;
    chunk.meshDirty = true;
}

int CountHorizontalSources(const World& world, int x, int y, int z) {
    static const int dirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    int n = 0;
    for (auto& d : dirs) {
        if (IsWaterSource(GetWorldBlock(world, x + d[0], y, z + d[1]))) n++;
    }
    return n;
}

// Flowing level L is only legal if a real parent feeds it (source / L-1 / fluid above).
// Same-level neighbors must NOT keep each other alive.
bool HasFlowSupport(const World& world, int x, int y, int z, int level) {
    if (level <= 0) return true;
    if (IsFluid(GetWorldBlock(world, x, y + 1, z))) return true;

    static const int dirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    for (auto& d : dirs) {
        int nl = WaterLevel(GetWorldBlock(world, x + d[0], y, z + d[1]));
        if (nl < 0) continue;
        if (nl == 0 && level == 1) return true;
        if (nl == level - 1) return true;
    }
    return false;
}

int ComputeIncomingLevel(const World& world, int x, int y, int z) {
    // Infinite source only on a solid floor — stops mid-air / column runaway.
    if (CountHorizontalSources(world, x, y, z) >= 2) {
        BlockId below = GetWorldBlock(world, x, y - 1, z);
        if (BlocksWater(below)) return 0;
    }

    int best = 8;

    if (IsFluid(GetWorldBlock(world, x, y + 1, z))) {
        best = 1;
    }

    static const int dirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    for (auto& d : dirs) {
        int nl = WaterLevel(GetWorldBlock(world, x + d[0], y, z + d[1]));
        if (nl < 0) continue;
        // Only accept a parent that is strictly "stronger" (lower level number).
        int next = (nl == 0) ? 1 : (nl + 1);
        if (next <= 7 && next < best) best = next;
    }
    return best;
}

void WakeNeighbors(int x, int y, int z, float delay) {
    static const int dirs[6][3] = {
        {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
    };
    for (auto& d : dirs) {
        Enqueue(x + d[0], y + d[1], z + d[2], delay);
    }
}

void SpreadFrom(World& world, int x, int y, int z, int level) {
    const float delay = WATER_FLOW_DELAY;

    BlockId below = GetWorldBlock(world, x, y - 1, z);
    if (ShouldWakeForFlow(below)) Enqueue(x, y - 1, z, delay);

    if (level >= 7) return;

    static const int dirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
    for (auto& d : dirs) {
        BlockId n = GetWorldBlock(world, x + d[0], y, z + d[1]);
        if (ShouldWakeForFlow(n)) Enqueue(x + d[0], y, z + d[1], delay);
    }
}

void Evaporate(World& world, int x, int y, int z) {
    SetWaterBlock(world, x, y, z, Block::AIR);
    WakeNeighbors(x, y, z, WATER_FLOW_DELAY);
}

void UpdateWaterCell(World& world, int x, int y, int z) {
    BlockId cur = GetWorldBlock(world, x, y, z);

    if (BlocksWater(cur) && !IsFluid(cur)) return;

    if (IsWaterSource(cur)) {
        SpreadFrom(world, x, y, z, 0);
        return;
    }

    // Existing flowing with no path back to a source → delete (no mutual sustain).
    if (IsFluid(cur)) {
        int curLevel = WaterLevel(cur);
        if (!HasFlowSupport(world, x, y, z, curLevel)) {
            Evaporate(world, x, y, z);
            return;
        }
    }

    int incoming = ComputeIncomingLevel(world, x, y, z);

    // New water only if supported at that level (or becoming a floor infinite source).
    if (incoming <= 7) {
        if (incoming > 0 && !HasFlowSupport(world, x, y, z, incoming)) {
            // Candidate level isn't actually fed — stay dry / evaporate.
            if (IsFluid(cur)) Evaporate(world, x, y, z);
            return;
        }

        BlockId want = WaterFromLevel(incoming);
        if (cur != want) {
            SetWaterBlock(world, x, y, z, want);
            cur = want;
            WakeNeighbors(x, y, z, WATER_FLOW_DELAY);
        }
    } else {
        if (IsFluid(cur)) Evaporate(world, x, y, z);
        return;
    }

    int level = WaterLevel(cur);
    if (level < 0) return;
    SpreadFrom(world, x, y, z, level);
}

// Prod all flowing water nearby so orphan puddles get a chance to die.
void WakeFlowingInRadius(World& world, int x, int y, int z, int radius) {
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                int wx = x + dx;
                int wy = y + dy;
                int wz = z + dz;
                BlockId id = GetWorldBlock(world, wx, wy, wz);
                if (IsFluid(id) && !IsWaterSource(id)) {
                    Enqueue(wx, wy, wz, 0.0f);
                }
            }
        }
    }
}

} // namespace

int WaterLevel(BlockId id) {
    switch (id) {
        case Block::WATER:   return 0;
        case Block::WATER_1: return 1;
        case Block::WATER_2: return 2;
        case Block::WATER_3: return 3;
        case Block::WATER_4: return 4;
        case Block::WATER_5: return 5;
        case Block::WATER_6: return 6;
        case Block::WATER_7: return 7;
        default: return -1;
    }
}

BlockId WaterFromLevel(int level) {
    switch (level) {
        case 0: return Block::WATER;
        case 1: return Block::WATER_1;
        case 2: return Block::WATER_2;
        case 3: return Block::WATER_3;
        case 4: return Block::WATER_4;
        case 5: return Block::WATER_5;
        case 6: return Block::WATER_6;
        case 7: return Block::WATER_7;
        default: return Block::AIR;
    }
}

bool IsFluid(BlockId id) {
    return WaterLevel(id) >= 0;
}

bool IsWaterSource(BlockId id) {
    return id == Block::WATER;
}

bool IsReplaceable(BlockId id) {
    return id == Block::AIR || IsFluid(id);
}

float WaterMeshHeight(BlockId id) {
    int level = WaterLevel(id);
    if (level < 0) return 1.0f;
    if (level == 0) return 1.0f;
    return (8.0f - (float)level) / 8.0f;
}

float WaterCornerHeight(const World& world, int bx, int by, int bz, int cx, int cz) {
    float sum = 0.0f;
    int count = 0;
    bool underFluid = false;

    for (int dx = cx - 1; dx <= cx; dx++) {
        for (int dz = cz - 1; dz <= cz; dz++) {
            const int x = bx + dx;
            const int z = bz + dz;
            if (IsFluid(GetWorldBlock(world, x, by + 1, z))) {
                underFluid = true;
            }
            BlockId id = GetWorldBlock(world, x, by, z);
            if (IsFluid(id)) {
                sum += WaterMeshHeight(id);
                count++;
            }
        }
    }

    // Fully submerged corner → flat full height (no early-out on nearby sources —
    // that made placing sources underwater spike random corners).
    if (underFluid) return 1.0f;
    if (count == 0) return WaterMeshHeight(GetWorldBlock(world, bx, by, bz));
    return sum / (float)count;
}

void NotifyWaterChange(World& world, int x, int y, int z) {
    Enqueue(x, y, z, 0.0f);
    Enqueue(x + 1, y, z, 0.0f);
    Enqueue(x - 1, y, z, 0.0f);
    Enqueue(x, y, z + 1, 0.0f);
    Enqueue(x, y, z - 1, 0.0f);
    Enqueue(x, y + 1, z, 0.0f);
    Enqueue(x, y - 1, z, 0.0f);
    // Clean up orphan flowing left over from older logic / broken sources.
    WakeFlowingInRadius(world, x, y, z, 8);
}

void ProcessWaterUpdates(World& world, float deltaTime, int budget) {
    g_waterTime += deltaTime;

    while (budget-- > 0 && !g_waterHeap.empty()) {
        WaterJob job = g_waterHeap.top();
        if (job.readyAt > g_waterTime) break;

        g_waterHeap.pop();
        WaterPos p{ job.x, job.y, job.z };
        g_waterQueued.erase(p);
        UpdateWaterCell(world, job.x, job.y, job.z);
    }
}

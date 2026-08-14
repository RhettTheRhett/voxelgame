#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "raylib.h"
#include "block.h"
#include "chunkcoord.h"

struct World;

const int CHUNK_SIZE = 16;
const int CHUNK_HEIGHT = 128;

// Integer floor-div / positive-mod so world (16,z) is always chunk 1 local 0,
// never local 16 (out of bounds — caused floor holes on chunk seams).
inline int DivFloor(int a, int b) {
    int q = a / b;
    int r = a % b;
    if (r != 0 && a < 0) q--;
    return q;
}
inline int ModFloor(int a, int b) {
    int r = a % b;
    if (r < 0) r += b;
    return r;
}
inline bool WorldToChunkLocal(int wx, int wy, int wz, ChunkCoord& coord, int& lx, int& lz) {
    if (wy < 0 || wy >= CHUNK_HEIGHT) return false;
    coord.x = DivFloor(wx, CHUNK_SIZE);
    coord.z = DivFloor(wz, CHUNK_SIZE);
    lx = ModFloor(wx, CHUNK_SIZE);
    lz = ModFloor(wz, CHUNK_SIZE);
    return true;
}

constexpr uint32_t CHUNK_FILE_SIGNATURE = 0x564F5843;
constexpr uint8_t  CHUNK_FILE_VERSION = 1;

static const std::string CHUNK_PATH = "saves/world";

// Cave generation parameters
static const float CAVE_CHAMBER_SCALE       = 0.008f;
static const int   CAVE_CHAMBER_OCTAVES     = 3;
static const float CAVE_CHAMBER_PERSISTENCE = 0.5f;

static const float CAVE_TUNNEL_SCALE        = 0.055f;
static const int   CAVE_TUNNEL_OCTAVES      = 3;
static const float CAVE_TUNNEL_PERSISTENCE  = 0.25f;

static const float CAVE_THRESHOLD           = 0.7f;
static const float CAVE_SURFACE_FADE_DEPTH  = 4.0f;

struct LightNode {
    int worldX, worldY, worldZ;
    uint8_t level;
};

struct Chunk {
    
    uint16_t blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    uint8_t sunLight[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    uint8_t blockLight[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    Vector3 position;
    Mesh mesh;
    bool meshDirty;
    bool needsSaving;
};

//void DrawChunk(const Chunk& chunk);
BlockId GetWorldBlock(const World& world, int worldBlockX, int worldBlockY, int worldBlockZ);
bool IsSolid(const World& world, int worldBlockX, int worldBlockY, int worldBlockZ);
Mesh BuildChunkMesh(const Chunk& chunk, const World& world, int chunkX, int chunkZ);
void GenerateChunk(Chunk& chunk, int chunkX, int chunkZ, float scale, int octaves, float persistence);

void PropagateSunlight(World& world, const std::vector<ChunkCoord>& affectedChunks);
void PropagateBlockLight(World& world, const std::vector<ChunkCoord>& affectedChunks);
void PropagateBlockLightOnChunkLoad(World& world, int chunkX, int chunkZ);
void ClearBlockLight(World& world, const std::vector<ChunkCoord>& affectedChunks);

std::vector<ChunkCoord> GetAffectedChunks(int chunkX, int chunkZ);

void PropagateSunlight(World& world, int chunkX, int chunkZ);
void PropagateBlockLight(World& world, int chunkX, int chunkZ);
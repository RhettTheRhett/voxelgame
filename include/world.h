#pragma once
#include "chunk.h"
#include "block.h"
#include "chunkcoord.h"
#include "saveformat.h"
#include <unordered_map>


struct World {
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks;
    WorldManifest manifest;
    float noiseScale;
    int noiseOctaves;
    float noisePersistence;
};

// forward declarations
void GenerateWorld(World& world, int renderDistance, int playerChunkX, int playerChunkZ);
void DrawWorld(World& world, Material& mat);
void UnloadDistantChunks(World& world, int playerChunkX, int playerChunkZ, int renderDistance);
void UnloadAllChunks(World& world);
void SetBlock(World& world, int worldX, int worldY, int worldZ, BlockId type);

struct Player;
struct RayHit;
struct Ray;

// Right-click place: slab stacking, no overwrite, no embed-in-player for solids.
bool TryPlaceBlock(World& world, const Player& player, const Ray& ray, const RayHit& hit, BlockId held);
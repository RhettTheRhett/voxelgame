#pragma once
#include "chunk.h"
#include "block.h"
#include "chunkcoord.h"
#include <unordered_map>


struct World {
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks;
    int32_t seed;
    float noiseScale;
    int noiseOctaves;
    float noisePersistence;
};

// forward declarations
void GenerateWorld(World& world, int renderDistance, int playerChunkX, int playerChunkZ);
void DrawWorld(World& world, Material& mat);
void UnloadDistantChunks(World& world, int playerChunkX, int playerChunkZ, int renderDistance);
void SetBlock(World& world, int worldX, int worldY, int worldZ, Block type);
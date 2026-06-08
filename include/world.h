#pragma once
#include "chunk.h"
#include <unordered_map>

struct ChunkCoord {
    int x, z;
    bool operator==(const ChunkCoord& o) const {
        return x == o.x && z == o.z;
    }
};

struct ChunkCoordHash {
    size_t operator()(const ChunkCoord& c) const {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.z) << 16);
    }
};

struct World {
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks;
    int seed;
    float noiseScale;
    int noiseOctaves;
    float noisePersistence;
};

// forward declarations
void GenerateWorld(World& world, int renderDistance, int playerChunkX, int playerChunkZ);
void DrawWorld(World& world, Material& mat);
void UnloadDistantChunks(World& world, int playerChunkX, int playerChunkZ, int renderDistance);
#pragma once

#include <cstdint>
#include "raylib.h"

struct World;

const int CHUNK_SIZE = 16;
const int CHUNK_HEIGHT = 64;
struct Chunk {
    
    uint16_t blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    Vector3 position;
    Mesh mesh;
    bool meshDirty;
};

void DrawChunk(const Chunk& chunk);
bool IsSolid(const World& world, int worldBlockX, int worldBlockY, int worldBlockZ);
Mesh BuildChunkMesh(const Chunk& chunk, const World& world, int chunkX, int chunkZ);
void GenerateChunk(Chunk& chunk, int chunkX, int chunkZ, float scale, int octaves, float persistence);
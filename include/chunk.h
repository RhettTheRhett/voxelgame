#pragma once

#include <cstdint>
#include "raylib.h"

const int CHUNK_SIZE = 16;
struct Chunk {
    
    uint16_t blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    Vector3 position;
    Mesh mesh;
    bool meshDirty;
};

void DrawChunk(const Chunk& chunk);
bool IsSolid(const Chunk& chunk, int x, int y, int z);
Mesh BuildChunkMesh(const Chunk& chunk);

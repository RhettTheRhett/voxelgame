#pragma once

#include <cstdint>
#include "raylib.h"

struct World;

const int CHUNK_SIZE = 16;
const int CHUNK_HEIGHT = 128;

// Cave generation parameters
static const float CAVE_CHAMBER_SCALE       = 0.008f;
static const int   CAVE_CHAMBER_OCTAVES     = 3;
static const float CAVE_CHAMBER_PERSISTENCE = 0.5f;

static const float CAVE_TUNNEL_SCALE        = 0.025f;
static const int   CAVE_TUNNEL_OCTAVES      = 2;
static const float CAVE_TUNNEL_PERSISTENCE  = 0.5f;

static const float CAVE_THRESHOLD           = 0.75f;
static const float CAVE_SURFACE_FADE_DEPTH  = 10.0f;

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
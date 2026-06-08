#include "world.h"
#include "chunk.h"
#include "raymath.h"

void GenerateWorld(World& world, int renderDistance, int playerChunkX, int playerChunkZ) {
    for (int dx = -renderDistance; dx <= renderDistance; dx++) {
        for (int dz = -renderDistance; dz <= renderDistance; dz++) {
            ChunkCoord coord = { playerChunkX + dx, playerChunkZ + dz };

            // 1. skip if chunk already exists in world.chunks
            if(world.chunks.count(coord) > 0) continue;
            // 2. create a new Chunk, set its position
            Chunk chunk = {};
            chunk.position = {(float) coord.x * CHUNK_SIZE, 0.0f, (float) coord.z * CHUNK_SIZE};

            // 3. call GenerateChunk with world noise params
            GenerateChunk(chunk, coord.x, coord.z, world.noiseScale, world.noiseOctaves, world.noisePersistence);
            // 4. insert it into world.chunks
            world.chunks[coord] = chunk;
        }
    }
}

void DrawWorld(World& world, Material& mat){
    for (auto& [coord, chunk] : world.chunks) {
        if (chunk.meshDirty) {
            UnloadMesh(chunk.mesh);
            chunk.mesh = BuildChunkMesh(chunk);
            chunk.meshDirty = false;
        }

        Matrix transform = MatrixTranslate(
            chunk.position.x,
            chunk.position.y,
            chunk.position.z
        );
        DrawMesh(chunk.mesh, mat, transform);
    }
}
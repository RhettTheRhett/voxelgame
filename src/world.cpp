#include "world.h"
#include "chunk.h"
#include "raymath.h"
#include "saveload.h"


#include <iostream>
#include <filesystem>
#include <string>

void GenerateWorld(World& world, int renderDistance, int playerChunkX, int playerChunkZ) {
    for (int dx = -renderDistance; dx <= renderDistance; dx++) {
        for (int dz = -renderDistance; dz <= renderDistance; dz++) {
            ChunkCoord coord = { playerChunkX + dx, playerChunkZ + dz };
            std::string chunkPath = GetChunkFilePath(CHUNK_PATH, coord.x, coord.z);

            Chunk chunk = {};
            chunk.position = {(float) coord.x * CHUNK_SIZE, 0.0f, (float) coord.z * CHUNK_SIZE};

            // skip if chunk already exists in world.chunks
            if(world.chunks.count(coord) > 0) continue;

            if(std::filesystem::exists(chunkPath)){
                LoadChunk(chunk, coord.x, coord.z, chunkPath);
            }else{
                GenerateChunk(chunk, coord.x, coord.z, world.noiseScale, world.noiseOctaves, world.noisePersistence);
            }
            world.chunks[coord] = chunk;

            auto affected = GetAffectedChunks(coord.x, coord.z);
            PropagateSunlight(world, affected);
            PropagateBlockLightOnChunkLoad(world, coord.x, coord.z);
           
        }
    }
}

void DrawWorld(World& world, Material& mat){
    // pass 1 — build all dirty meshes first
    for (auto& [coord, chunk] : world.chunks) {
        if (chunk.meshDirty) {
            if (chunk.mesh.vaoId != 0) UnloadMesh(chunk.mesh);
            chunk.mesh = BuildChunkMesh(chunk, world, coord.x, coord.z);
            chunk.meshDirty = false;
        }
    }

    // pass 2 — draw everything
    for (auto& [coord, chunk] : world.chunks) {
        Matrix transform = MatrixTranslate(
            chunk.position.x,
            chunk.position.y,
            chunk.position.z
        );
        DrawMesh(chunk.mesh, mat, transform);
    }
}

void UnloadDistantChunks(World& world, int playerChunkX, int playerChunkZ, int renderDistance){
    // collect keys to erase first
    std::vector<ChunkCoord> toErase;
    
    for (auto& [coord, chunk] : world.chunks) {
        //int cx = (int)(chunk.position.x / CHUNK_SIZE);
        //int cz = (int)(chunk.position.z / CHUNK_SIZE);
       if ((abs(coord.x - playerChunkX) > renderDistance || abs(coord.z - playerChunkZ) > renderDistance) ) toErase.push_back(coord);
    }
    // then erase them
    
    for (auto& coord : toErase) {
        Chunk& chunk = world.chunks.at(coord);
        //printf("Unloading chunk %d, %d - needsSaving=%d\n", coord.x, coord.z, chunk.needsSaving);
        if (chunk.needsSaving) {
            std::string chunkPath = GetChunkFilePath(CHUNK_PATH, coord.x, coord.z);
            //printf("Saving chunk %d, %d to %s\n", coord.x, coord.z, chunkPath.c_str());
            SaveChunk(chunk, coord.x, coord.z, chunkPath);
        }
        UnloadMesh(chunk.mesh);
        world.chunks.erase(coord);
    }
}

void UnloadAllChunks(World& world) {
    for (auto& [coord, chunk] : world.chunks) {
        if (chunk.mesh.vaoId != 0) {
            UnloadMesh(chunk.mesh);
        }
    }
    world.chunks.clear();
}

void SetBlock(World& world, int worldX, int worldY, int worldZ, BlockId type) {
    ChunkCoord coord{};
    int localX = 0, localZ = 0;
    if (!WorldToChunkLocal(worldX, worldY, worldZ, coord, localX, localZ)) {
        return;
    }
    if (world.chunks.count(coord) == 0) return;

    auto affected = GetAffectedChunks(coord.x, coord.z);

    world.chunks.at(coord).blocks[localX][worldY][localZ] = type;
    world.chunks.at(coord).needsSaving = true;

    ClearBlockLight(world, affected);
    PropagateSunlight(world, affected);  
    PropagateBlockLight(world, affected); 

    for (const ChunkCoord& c : affected) {
        if (world.chunks.count(c)) {
            world.chunks.at(c).meshDirty = true;
        }
    }
}
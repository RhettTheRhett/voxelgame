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
            else if(std::filesystem::exists(chunkPath)){
                //Chunk chunk = {};
                LoadChunk(chunk, coord.x, coord.z, chunkPath);
                PropagateSunlight(chunk);
                world.chunks[coord] = chunk;                
            }else{
            // call GenerateChunk with world noise params
            GenerateChunk(chunk, coord.x, coord.z, world.noiseScale, world.noiseOctaves, world.noisePersistence);
            // insert it into world.chunks
            PropagateSunlight(chunk);
            world.chunks[coord] = chunk; 
            }
           
        }
    }
}

void DrawWorld(World& world, Material& mat){
    // pass 1 — build all dirty meshes first
    for (auto& [coord, chunk] : world.chunks) {
        if (chunk.meshDirty) {
            if (chunk.meshDirty) {
                //TraceLog(LOG_INFO, "Building mesh for chunk %d, %d", coord.x, coord.z);

            }
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
        printf("Unloading chunk %d, %d - needsSaving=%d\n", coord.x, coord.z, chunk.needsSaving);
        if (chunk.needsSaving) {
            std::string chunkPath = GetChunkFilePath(CHUNK_PATH, coord.x, coord.z);
            printf("Saving chunk %d, %d to %s\n", coord.x, coord.z, chunkPath.c_str());
            SaveChunk(chunk, coord.x, coord.z, chunkPath);
        }
        UnloadMesh(chunk.mesh);
        world.chunks.erase(coord);
    }
}

void SetBlock(World& world, int worldX, int worldY, int worldZ, Block type) {
    int chunkX = (int)floor(worldX / (float)CHUNK_SIZE);
    int chunkZ = (int)floor(worldZ / (float)CHUNK_SIZE);
    int localX = worldX - chunkX * CHUNK_SIZE;
    int localZ = worldZ - chunkZ * CHUNK_SIZE;

    ChunkCoord coord = { chunkX, chunkZ };
    auto affected = GetAffectedChunks(chunkX, chunkZ);

    world.chunks.at(coord).blocks[localX][worldY][localZ] = type;
    world.chunks.at(coord).needsSaving = true;

    ClearBlockLight(world, affected);
    PropagateSunlight(world.chunks.at(coord));  // still single-chunk for now
    PropagateBlockLight(world, affected); // still single-chunk for now

    for (const ChunkCoord& c : affected) {
        if (world.chunks.count(c)) {
            world.chunks.at(c).meshDirty = true;
        }
    }
}
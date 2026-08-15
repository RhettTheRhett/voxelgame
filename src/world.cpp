#include "world.h"
#include "chunk.h"
#include "raymath.h"
#include "saveload.h"
#include "player.h"
#include "raycast.h"
#include "collision.h"
#include "water.h"

// Avoid windows.h vs raylib CloseWindow/ShowCursor clash — only need glDepthMask.
extern "C" void __stdcall glDepthMask(unsigned char flag);
#ifndef GL_FALSE
#define GL_FALSE 0
#define GL_TRUE  1
#endif

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

static void UnloadChunkMeshes(Chunk& chunk) {
    if (chunk.mesh.vaoId != 0) {
        UnloadMesh(chunk.mesh);
        chunk.mesh = {};
    }
    if (chunk.translucentMesh.vaoId != 0) {
        UnloadMesh(chunk.translucentMesh);
        chunk.translucentMesh = {};
    }
}

void DrawWorld(World& world, Material& mat){
    // pass 1 — rebuild dirty opaque + translucent meshes
    for (auto& [coord, chunk] : world.chunks) {
        if (chunk.meshDirty) {
            UnloadChunkMeshes(chunk);
            BuildChunkMeshes(chunk, world, coord.x, coord.z, chunk.mesh, chunk.translucentMesh);
            chunk.meshDirty = false;
        }
    }

    // pass 2 — opaque (writes depth)
    for (auto& [coord, chunk] : world.chunks) {
        if (chunk.mesh.vaoId == 0) continue;
        Matrix transform = MatrixTranslate(
            chunk.position.x,
            chunk.position.y,
            chunk.position.z
        );
        DrawMesh(chunk.mesh, mat, transform);
    }

    // pass 3 — translucent (alpha blend, depth test, no depth write)
    BeginBlendMode(BLEND_ALPHA);
    glDepthMask(GL_FALSE);
    for (auto& [coord, chunk] : world.chunks) {
        if (chunk.translucentMesh.vaoId == 0) continue;
        Matrix transform = MatrixTranslate(
            chunk.position.x,
            chunk.position.y,
            chunk.position.z
        );
        DrawMesh(chunk.translucentMesh, mat, transform);
    }
    glDepthMask(GL_TRUE);
    EndBlendMode();
}

void UnloadDistantChunks(World& world, int playerChunkX, int playerChunkZ, int renderDistance){
    // collect keys to erase first
    std::vector<ChunkCoord> toErase;
    
    for (auto& [coord, chunk] : world.chunks) {
       if ((abs(coord.x - playerChunkX) > renderDistance || abs(coord.z - playerChunkZ) > renderDistance) ) toErase.push_back(coord);
    }
    
    for (auto& coord : toErase) {
        Chunk& chunk = world.chunks.at(coord);
        if (chunk.needsSaving) {
            std::string chunkPath = GetChunkFilePath(CHUNK_PATH, coord.x, coord.z);
            SaveChunk(chunk, coord.x, coord.z, chunkPath);
        }
        UnloadChunkMeshes(chunk);
        world.chunks.erase(coord);
    }
}

void UnloadAllChunks(World& world) {
    for (auto& [coord, chunk] : world.chunks) {
        UnloadChunkMeshes(chunk);
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

    // Water may need to flood into / evaporate from this edit.
    NotifyWaterChange(world, worldX, worldY, worldZ);
}

bool TryPlaceBlock(World& world, const Player& player, const Ray& ray, const RayHit& hit, BlockId held) {
    if (!hit.didHit || held == Block::AIR) return false;

    const int hx = (int)hit.position.x;
    const int hy = (int)hit.position.y;
    const int hz = (int)hit.position.z;
    const BlockId hitId = GetWorldBlock(world, hx, hy, hz);

    int placeX = hx;
    int placeY = hy;
    int placeZ = hz;
    BlockId placeId = held;
    bool placeAdjacent = true;

    if (IsSlab(held) && IsSlab(hitId)) {
        // TECH DEBT: merges to Block::STONE — wrong once other slab materials exist.
        const bool fillBottom =
            IsBottomSlab(hitId) && hit.faceHit == Face::TOP_FACE;
        const bool fillTop =
            IsTopSlab(hitId) && hit.faceHit == Face::BOTTOM_FACE;
        if (fillBottom || fillTop) {
            placeId = Block::STONE;
            placeAdjacent = false;
        }
    }

    if (placeAdjacent) {
        placeX = hx + FACE_DIRS[hit.faceHit][0];
        placeY = hy + FACE_DIRS[hit.faceHit][1];
        placeZ = hz + FACE_DIRS[hit.faceHit][2];

        if (held == Block::STONE_SLAB || held == Block::STONE_SLAB_TOP) {
            Vector3 hitPoint = Vector3Add(ray.position, Vector3Scale(ray.direction, hit.distance));
            if (hit.faceHit == Face::TOP_FACE) {
                placeId = Block::STONE_SLAB;
            } else if (hit.faceHit == Face::BOTTOM_FACE) {
                placeId = Block::STONE_SLAB_TOP;
            } else {
                float localY = hitPoint.y - (float)hy;
                placeId = (localY > 0.5f) ? Block::STONE_SLAB_TOP : Block::STONE_SLAB;
            }
        }

        BlockId dest = GetWorldBlock(world, placeX, placeY, placeZ);
        if (!IsReplaceable(dest)) {
            return false;
        }

        // Nothing sits on the water surface for now (no lilypads/boats yet).
        // Replacing fluid in-cell is fine; placing into air above fluid is not.
        if (dest == Block::AIR && IsFluid(GetWorldBlock(world, placeX, placeY - 1, placeZ))) {
            return false;
        }
    }

    if (GetBlockDef(placeId).blocksMotion) {
        BoundingBox blockBox = MakeBlockCollisionBounds(placeX, placeY, placeZ, placeId);
        if (Overlaps(player.GetBounds(), blockBox)) {
            return false;
        }
    }

    SetBlock(world, placeX, placeY, placeZ, placeId);
    return true;
}

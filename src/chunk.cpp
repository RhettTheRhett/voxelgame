#include "chunk.h"
#include "raylib.h"
#include "noise.h"
#include "world.h"
#include "block.h"
#include "chunkcoord.h"
#include "water.h"
#include <queue>

static const Chunk* FindChunkAt(const World& world, int wx, int wy, int wz, int& lx, int& lz)
{
    ChunkCoord coord{};
    if (!WorldToChunkLocal(wx, wy, wz, coord, lx, lz)) return nullptr;
    auto it = world.chunks.find(coord);
    if (it == world.chunks.end()) return nullptr;
    return &it->second;
}

BlockId GetWorldBlock(const World& world, int worldBlockX, int worldBlockY, int worldBlockZ)
{
    int localX = 0, localZ = 0;
    const Chunk* chunk = FindChunkAt(world, worldBlockX, worldBlockY, worldBlockZ, localX, localZ);
    if (!chunk) return Block::AIR;
    return chunk->blocks[localX][worldBlockY][localZ];
}

bool IsSolid(const World& world, int worldBlockX, int worldBlockY, int worldBlockZ)
{
    int localX = 0, localZ = 0;
    const Chunk* chunk = FindChunkAt(world, worldBlockX, worldBlockY, worldBlockZ, localX, localZ);
    if (!chunk) return false; // unloaded = air (must match GetWorldBlock)
    return GetBlockDef(chunk->blocks[localX][worldBlockY][localZ]).blocksMotion;
}

bool IsRaycastTarget(const World& world, int worldBlockX, int worldBlockY, int worldBlockZ)
{
    int localX = 0, localZ = 0;
    const Chunk* chunk = FindChunkAt(world, worldBlockX, worldBlockY, worldBlockZ, localX, localZ);
    if (!chunk) return false;
    const BlockDefinition& def = GetBlockDef(chunk->blocks[localX][worldBlockY][localZ]);
    // Solids + cross plants (mushrooms): fluids stay pass-through for mining/placing.
    return def.blocksMotion || def.meshShape == BlockMeshShape::Cross;
}

static uint8_t GetSunLight(const World& world, int worldBlockX, int worldBlockY, int worldBlockZ)
{
    int localX = 0, localZ = 0;
    const Chunk* chunk = FindChunkAt(world, worldBlockX, worldBlockY, worldBlockZ, localX, localZ);
    if (!chunk) return 0;
    return chunk->sunLight[localX][worldBlockY][localZ];
}

static uint8_t GetBlockLight(const World& world, int worldBlockX, int worldBlockY, int worldBlockZ)
{
    int localX = 0, localZ = 0;
    const Chunk* chunk = FindChunkAt(world, worldBlockX, worldBlockY, worldBlockZ, localX, localZ);
    if (!chunk) return 0;
    return chunk->blockLight[localX][worldBlockY][localZ];
}

struct MeshWriter {
    Mesh mesh{};
    int vertCursor = 0;
    int indexCursor = 0;
    int colorCursor = 0;
    int textureCursor = 0;

    void Init(int maxFaces) {
        mesh = {};
        mesh.vertexCount = maxFaces * 4;
        mesh.triangleCount = maxFaces * 2;
        mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
        mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));
        mesh.colors = (unsigned char*)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));
        mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    }

    Mesh Finalize() {
        mesh.vertexCount = vertCursor / 3;
        mesh.triangleCount = indexCursor / 3;
        if (mesh.vertexCount > 0) {
            UploadMesh(&mesh, false);
        } else {
            // Empty mesh — free CPU buffers; leave vaoId 0 so draw skips it.
            if (mesh.vertices) { MemFree(mesh.vertices); mesh.vertices = nullptr; }
            if (mesh.indices) { MemFree(mesh.indices); mesh.indices = nullptr; }
            if (mesh.colors) { MemFree(mesh.colors); mesh.colors = nullptr; }
            if (mesh.texcoords) { MemFree(mesh.texcoords); mesh.texcoords = nullptr; }
        }
        return mesh;
    }
};

static void EmitFaceUVs(MeshWriter& w, int face, float u0, float v0, float u1, float v1) {
    switch (face) {
        case 0:
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v1;
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v1;
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v0;
            break;
        case 1:
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v1;
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v1;
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v0;
            break;
        case 2:
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v1;
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v1;
            break;
        case 3:
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v1;
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v1;
            break;
        case 4:
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v1;
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v1;
            break;
        default:
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v0;
            w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = v1;
            w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = v1;
            break;
    }
}

// One vertical plant plane, double-sided. UVs are bound to corners by role
// (bottom=stem, top=cap), not by triangle winding — so the back face can't
// accidentally map stem along a horizontal edge.
static void EmitDoubleSidedPlantPlane(
    MeshWriter& w,
    float blx, float bly, float blz,
    float brx, float bry, float brz,
    float trx, float try_, float trz,
    float tlx, float tly, float tlz,
    float u0, float vStem,
    float u1, float vCap,
    unsigned char sunShade,
    unsigned char blockShade
) {
    int base = w.vertCursor / 3;

    w.mesh.vertices[w.vertCursor++] = blx; w.mesh.vertices[w.vertCursor++] = bly; w.mesh.vertices[w.vertCursor++] = blz;
    w.mesh.vertices[w.vertCursor++] = brx; w.mesh.vertices[w.vertCursor++] = bry; w.mesh.vertices[w.vertCursor++] = brz;
    w.mesh.vertices[w.vertCursor++] = trx; w.mesh.vertices[w.vertCursor++] = try_; w.mesh.vertices[w.vertCursor++] = trz;
    w.mesh.vertices[w.vertCursor++] = tlx; w.mesh.vertices[w.vertCursor++] = tly; w.mesh.vertices[w.vertCursor++] = tlz;

    // Front
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 0);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 1);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 2);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 0);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 2);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 3);
    // Back (same verts/UVs, reversed winding)
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 0);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 3);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 2);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 0);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 2);
    w.mesh.indices[w.indexCursor++] = (unsigned short)(base + 1);

    for (int v = 0; v < 4; v++) {
        w.mesh.colors[w.colorCursor++] = sunShade;
        w.mesh.colors[w.colorCursor++] = sunShade;
        w.mesh.colors[w.colorCursor++] = sunShade;
        w.mesh.colors[w.colorCursor++] = blockShade;
    }

    // BL, BR = stem (atlas tile bottom); TR, TL = cap (atlas tile top)
    w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = vStem;
    w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = vStem;
    w.mesh.texcoords[w.textureCursor++] = u1; w.mesh.texcoords[w.textureCursor++] = vCap;
    w.mesh.texcoords[w.textureCursor++] = u0; w.mesh.texcoords[w.textureCursor++] = vCap;
}

static void EmitCrossPlant(
    MeshWriter& w,
    int x, int y, int z,
    const BlockDefinition& def,
    uint8_t sunLight,
    uint8_t blockLight,
    float atlasTileSize
) {
    const float MIN_LIGHT = 0.15f;
    float sunFactor = MIN_LIGHT + (1.0f - MIN_LIGHT) * (sunLight / 15.0f);
    unsigned char sunShade = (unsigned char)(220 * sunFactor);
    unsigned char blockShade = (unsigned char)(255 * (blockLight / 15.0f));

    Vector2 tile = def.FACE_TEX[0];
    float u0 = tile.x * atlasTileSize;
    float vTop = tile.y * atlasTileSize;           // cap (top of tile in atlas image)
    float u1 = u0 + atlasTileSize;
    float vStem = vTop + atlasTileSize;            // stem (bottom of tile)

    const float x0 = (float)x, x1 = (float)x + 1.0f;
    const float y0 = (float)y, y1 = (float)y + 1.0f;
    const float z0 = (float)z, z1 = (float)z + 1.0f;

    // Diagonal X — two planes, each emitted once with double-sided indices.
    EmitDoubleSidedPlantPlane(
        w,
        x0, y0, z0,  x1, y0, z1,  x1, y1, z1,  x0, y1, z0,
        u0, vStem, u1, vTop, sunShade, blockShade);
    EmitDoubleSidedPlantPlane(
        w,
        x1, y0, z0,  x0, y0, z1,  x0, y1, z1,  x1, y1, z0,
        u0, vStem, u1, vTop, sunShade, blockShade);
}

void BuildChunkMeshes(const Chunk& chunk, const World& world, int chunkX, int chunkZ,
                      Mesh& outOpaque, Mesh& outTranslucent) {
    const int MAX_FACES = (CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE / 2) * 6;
    const float ATLAS_TILE_SIZE = 1.0f / 16.0f;

    MeshWriter opaque{};
    MeshWriter translucent{};
    opaque.Init(MAX_FACES);
    // Water lakes need more translucent faces than glass houses.
    translucent.Init(MAX_FACES / 2 + 64);

    for (int x = 0; x < CHUNK_SIZE; x++)
    for (int y = 0; y < CHUNK_HEIGHT; y++)
    for (int z = 0; z < CHUNK_SIZE; z++) {
        BlockId blockType = chunk.blocks[x][y][z];
        if (blockType == Block::AIR) continue;

        const BlockDefinition& def = GetBlockDef(blockType);
        MeshWriter& w = def.translucent ? translucent : opaque;
        const bool fluid = IsFluid(blockType);
        const int worldBX = chunkX * CHUNK_SIZE + x;
        const int worldBZ = chunkZ * CHUNK_SIZE + z;

        if (def.meshShape == BlockMeshShape::Cross) {
            EmitCrossPlant(
                w, x, y, z, def,
                chunk.sunLight[x][y][z],
                chunk.blockLight[x][y][z],
                ATLAS_TILE_SIZE
            );
            continue;
        }

        float c00 = 1.0f, c10 = 1.0f, c01 = 1.0f, c11 = 1.0f;
        if (fluid) {
            c00 = WaterCornerHeight(world, worldBX, y, worldBZ, 0, 0);
            c10 = WaterCornerHeight(world, worldBX, y, worldBZ, 1, 0);
            c01 = WaterCornerHeight(world, worldBX, y, worldBZ, 0, 1);
            c11 = WaterCornerHeight(world, worldBX, y, worldBZ, 1, 1);
        }

        for (int f = 0; f < 6; f++) {
            int nx = x + FACE_DIRS[f][0];
            int ny = y + FACE_DIRS[f][1];
            int nz = z + FACE_DIRS[f][2];

            int worldX = chunkX * CHUNK_SIZE + x + FACE_DIRS[f][0];
            int worldY = y + FACE_DIRS[f][1];
            int worldZ = chunkZ * CHUNK_SIZE + z + FACE_DIRS[f][2];

            BlockId neighborId = Block::AIR;
            if (nx >= 0 && nx < CHUNK_SIZE &&
                ny >= 0 && ny < CHUNK_HEIGHT &&
                nz >= 0 && nz < CHUNK_SIZE) {
                neighborId = chunk.blocks[nx][ny][nz];
            } else {
                neighborId = GetWorldBlock(world, worldX, worldY, worldZ);
            }

            if (OccludesNeighborFace(blockType, neighborId, f)) continue;
            // Same non-opaque cube (glass / leaves): skip shared interior faces.
            if (!def.opaque && !fluid && neighborId == blockType) continue;

            if (fluid && IsFluid(neighborId)) {
                // Never draw water-against-water faces (sides looked like blue walls
                // when seen through the volume). Height shows on the sloped top only.
                continue;
            }

            int baseVertex = w.vertCursor / 3;
            for (int v = 0; v < 4; v++) {
                float lx = FACE_VERTS[f][v * 3 + 0];
                float ly = FACE_VERTS[f][v * 3 + 1];
                float lz = FACE_VERTS[f][v * 3 + 2];

                float px, py, pz;
                if (fluid) {
                    px = (float)x + lx;
                    pz = (float)z + lz;
                    float topY;
                    if (lx < 0.5f && lz < 0.5f) topY = c00;
                    else if (lx >= 0.5f && lz < 0.5f) topY = c10;
                    else if (lx < 0.5f && lz >= 0.5f) topY = c01;
                    else topY = c11;

                    if (f == 0) {
                        // Sloped / pointed top from corner heights.
                        py = (float)y + topY;
                    } else if (f == 1) {
                        py = (float)y;
                    } else {
                        // Side: top edge follows the slope, bottom stays at cell floor.
                        py = (float)y + (ly > 0.5f ? topY : 0.0f);
                    }
                } else {
                    px = x + def.collisionMin.x + lx * (def.collisionMax.x - def.collisionMin.x);
                    py = y + def.collisionMin.y + ly * (def.collisionMax.y - def.collisionMin.y);
                    pz = z + def.collisionMin.z + lz * (def.collisionMax.z - def.collisionMin.z);
                }

                w.mesh.vertices[w.vertCursor++] = px;
                w.mesh.vertices[w.vertCursor++] = py;
                w.mesh.vertices[w.vertCursor++] = pz;
            }

            w.mesh.indices[w.indexCursor++] = (unsigned short)(baseVertex + 0);
            w.mesh.indices[w.indexCursor++] = (unsigned short)(baseVertex + 3);
            w.mesh.indices[w.indexCursor++] = (unsigned short)(baseVertex + 2);
            w.mesh.indices[w.indexCursor++] = (unsigned short)(baseVertex + 0);
            w.mesh.indices[w.indexCursor++] = (unsigned short)(baseVertex + 2);
            w.mesh.indices[w.indexCursor++] = (unsigned short)(baseVertex + 1);

            unsigned char shade;
            switch (f) {
                case 0: shade = 255; break;
                case 1: shade = 60;  break;
                case 2: shade = 180; break;
                case 3: shade = 180; break;
                case 4: shade = 220; break;
                case 5: shade = 220; break;
                default: shade = 255; break;
            }

            const float MIN_LIGHT = 0.15f;
            uint8_t neighborSun, neighborBlock;
            if (nx >= 0 && nx < CHUNK_SIZE &&
                ny >= 0 && ny < CHUNK_HEIGHT &&
                nz >= 0 && nz < CHUNK_SIZE) {
                neighborSun   = chunk.sunLight[nx][ny][nz];
                neighborBlock = chunk.blockLight[nx][ny][nz];
            } else {
                neighborSun   = GetSunLight(world, worldX, worldY, worldZ);
                neighborBlock = GetBlockLight(world, worldX, worldY, worldZ);
            }

            // Side faces against air: sample this cell so sides aren't pitch black.
            if (fluid && neighborId == Block::AIR) {
                neighborSun   = chunk.sunLight[x][y][z];
                neighborBlock = chunk.blockLight[x][y][z];
            }

            float sunFactor = MIN_LIGHT + (1.0f - MIN_LIGHT) * (neighborSun / 15.0f);
            unsigned char sunShade = (unsigned char)(shade * sunFactor);
            float blockFactor = neighborBlock / 15.0f;
            unsigned char blockShade = (unsigned char)(255 * blockFactor);

            for (int v = 0; v < 4; v++) {
                w.mesh.colors[w.colorCursor++] = sunShade;
                w.mesh.colors[w.colorCursor++] = sunShade;
                w.mesh.colors[w.colorCursor++] = sunShade;
                w.mesh.colors[w.colorCursor++] = blockShade;
            }

            Vector2 tileCoord = def.FACE_TEX[f];
            float u0 = tileCoord.x * ATLAS_TILE_SIZE;
            float v0 = tileCoord.y * ATLAS_TILE_SIZE;
            float u1 = u0 + ATLAS_TILE_SIZE;
            float v1 = v0 + ATLAS_TILE_SIZE;
            EmitFaceUVs(w, f, u0, v0, u1, v1);
        }
    }

    outOpaque = opaque.Finalize();
    outTranslucent = translucent.Finalize();
}

void GenerateChunk(Chunk& chunk,int chunkX,int chunkZ, float scale,int octaves,float persistence)
{
    for (int x = 0; x < CHUNK_SIZE; x++)
    {
        for (int z = 0; z < CHUNK_SIZE; z++)
        {
            float wx = (chunkX * CHUNK_SIZE + x) * scale;
            float wz = (chunkZ * CHUNK_SIZE + z) * scale;

            // HEIGHTMAP (fast, 2D only)
            float n = FBm2D(wx, wz, octaves, persistence);
            float normalizedHeight = (n + 1.0f) * 0.5f;

            int seaLevel  = CHUNK_HEIGHT / 4;
            int maxHeight = CHUNK_HEIGHT * 3 / 4;

            int height = seaLevel +
                (int)(normalizedHeight * (maxHeight - seaLevel));

            for (int y = 0; y < CHUNK_HEIGHT; y++)
            {
                if (y > height)
                {
                    chunk.blocks[x][y][z] = Block::AIR;
                    continue;
                }
                

                
                float rawX = chunkX * CHUNK_SIZE + x;
                float rawZ = chunkZ * CHUNK_SIZE + z;
                // heightmap uses rawX * scale as before
                float cave1 = FBm3D(
                    rawX * CAVE_CHAMBER_SCALE,y * CAVE_CHAMBER_SCALE,rawZ * CAVE_CHAMBER_SCALE,
                    CAVE_CHAMBER_OCTAVES, CAVE_CHAMBER_PERSISTENCE);

                float cave2 = FBm3D(
                    rawX * CAVE_TUNNEL_SCALE, y * CAVE_TUNNEL_SCALE, rawZ * CAVE_TUNNEL_SCALE, 
                    CAVE_TUNNEL_OCTAVES, CAVE_TUNNEL_PERSISTENCE);

                float cave = fmaxf(cave1, cave2);

                int   depth     = height - y;
                float depthFade = fminf((float)depth / CAVE_SURFACE_FADE_DEPTH, 1.0f);

                if (cave * depthFade > CAVE_THRESHOLD)
                    chunk.blocks[x][y][z] = Block::AIR;
                else {
                    if(depth == 0){
                        chunk.blocks[x][y][z] = Block::GRASS;
                    }
                    else if(depth > 0  && depth <= 6){
                        chunk.blocks[x][y][z] = Block::DIRT;

                    } else{
                        chunk.blocks[x][y][z] = Block::STONE;

                    } 
                }
                if (y == 0){
                    chunk.blocks[x][y][z] = Block::BEDROCK;
                    continue;
                }
                    
            }
        }
    }

    chunk.meshDirty = true;
}



void PropagateSunlight(World& world, const std::vector<ChunkCoord>& affectedChunks){
    for (const ChunkCoord& coord : affectedChunks) {
        if (!world.chunks.count(coord)) continue;
        Chunk& chunk = world.chunks.at(coord);
        for (int x = 0; x < CHUNK_SIZE; x++){
            for (int z = 0; z < CHUNK_SIZE; z++){
                bool inSunlight = true;
                for (int y = CHUNK_HEIGHT - 1; y >= 0; y--){
                    const BlockDefinition& def = GetBlockDef(chunk.blocks[x][y][z]);
                    // Non-opaque lets skylight continue; lightOpacity (leaves) breaks the
                    // full-15 column so BFS spreads with extra attenuation below.
                    if (!def.opaque) {
                        chunk.sunLight[x][y][z] = inSunlight ? 15 : 0;
                        if (inSunlight && def.lightOpacity > 0) {
                            inSunlight = false;
                        }
                    } else {
                        if (inSunlight) {
                            chunk.sunLight[x][y][z] = 15;
                            inSunlight = false;
                        } else {
                            chunk.sunLight[x][y][z] = 0;
                        }
                    }
                }
            }
        }  
    }
    std::queue<LightNode> queue;
    for (const ChunkCoord& coord : affectedChunks) {
        if (!world.chunks.count(coord)) continue;
        Chunk& chunk = world.chunks.at(coord);
        for (int x = 0; x < CHUNK_SIZE; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    if (!GetBlockDef(chunk.blocks[x][y][z]).opaque && chunk.sunLight[x][y][z] == 15) {
                        BlockId below = (y == 0) ? Block::AIR : chunk.blocks[x][y-1][z];
                        if (y == 0 || GetBlockDef(below).opaque) {
                            queue.push({coord.x * CHUNK_SIZE + x, y, coord.z * CHUNK_SIZE + z,15 });
                            break;
                        }
                    }
                }
    }
    while (!queue.empty()) {
        LightNode node = queue.front();
        queue.pop();

        int dirs[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        for (auto& d : dirs) {
            int nx = node.worldX + d[0];
            int ny = node.worldY + d[1];
            int nz = node.worldZ + d[2];
            if (node.level <= 1) continue;
            if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

            int nChunkX = (int)floor(nx / (float)CHUNK_SIZE);
            int nChunkZ = (int)floor(nz / (float)CHUNK_SIZE);
            ChunkCoord nCoord = {nChunkX, nChunkZ};

            if (!world.chunks.count(nCoord)) continue;
            Chunk& nChunk = world.chunks.at(nCoord);

            int lx = nx - nChunkX * CHUNK_SIZE;
            int lz = nz - nChunkZ * CHUNK_SIZE;

            const BlockDefinition& nDef = GetBlockDef(nChunk.blocks[lx][ny][lz]);
            if (nDef.isLightSource) continue;
            // Skylight only travels through non-opaque cells (air/glass/leaves).
            if (nDef.opaque) continue;

            const uint8_t cost = (uint8_t)(1 + nDef.lightOpacity);
            if (node.level <= cost) continue;
            uint8_t newLevel = (uint8_t)(node.level - cost);
            if (nChunk.sunLight[lx][ny][lz] >= newLevel) continue;

            nChunk.sunLight[lx][ny][lz] = newLevel;
            nChunk.meshDirty = true;
            queue.push({nx, ny, nz, newLevel});
        }
    }

}
static void SeedBlockLightSources(World& world, const std::vector<ChunkCoord>& chunks, std::queue<LightNode>& queue) {
    for (const ChunkCoord& coord : chunks) {
        if (!world.chunks.count(coord)) continue;
        Chunk& chunk = world.chunks.at(coord);

        for (int x = 0; x < CHUNK_SIZE; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    BlockId b = chunk.blocks[x][y][z];
                    const BlockDefinition& def = GetBlockDef(b);
                    if (def.isLightSource) {
                        int worldX = coord.x * CHUNK_SIZE + x;
                        int worldZ = coord.z * CHUNK_SIZE + z;
                        chunk.blockLight[x][y][z] = def.lightLevel;
                        queue.push({worldX, y, worldZ, def.lightLevel});
                    }
                }
    }
}

static void SeedBlockLightFromAdjacentChunks(World& world, int chunkX, int chunkZ, std::queue<LightNode>& queue) {
    static const int neighborOffsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (auto& off : neighborOffsets) {
        ChunkCoord nCoord = {chunkX + off[0], chunkZ + off[1]};
        if (!world.chunks.count(nCoord)) continue;
        Chunk& nChunk = world.chunks.at(nCoord);

        if (off[0] == -1) {
            int worldX = nCoord.x * CHUNK_SIZE + (CHUNK_SIZE - 1);
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    uint8_t level = nChunk.blockLight[CHUNK_SIZE - 1][y][z];
                    if (level == 0) continue;
                    int worldZ = nCoord.z * CHUNK_SIZE + z;
                    if (!GetBlockDef(GetWorldBlock(world, worldX, y, worldZ)).opaque)
                        queue.push({worldX, y, worldZ, level});
                }
        } else if (off[0] == 1) {
            int worldX = nCoord.x * CHUNK_SIZE;
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    uint8_t level = nChunk.blockLight[0][y][z];
                    if (level == 0) continue;
                    int worldZ = nCoord.z * CHUNK_SIZE + z;
                    if (!GetBlockDef(GetWorldBlock(world, worldX, y, worldZ)).opaque)
                        queue.push({worldX, y, worldZ, level});
                }
        } else if (off[1] == -1) {
            int worldZ = nCoord.z * CHUNK_SIZE + (CHUNK_SIZE - 1);
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    uint8_t level = nChunk.blockLight[x][y][CHUNK_SIZE - 1];
                    if (level == 0) continue;
                    int worldX = nCoord.x * CHUNK_SIZE + x;
                    if (!GetBlockDef(GetWorldBlock(world, worldX, y, worldZ)).opaque)
                        queue.push({worldX, y, worldZ, level});
                }
        } else if (off[1] == 1) {
            int worldZ = nCoord.z * CHUNK_SIZE;
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    uint8_t level = nChunk.blockLight[x][y][0];
                    if (level == 0) continue;
                    int worldX = nCoord.x * CHUNK_SIZE + x;
                    if (!GetBlockDef(GetWorldBlock(world, worldX, y, worldZ)).opaque)
                        queue.push({worldX, y, worldZ, level});
                }
        }
    }
}

static void SpreadBlockLightBFS(World& world, std::queue<LightNode>& queue) {
    while (!queue.empty()) {
        LightNode node = queue.front();
        queue.pop();

        int dirs[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
        for (auto& d : dirs) {
            int nx = node.worldX + d[0];
            int ny = node.worldY + d[1];
            int nz = node.worldZ + d[2];

            if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

            int nChunkX = (int)floor(nx / (float)CHUNK_SIZE);
            int nChunkZ = (int)floor(nz / (float)CHUNK_SIZE);
            ChunkCoord nCoord = {nChunkX, nChunkZ};

            if (!world.chunks.count(nCoord)) continue;
            Chunk& nChunk = world.chunks.at(nCoord);

            int lx = nx - nChunkX * CHUNK_SIZE;
            int lz = nz - nChunkZ * CHUNK_SIZE;

            const BlockDefinition& nDef = GetBlockDef(nChunk.blocks[lx][ny][lz]);
            const uint8_t cost = (uint8_t)(1 + nDef.lightOpacity);
            if (node.level <= cost) continue;
            uint8_t newLevel = (uint8_t)(node.level - cost);

            if (nChunk.blockLight[lx][ny][lz] >= newLevel) continue;

            nChunk.blockLight[lx][ny][lz] = newLevel;
            nChunk.meshDirty = true;
            // Propagate through non-opaque (air/glass/leaves), not merely non-colliding.
            if (!nDef.opaque) {
                queue.push({nx, ny, nz, newLevel});
            }
        }
    }
}

void PropagateBlockLight(World& world, const std::vector<ChunkCoord>& affectedChunks) {
    std::queue<LightNode> queue;
    SeedBlockLightSources(world, affectedChunks, queue);
    SpreadBlockLightBFS(world, queue);
}

void PropagateBlockLightOnChunkLoad(World& world, int chunkX, int chunkZ) {
    ChunkCoord loaded = {chunkX, chunkZ};
    if (!world.chunks.count(loaded)) return;

    ClearBlockLight(world, {loaded});

    std::queue<LightNode> queue;
    auto affected = GetAffectedChunks(chunkX, chunkZ);
    SeedBlockLightSources(world, affected, queue);
    SeedBlockLightFromAdjacentChunks(world, chunkX, chunkZ, queue);
    SpreadBlockLightBFS(world, queue);

    world.chunks.at(loaded).meshDirty = true;
}

void ClearBlockLight(World& world, const std::vector<ChunkCoord>& affectedChunks) {
    for (const ChunkCoord& coord : affectedChunks) {
        if (!world.chunks.count(coord)) continue;  // skip unloaded chunks
        Chunk& chunk = world.chunks.at(coord);
        memset(chunk.blockLight, 0, sizeof(chunk.blockLight));
    }
}

std::vector<ChunkCoord> GetAffectedChunks(int chunkX, int chunkZ) {
    std::vector<ChunkCoord> result;
    for (int dx = -1; dx <= 1; dx++)
        for (int dz = -1; dz <= 1; dz++)
            result.push_back({chunkX + dx, chunkZ + dz});
    return result;
}

void PropagateSunlight(World& world, int chunkX, int chunkZ) {
    auto affected = GetAffectedChunks(chunkX, chunkZ);
    PropagateSunlight(world, affected);
}

void PropagateBlockLight(World& world, int chunkX, int chunkZ) {
    auto affected = GetAffectedChunks(chunkX, chunkZ);
    PropagateBlockLight(world, affected);
}
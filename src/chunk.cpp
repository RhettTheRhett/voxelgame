#include "chunk.h"
#include "raylib.h"
#include "noise.h"
#include "world.h"
#include "block.h"
#include "chunkcoord.h"
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

Mesh BuildChunkMesh(const Chunk& chunk, const World& world, int chunkX, int chunkZ){

    // 1. constants and tables (FACE_VERTS, FACE_DIRS)
    const int MAX_FACES = (CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE / 2) * 6;
    const float ATLAS_TILE_SIZE = 1.0f / 16.0f;
    
    //const int MAX_FACES = 16383;

    Mesh mesh = {0};

    mesh.vertexCount = MAX_FACES * 4;
    mesh.triangleCount = MAX_FACES * 2;

    // 2. allocate mesh
    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));;
    mesh.colors = (unsigned char*)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));
    mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    // 3. cursors
    int vertCursor  = 0;
    int indexCursor = 0;
    int colorCursor = 0;
    int textureCursor = 0;
    // 4. fill loop
    for (int x = 0; x < CHUNK_SIZE; x++)
        for (int y = 0; y < CHUNK_HEIGHT; y++)
            for (int z = 0; z < CHUNK_SIZE; z++)
            {
                Block blockType = (Block)chunk.blocks[x][y][z];
                if (blockType == Block::AIR) continue;
                for (int f = 0; f < 6; f++)
                {
                    int nx = x + FACE_DIRS[f][0];
                    int ny = y + FACE_DIRS[f][1];
                    int nz = z + FACE_DIRS[f][2];

                    int worldX = chunkX * CHUNK_SIZE + x + FACE_DIRS[f][0];
                    int worldY = y + FACE_DIRS[f][1];
                    int worldZ = chunkZ * CHUNK_SIZE + z + FACE_DIRS[f][2];

                    // Cull only when the neighbor's collision fully covers this face
                    // (slabs must not hide adjacent full-block sides).
                    if (nx >= 0 && nx < CHUNK_SIZE &&
                        ny >= 0 && ny < CHUNK_HEIGHT &&
                        nz >= 0 && nz < CHUNK_SIZE) {
                        if (OccludesNeighborFace(chunk.blocks[nx][ny][nz], f)) continue;
                    } else {
                        // Cross-chunk / OOB: AIR if unloaded, so the rim keeps its faces.
                        if (OccludesNeighborFace(GetWorldBlock(world, worldX, worldY, worldZ), f)) continue;
                    }
                    const BlockDefinition& def = GetBlockDef(blockType);
                    int baseVertex = vertCursor / 3;
                    for (int v = 0; v < 4; v++)
                    {
                        float lx = FACE_VERTS[f][v*3 + 0];
                        float ly = FACE_VERTS[f][v*3 + 1];
                        float lz = FACE_VERTS[f][v*3 + 2];
                        mesh.vertices[vertCursor++] = x + def.collisionMin.x + lx * (def.collisionMax.x - def.collisionMin.x);
                        mesh.vertices[vertCursor++] = y + def.collisionMin.y + ly * (def.collisionMax.y - def.collisionMin.y);
                        mesh.vertices[vertCursor++] = z + def.collisionMin.z + lz * (def.collisionMax.z - def.collisionMin.z);
                    }
                    //write 6 indices into mesh.indices using indexCursor
                    mesh.indices[indexCursor++] = baseVertex + 0;
                    mesh.indices[indexCursor++] = baseVertex + 3;
                    mesh.indices[indexCursor++] = baseVertex + 2;
                    mesh.indices[indexCursor++] = baseVertex + 0;
                    mesh.indices[indexCursor++] = baseVertex + 2;
                    mesh.indices[indexCursor++] = baseVertex + 1;
                    // write 4 colors into mesh.colors
                    
                    unsigned char shade;
                    
                    
                    //Color faceIndexColor;
                    switch(f) {
                        case 0: shade = 255; break; // +Y top     - brightest
                        case 1: shade = 60;  break; // -Y bottom  - darkest
                        case 2: shade = 180; break; // +X 
                        case 3: shade = 180; break; // -X
                        case 4: shade = 220; break; // +Z
                        case 5: shade = 220; break; // -Z
                        default: shade = 255; break;
                    }

                    const float MIN_LIGHT = 0.15f;

                    // Sample light from adjacent air block (visible face neighbor)
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

                    // Sunlight -> drives RGB, gets scaled by sunBrightness uniform later
                    float sunFactor = MIN_LIGHT + (1.0f - MIN_LIGHT) * (neighborSun / 15.0f);
                    unsigned char sunShade = (unsigned char)(shade * sunFactor);

                    // Block light -> drives alpha, immune to sunBrightness
                    float blockFactor = neighborBlock / 15.0f;
                    unsigned char blockShade = (unsigned char)(255 * blockFactor);

                    for (int v = 0; v < 4; v++)
                    {
                        mesh.colors[colorCursor++] = sunShade;   // R
                        mesh.colors[colorCursor++] = sunShade;   // G
                        mesh.colors[colorCursor++] = sunShade;   // B
                        mesh.colors[colorCursor++] = blockShade; // A -- now carries block light, not transparency
                    }

                    Vector2 tileCoord = GetBlockDef(blockType).FACE_TEX[f];
                    float u0 = tileCoord.x * ATLAS_TILE_SIZE;
                    float v0 = tileCoord.y * ATLAS_TILE_SIZE;
                    float u1 = u0 + ATLAS_TILE_SIZE;
                    float v1 = v0 + ATLAS_TILE_SIZE;
                    switch (f)
                    {
                        case 0: 
                        //(u1,v1), (u0,v1), (u0,v0), (u1,v0)
                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v1;

                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v1;

                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v0;
                            break;
                        case 1:
                        //(u0,v1),(u1,v1), (u1,v0), (u0,v0)
                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v1;

                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v1;

                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v0;
                            break;
                        case 2:
                        //(u0,v0),(u1,v0),(u1,v1),(u0,v1)
                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v1;

                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v1;
                            break;
                        case 3:
                        //(u1,v0),(u0,v0),(u0,v1),(u1,v1)
                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v1;

                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v1;
                            break;
                        case 4:
                        //(u0, v0),(u1, v0),(u1, v1),(u0, v1)
                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v1;

                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v1;
                            break;
                        case 5:
                        //(u1, v0),(u0, v0),(u0, v1),(u1, v1)
                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v0;

                            mesh.texcoords[textureCursor++] = u0;
                            mesh.texcoords[textureCursor++] = v1;

                            mesh.texcoords[textureCursor++] = u1;
                            mesh.texcoords[textureCursor++] = v1;
                            break;
                    }
                }
            }
    // 5. update final counts
    mesh.vertexCount  = vertCursor / 3;
    mesh.triangleCount = indexCursor / 3;
    // 6. upload and return
    UploadMesh(&mesh, false);
    return mesh;
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
                    
                    if(chunk.blocks[x][y][z] == Block::AIR){
                        if(inSunlight){
                            chunk.sunLight[x][y][z] = 15;
                        } else {
                            chunk.sunLight[x][y][z] = 0;
                        }
                    } else {
                        if(inSunlight){
                            chunk.sunLight[x][y][z] = 15;
                            inSunlight = false;
                        }else {chunk.sunLight[x][y][z] = 0;}
                        
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
                    if (chunk.blocks[x][y][z] == (uint16_t)Block::AIR && chunk.sunLight[x][y][z] == 15) {
                        if (y == 0 || chunk.blocks[x][y-1][z] != (uint16_t)Block::AIR) {
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

            uint8_t newLevel = node.level - 1;
            if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

            int nChunkX = (int)floor(nx / (float)CHUNK_SIZE);
            int nChunkZ = (int)floor(nz / (float)CHUNK_SIZE);
            ChunkCoord nCoord = {nChunkX, nChunkZ};

            if (!world.chunks.count(nCoord)) continue;
            Chunk& nChunk = world.chunks.at(nCoord);

            int lx = nx - nChunkX * CHUNK_SIZE;
            int lz = nz - nChunkZ * CHUNK_SIZE;

            if (GetBlockDef(nChunk.blocks[lx][ny][lz]).isLightSource) continue;
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
                    if (!IsSolid(world, worldX, y, worldZ))
                        queue.push({worldX, y, worldZ, level});
                }
        } else if (off[0] == 1) {
            int worldX = nCoord.x * CHUNK_SIZE;
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    uint8_t level = nChunk.blockLight[0][y][z];
                    if (level == 0) continue;
                    int worldZ = nCoord.z * CHUNK_SIZE + z;
                    if (!IsSolid(world, worldX, y, worldZ))
                        queue.push({worldX, y, worldZ, level});
                }
        } else if (off[1] == -1) {
            int worldZ = nCoord.z * CHUNK_SIZE + (CHUNK_SIZE - 1);
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    uint8_t level = nChunk.blockLight[x][y][CHUNK_SIZE - 1];
                    if (level == 0) continue;
                    int worldX = nCoord.x * CHUNK_SIZE + x;
                    if (!IsSolid(world, worldX, y, worldZ))
                        queue.push({worldX, y, worldZ, level});
                }
        } else if (off[1] == 1) {
            int worldZ = nCoord.z * CHUNK_SIZE;
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    uint8_t level = nChunk.blockLight[x][y][0];
                    if (level == 0) continue;
                    int worldX = nCoord.x * CHUNK_SIZE + x;
                    if (!IsSolid(world, worldX, y, worldZ))
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
            uint8_t newLevel = node.level - 1;

            if (newLevel <= 0) continue;
            if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

            int nChunkX = (int)floor(nx / (float)CHUNK_SIZE);
            int nChunkZ = (int)floor(nz / (float)CHUNK_SIZE);
            ChunkCoord nCoord = {nChunkX, nChunkZ};

            if (!world.chunks.count(nCoord)) continue;
            Chunk& nChunk = world.chunks.at(nCoord);

            int lx = nx - nChunkX * CHUNK_SIZE;
            int lz = nz - nChunkZ * CHUNK_SIZE;

            if (nChunk.blockLight[lx][ny][lz] >= newLevel) continue;

            nChunk.blockLight[lx][ny][lz] = newLevel;
            nChunk.meshDirty = true;
            if (!IsSolid(world, nx, ny, nz)) {
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
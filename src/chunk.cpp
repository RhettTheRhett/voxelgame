#include "chunk.h"
#include "raylib.h"
#include "noise.h"
#include "world.h"
#include "block.h"
#include "chunkcoord.h"
#include <queue>

bool IsSolid(const World& world, int worldBlockX, int worldBlockY, int worldBlockZ)
{
    int chunkX = (int)floor(worldBlockX / (float)CHUNK_SIZE);
    int chunkZ = (int)floor(worldBlockZ / (float)CHUNK_SIZE);
    int localX = worldBlockX - chunkX * CHUNK_SIZE;
    int localZ = worldBlockZ - chunkZ * CHUNK_SIZE;

    ChunkCoord coord = { chunkX, chunkZ };
    if (world.chunks.count(coord) == 0) return true; // chunk not loaded
    if (worldBlockY < 0 || worldBlockY >= CHUNK_HEIGHT) return false;
    const Chunk& chunk = world.chunks.at(coord);

    return chunk.blocks[localX][worldBlockY][localZ] != Block::AIR; 
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

                    // fast path — local block, just array indexing
                        if (nx >= 0 && nx < CHUNK_SIZE && 
                            ny >= 0 && ny < CHUNK_HEIGHT && 
                            nz >= 0 && nz < CHUNK_SIZE) {
                            if (chunk.blocks[nx][ny][nz] != Block::AIR) continue;
                        }
                        // slow path — border block, hash map lookup
                        else {
                            int worldX = chunkX * CHUNK_SIZE + nx;
                            int worldZ = chunkZ * CHUNK_SIZE + nz;
                            if (IsSolid(world, worldX, ny, worldZ)) continue;
                        }
                    // emit face f for block at (x, y, z)
                    // write 4 vertices into mesh.vertices using vertCursor
                    int baseVertex = vertCursor / 3; // vertex index of this face's first vert
                    for (int v = 0; v < 4; v++)
                    {
                        mesh.vertices[vertCursor++] = x + FACE_VERTS[f][v*3 + 0];
                        mesh.vertices[vertCursor++] = y + FACE_VERTS[f][v*3 + 1];
                        mesh.vertices[vertCursor++] = z + FACE_VERTS[f][v*3 + 2];
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
                    uint8_t effectiveLight = fmaxf(chunk.sunLight[x][y][z], chunk.blockLight[x][y][z]);
                    float lightFactor = MIN_LIGHT + (1.0f - MIN_LIGHT) * (effectiveLight / 15.0f);
                    unsigned char finalShade = (unsigned char)(shade * lightFactor);

                    for (int v = 0; v < 4; v++)
                    {
                        mesh.colors[colorCursor++] = finalShade;  // R
                        mesh.colors[colorCursor++] = finalShade;  // G
                        mesh.colors[colorCursor++] = finalShade;  // B
                        mesh.colors[colorCursor++] = 255;                 // A
                    }

                    Vector2 tileCoord = BLOCK_DEFINITIONS[blockType].FACE_TEX[f];
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

            if (BLOCK_DEFINITIONS[(Block)nChunk.blocks[lx][ny][lz]].isLightSource) continue;
            if (nChunk.sunLight[lx][ny][lz] >= newLevel) continue;

            nChunk.sunLight[lx][ny][lz] = newLevel;
            nChunk.meshDirty = true;
            queue.push({nx, ny, nz, newLevel});
        }
    }

}
void PropagateBlockLight(World& world, const std::vector<ChunkCoord>& affectedChunks) {
    std::queue<LightNode> queue;

    // seed from every light source in every affected chunk
    for (const ChunkCoord& coord : affectedChunks) {
        if (!world.chunks.count(coord)) continue;
        Chunk& chunk = world.chunks.at(coord);

        for (int x = 0; x < CHUNK_SIZE; x++)
            for (int y = 0; y < CHUNK_HEIGHT; y++)
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    Block b = (Block)chunk.blocks[x][y][z];
                    if (BLOCK_DEFINITIONS[b].isLightSource) {
                        int worldX = coord.x * CHUNK_SIZE + x;
                        int worldZ = coord.z * CHUNK_SIZE + z;
                        chunk.blockLight[x][y][z] = BLOCK_DEFINITIONS[b].lightLevel;
                        queue.push({worldX, y, worldZ, BLOCK_DEFINITIONS[b].lightLevel});
                    }
                }
    }

    // BFS spread 
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

            if (BLOCK_DEFINITIONS[(Block)nChunk.blocks[lx][ny][lz]].isLightSource) continue;
            if (nChunk.blockLight[lx][ny][lz] >= newLevel) continue;

            nChunk.blockLight[lx][ny][lz] = newLevel;
            nChunk.meshDirty = true;
            queue.push({nx, ny, nz, newLevel});
        }
    }
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
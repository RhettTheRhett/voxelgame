#include "chunk.h"
#include "raylib.h"
#include "noise.h"
#include "world.h"

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
    
    //const int MAX_FACES = 16383;

    Mesh mesh = {0};

    mesh.vertexCount = MAX_FACES * 4;
    mesh.triangleCount = MAX_FACES * 2;

    static const float FACE_VERTS[6][12] = {
    // +Y top
    { 1,1,1,  0,1,1,  0,1,0,  1,1,0 },
    // -Y bottom
    { 1,0,0,  0,0,0,  0,0,1,  1,0,1 },
    // +X right
    { 1,1,1,  1,1,0,  1,0,0,  1,0,1 },
    // -X left
    { 0,1,0,  0,1,1,  0,0,1,  0,0,0 },
    // +Z front
    { 0,1,1,  1,1,1,  1,0,1,  0,0,1 },
    // -Z back
    { 1,1,0,  0,1,0,  0,0,0,  1,0,0 },
    };

    static const int FACE_DIRS[6][3] = {
    {  0,  1,  0 },  // +Y
    {  0, -1,  0 },  // -Y
    {  1,  0,  0 },  // +X
    { -1,  0,  0 },  // -X
    {  0,  0,  1 },  // +Z
    {  0,  0, -1 },  // -Z
    };

    // 2. allocate mesh
    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));;
    mesh.colors = (unsigned char*)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));
    // 3. cursors
    int vertCursor  = 0;
    int indexCursor = 0;
    int colorCursor = 0;
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
                    Color faceIndexColor;
                    switch(f) {
                        case 0: shade = 255; faceIndexColor = BLOCK_DEFINITIONS[blockType].TOP_COLOR ; break; // +Y top     - brightest
                        case 1: shade = 60; faceIndexColor = BLOCK_DEFINITIONS[blockType].BOTTOM_COLOR ;  break; // -Y bottom  - darkest
                        case 2: shade = 180; faceIndexColor = BLOCK_DEFINITIONS[blockType].RIGHT_COLOR ; break; // +X 
                        case 3: shade = 180; faceIndexColor = BLOCK_DEFINITIONS[blockType].LEFT_COLOR ; break; // -X
                        case 4: shade = 220; faceIndexColor = BLOCK_DEFINITIONS[blockType].FRONT_COLOR ; break; // +Z
                        case 5: shade = 220; faceIndexColor = BLOCK_DEFINITIONS[blockType].BACK_COLOR ; break; // -Z
                    }
                    for (int v = 0; v < 4; v++)
                    {
                        mesh.colors[colorCursor++] = (unsigned char)(faceIndexColor.r * shade / 255);  // R
                        mesh.colors[colorCursor++] = (unsigned char)(faceIndexColor.g * shade / 255);  // G
                        mesh.colors[colorCursor++] = (unsigned char)(faceIndexColor.b  * shade / 255);  // B
                        mesh.colors[colorCursor++] = (unsigned char)(faceIndexColor.a);                 // A
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

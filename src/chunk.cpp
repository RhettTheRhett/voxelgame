#include "chunk.h"
#include "raylib.h"
#include "noise.h"

bool IsSolid(const Chunk& chunk, int x, int y, int z)
{
    // if out of bounds, treat as air
    if(x < 0 || x >= CHUNK_SIZE) return false;
    if(y < 0 || y >= CHUNK_HEIGHT) return false;
    if(z < 0 || z >= CHUNK_SIZE) return false;
    return chunk.blocks[x][y][z] != 0;
}

Mesh BuildChunkMesh(const Chunk& chunk){

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
                if (chunk.blocks[x][y][z] == 0) continue;
                for (int f = 0; f < 6; f++)
                {
                    int nx = x + FACE_DIRS[f][0];
                    int ny = y + FACE_DIRS[f][1];
                    int nz = z + FACE_DIRS[f][2];

                    if (IsSolid(chunk, nx, ny, nz)) continue;
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
                    switch(f) {
                        case 0: shade = 255; break; // +Y top     - brightest
                        case 1: shade = 60;  break; // -Y bottom  - darkest
                        case 2: shade = 180; break; // +X 
                        case 3: shade = 180; break; // -X
                        case 4: shade = 220; break; // +Z
                        case 5: shade = 220; break; // -Z
                    }

                    for (int v = 0; v < 4; v++)
                    {
                        mesh.colors[colorCursor++] = (unsigned char)(86  * shade / 255);  // R
                        mesh.colors[colorCursor++] = (unsigned char)(125 * shade / 255);  // G
                        mesh.colors[colorCursor++] = (unsigned char)(70  * shade / 255);  // B
                        mesh.colors[colorCursor++] = 255;                                  // A
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

void DrawChunk(const Chunk& chunk){
    for(int x = 0; x < CHUNK_SIZE; x++)
        for(int y = 0; y < CHUNK_HEIGHT; y++)
            for(int z = 0; z < CHUNK_SIZE; z++)
                if(chunk.blocks[x][y][z] == 1){
                    Vector3 worldPos = {
                        chunk.position.x + x + 0.5f,
                        chunk.position.y + y + 0.5f,
                        chunk.position.z + z + 0.5f
                    };
                    bool surrounded = 
                    IsSolid(chunk, x+1,y,z) 
                    && IsSolid(chunk,x-1,y,z) 
                    &&IsSolid(chunk,x,y+1,z) 
                    && IsSolid(chunk,x,y-1,z) 
                    &&IsSolid(chunk,x,y,z+1) 
                    && IsSolid(chunk,x,y,z-1);
                    if(!surrounded){ 
                        DrawCube(worldPos, 1.0f, 1.0f, 1.0f, GREEN);
                        DrawCubeWires(worldPos, 1.0f, 1.0f, 1.0f, BLACK);
                    }
                }
}

void GenerateChunk(Chunk& chunk, int chunkX, int chunkZ, float scale, int octaves, float persistence) {
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            float wx = (chunkX * CHUNK_SIZE + x) * scale ;
            float wz = (chunkZ * CHUNK_SIZE + z) * scale ;

            float n = FBm2D(wx, wz, octaves, persistence);
            float t = (n + 1.0f) / 2.0f;
            int seaLevel  = CHUNK_HEIGHT / 4;      // 16
            int maxHeight = CHUNK_HEIGHT * 3 / 4;  // 48
            int height = seaLevel + (int)(t * (maxHeight - seaLevel));

            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                chunk.blocks[x][y][z] = (y < height) ? 1 : 0;
            }
        }
    }
    chunk.meshDirty = true;
}

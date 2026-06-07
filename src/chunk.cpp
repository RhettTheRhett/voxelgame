#include "chunk.h"
#include "raylib.h"

void DrawChunk(const Chunk& chunk){
    for(int x = 0; x < CHUNK_SIZE; x++)
        for(int y = 0; y < CHUNK_SIZE; y++)
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
                    if(!surrounded) DrawCube(worldPos, 1.0f, 1.0f, 1.0f, GREEN);
                }
}

bool IsSolid(const Chunk& chunk, int x, int y, int z)
{
    // if out of bounds, treat as air
    if(x < 0 || x >= CHUNK_SIZE) return false;
    if(y < 0 || y >= CHUNK_SIZE) return false;
    if(z < 0 || z >= CHUNK_SIZE) return false;
    return chunk.blocks[x][y][z] != 0;
}
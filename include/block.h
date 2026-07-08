#pragma once
#include "raylib.h"
#include <cstdint>

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

enum Block{
        AIR, BEDROCK, GRASS, DIRT, STONE, LIGHT_STONE, WOOD, PLANKS, BRICK, SAND
};


struct BlockDefinition {
        const char* BLOCK_NAME;     
        Vector2 FACE_TEX[6];
        bool isLightSource;
        uint8_t lightLevel;  
};



static BlockDefinition BLOCK_DEFINITIONS[] = {
        {"AIR", {{},{},{},{},{},{}}, false, 0},
        {"BEDROCK",{{15,15},{15,15},{15,15},{15,15},{15,15},{15,15},}, false, 0},
        {"GRASS",  {{0,0}, {1,0}, {0, 1}, {0, 1}, {0, 1}, {0, 1}}, false, 0},
        {"DIRT", {{1,0},{1,0},{1,0},{1,0},{1,0},{1,0}}, false, 0},
        {"STONE", {{2,0}, {2,0}, {2,0}, {2,0}, {2,0}, {2,0}}, false, 0},
        {"LIGHT_STONE", {{1,1}, {1,1},{1,1},{1,1},{1,1},{1,1}}, true, 15},
        {"WOOD", {{0,2}, {0,2}, {1,2},{1,2},{1,2},{1,2}}, false, 0},
        {"PLANKS", {{0,3}, {0,3},{0,3},{0,3},{0,3},{0,3}}, false, 0},
        {"BRICK", {{2,1}, {2,1}, {2,1},{2,1},{2,1},{2,1}}, false, 0},
        {"SAND", {{2,2}, {2,2},{2,2},{2,2},{2,2},{2,2}}, false, 0},
};


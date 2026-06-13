#pragma once
#include "raylib.h"

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
        AIR, GRASS, DIRT, STONE
};

struct BlockDefinition {
        const char* BLOCK_NAME;
        Color TOP_COLOR;
        Color BOTTOM_COLOR;
        Color FRONT_COLOR;
        Color BACK_COLOR;
        Color LEFT_COLOR;
        Color RIGHT_COLOR;
};

//static Color BLOCK_COLORS[] = {{0,0,0,0}, {85, 125, 70, 255}, {131,101,57,255}, {146,142,133,255}};

static BlockDefinition BLOCK_DEFINITIONS[] = {
        {"AIR", {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {"GRASS",  {85, 125, 70, 255}, {131,101,57,255}, {131,101,57,255}, {131,101,57,255}, {131,101,57,255}, {131,101,57,255},},
        {"DIRT", {131,101,57,255}, {131,101,57,255}, {131,101,57,255}, {131,101,57,255}, {131,101,57,255}, {131,101,57,255},},
        {"STONE", {146,142,133,255}, {146,142,133,255}, {146,142,133,255}, {146,142,133,255},{146,142,133,255}, {146,142,133,255}} 
};
#pragma once
#include "raylib.h"

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
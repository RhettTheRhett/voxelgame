#pragma once
#include "block.h"

struct World;

// Minecraft-like levels without block-state metadata:
//   0 = source (full), 1..7 = flowing (decreasing depth).
int  WaterLevel(BlockId id);          // 0..7, or -1 if not water
BlockId WaterFromLevel(int level);    // level 0 → WATER source
bool IsFluid(BlockId id);
bool IsWaterSource(BlockId id);
bool IsReplaceable(BlockId id);       // air or fluid — solids may overwrite
float WaterMeshHeight(BlockId id);    // local Y size for meshing / raycast

// Corner height in local Y for sloped flowing tops (cx/cz = 0 or 1).
float WaterCornerHeight(const World& world, int bx, int by, int bz, int cx, int cz);

// Enqueue this cell + neighbors after a block edit (immediate).
void NotifyWaterChange(World& world, int x, int y, int z);

// Advance water clock and process due flow updates (call each play frame).
void ProcessWaterUpdates(World& world, float deltaTime, int budget);

#pragma once
#include "raylib.h"

struct World;

bool Overlaps(BoundingBox a, BoundingBox b);

// World-space collision AABB for the block at (bx, by, bz), using BlockDefinition shape.
BoundingBox GetBlockCollisionBounds(const World& world, int bx, int by, int bz);

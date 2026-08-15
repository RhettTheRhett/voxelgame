#pragma once
#include "raylib.h"
#include "block.h"

struct World;

bool Overlaps(BoundingBox a, BoundingBox b);
bool OverlapsXZ(BoundingBox a, BoundingBox b);
float Overlap1D(float aMin, float aMax, float bMin, float bMax);

// World-space collision AABB for the block at (bx, by, bz), using BlockDefinition shape.
BoundingBox GetBlockCollisionBounds(const World& world, int bx, int by, int bz);
BoundingBox MakeBlockCollisionBounds(int bx, int by, int bz, BlockId id);

// Ray vs AABB. Returns true on hit within maxDist; outFace is the face entered.
bool RayIntersectAABB(Ray ray, BoundingBox box, float maxDist, float& outDist, int& outFace);

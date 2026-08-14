#include "collision.h"
#include "block.h"
#include "chunk.h"
#include "world.h"
#include <cmath>

bool Overlaps(BoundingBox a, BoundingBox b) {
    return a.min.x < b.max.x && a.max.x > b.min.x &&
           a.min.y < b.max.y && a.max.y > b.min.y &&
           a.min.z < b.max.z && a.max.z > b.min.z;
}

bool OverlapsXZ(BoundingBox a, BoundingBox b) {
    return a.min.x < b.max.x && a.max.x > b.min.x &&
           a.min.z < b.max.z && a.max.z > b.min.z;
}

float Overlap1D(float aMin, float aMax, float bMin, float bMax) {
    return fminf(aMax, bMax) - fmaxf(aMin, bMin);
}

BoundingBox GetBlockCollisionBounds(const World& world, int bx, int by, int bz) {
    BlockId id = GetWorldBlock(world, bx, by, bz);
    const BlockDefinition& def = GetBlockDef(id);

    BoundingBox box{};
    box.min = {
        (float)bx + def.collisionMin.x,
        (float)by + def.collisionMin.y,
        (float)bz + def.collisionMin.z
    };
    box.max = {
        (float)bx + def.collisionMax.x,
        (float)by + def.collisionMax.y,
        (float)bz + def.collisionMax.z
    };
    return box;
}

#include "raycast.h"
#include "world.h"
#include "chunk.h"
#include "block.h"
#include "collision.h"
#include <cfloat>
#include <cmath>

static Face FaceFromMeshIndex(int meshFace) {
    // Mesh / FACE_DIRS indices match RayHit::Face ordinals.
    return static_cast<Face>(meshFace);
}

RayHit RayCast(Ray ray, const World& world, float reachDistance) {
    RayHit rayHit = {};

    int blockX = (int)floorf(ray.position.x);
    int blockY = (int)floorf(ray.position.y);
    int blockZ = (int)floorf(ray.position.z);

    int stepX = (ray.direction.x >= 0) ? 1 : -1;
    int stepY = (ray.direction.y >= 0) ? 1 : -1;
    int stepZ = (ray.direction.z >= 0) ? 1 : -1;

    Vector3 deltaDist;
    if (fabsf(ray.direction.x) < 1e-8f) { deltaDist.x = FLT_MAX; }
    else { deltaDist.x = fabsf(1.0f / ray.direction.x); }
    if (fabsf(ray.direction.y) < 1e-8f) { deltaDist.y = FLT_MAX; }
    else { deltaDist.y = fabsf(1.0f / ray.direction.y); }
    if (fabsf(ray.direction.z) < 1e-8f) { deltaDist.z = FLT_MAX; }
    else { deltaDist.z = fabsf(1.0f / ray.direction.z); }

    Vector3 sideDist;
    if (stepX > 0) { sideDist.x = ((float)blockX + 1.0f - ray.position.x) * deltaDist.x; }
    else { sideDist.x = (ray.position.x - (float)blockX) * deltaDist.x; }
    if (stepY > 0) { sideDist.y = ((float)blockY + 1.0f - ray.position.y) * deltaDist.y; }
    else { sideDist.y = (ray.position.y - (float)blockY) * deltaDist.y; }
    if (stepZ > 0) { sideDist.z = ((float)blockZ + 1.0f - ray.position.z) * deltaDist.z; }
    else { sideDist.z = (ray.position.z - (float)blockZ) * deltaDist.z; }

    // Also test the cell we start in (eyes inside a solid / slab).
    auto tryHitCell = [&](int bx, int by, int bz) -> bool {
        if (!IsRaycastTarget(world, bx, by, bz)) return false;
        BoundingBox box = GetBlockCollisionBounds(world, bx, by, bz);
        float hitDist = 0.0f;
        int meshFace = 0;
        if (!RayIntersectAABB(ray, box, reachDistance, hitDist, meshFace)) return false;
        rayHit.didHit = true;
        rayHit.distance = hitDist;
        rayHit.position = { (float)bx, (float)by, (float)bz };
        rayHit.hitType = HitType::HIT_BLOCK;
        rayHit.faceHit = FaceFromMeshIndex(meshFace);
        return true;
    };

    if (tryHitCell(blockX, blockY, blockZ)) {
        return rayHit;
    }

    while (true) {
        float nextDist;
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            nextDist = sideDist.x;
            sideDist.x += deltaDist.x;
            blockX += stepX;
        } else if (sideDist.y < sideDist.x && sideDist.y < sideDist.z) {
            nextDist = sideDist.y;
            sideDist.y += deltaDist.y;
            blockY += stepY;
        } else {
            nextDist = sideDist.z;
            sideDist.z += deltaDist.z;
            blockZ += stepZ;
        }

        if (nextDist > reachDistance) break;

        if (tryHitCell(blockX, blockY, blockZ)) {
            return rayHit;
        }
    }
    return rayHit;
}

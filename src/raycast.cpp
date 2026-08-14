#include"raycast.h"
#include"world.h"
#include "chunk.h"
#include "block.h"

RayHit RayCast(Ray ray, const World& world, float reachDistance){
    RayHit rayHit = {};

    // Current block position
    int blockX = (int)floor(ray.position.x);
    int blockY = (int)floor(ray.position.y);
    int blockZ = (int)floor(ray.position.z);

    // Step direction (-1 or 1 per axis)
    int stepX = (ray.direction.x >= 0) ? 1 : -1;
    int stepY = (ray.direction.y >= 0) ? 1 : -1;
    int stepZ = (ray.direction.z >= 0) ? 1 : -1;

    // Cost to cross one full cell per axis
    Vector3 deltaDist;
    if (ray.direction.x == 0) { deltaDist.x = FLT_MAX; }
    else { deltaDist.x = fabsf(1.0f / ray.direction.x); }

    if (ray.direction.y == 0) { deltaDist.y = FLT_MAX; }
    else { deltaDist.y = fabsf(1.0f / ray.direction.y); }

    if (ray.direction.z == 0) { deltaDist.z = FLT_MAX; }
    else { deltaDist.z = fabsf(1.0f / ray.direction.z); }

    // Distance to first boundary per axis
    Vector3 sideDist;
    if (stepX > 0) { sideDist.x = (blockX + 1 - ray.position.x) * deltaDist.x; }
    else { sideDist.x = (ray.position.x - blockX) * deltaDist.x; }

    if (stepY > 0) { sideDist.y = (blockY + 1 - ray.position.y) * deltaDist.y; }
    else { sideDist.y = (ray.position.y - blockY) * deltaDist.y; }

    if (stepZ > 0) { sideDist.z = (blockZ + 1 - ray.position.z) * deltaDist.z; }
    else { sideDist.z = (ray.position.z - blockZ) * deltaDist.z; }

    float distance = 0.0f;
    Face lastface{};

    while(true){
        float nextDist;
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            nextDist = sideDist.x;
            sideDist.x += deltaDist.x;
            blockX += stepX;
            lastface = (stepX == 1) ? Face::LEFT_FACE : Face::RIGHT_FACE;
        }
        else if (sideDist.y < sideDist.x && sideDist.y < sideDist.z) {
            nextDist = sideDist.y;
            sideDist.y += deltaDist.y;
            blockY += stepY;
            lastface = (stepY == 1) ? Face::BOTTOM_FACE : Face::TOP_FACE;
        }
        else {
            nextDist = sideDist.z;
            sideDist.z += deltaDist.z;
            blockZ += stepZ;
            lastface = (stepZ == 1) ? Face::BACK_FACE : Face::FRONT_FACE;
        }

        if (nextDist > reachDistance) break;
        distance = nextDist;

        if (IsSolid(world, blockX, blockY, blockZ)){
            rayHit.didHit = true;
            rayHit.distance = distance;
            rayHit.position = {(float)blockX, (float)blockY, (float)blockZ};
            rayHit.hitType = HitType::HIT_BLOCK;
            rayHit.faceHit = lastface;
            return rayHit;
        }
    }
    return rayHit;
}

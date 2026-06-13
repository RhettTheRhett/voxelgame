#include"raycast.h"
#include"world.h"
#include "chunk.h"

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
    Face lastface;

    while(distance < reachDistance){
        if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
            sideDist.x += deltaDist.x;
            blockX += stepX;
            distance = sideDist.x;
            if (stepX == 1){
                lastface = Face::LEFT_FACE;
            } else if (stepX == -1){
                lastface = Face::RIGHT_FACE;
            }
        }
        else if (sideDist.y < sideDist.x && sideDist.y < sideDist.z) {
            sideDist.y += deltaDist.y;
            blockY += stepY;
            distance = sideDist.y;
            if (stepY == 1){
                lastface = Face::BOTTOM_FACE;
            } else if (stepY == -1){
                lastface = Face::TOP_FACE;
            }
        }
        else {
            sideDist.z += deltaDist.z;
            blockZ += stepZ;
            distance = sideDist.z;
            if (stepZ == 1){
                lastface = Face::BACK_FACE;
            } else if (stepZ == -1){
                lastface = Face::FRONT_FACE;
            }
        }

        if (IsSolid(world, blockX, blockY, blockZ)){
            rayHit.didHit = true;
            rayHit.distance = distance;
            rayHit.position = {(float)blockX, (float)blockY, (float)blockZ};
            rayHit.hitType = HitType::Block;
            rayHit.faceHit = lastface;
            return rayHit;
        }
    }
    return rayHit;
}
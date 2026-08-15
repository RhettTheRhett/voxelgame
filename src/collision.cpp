#include "collision.h"
#include "block.h"
#include "chunk.h"
#include "world.h"
#include <cmath>
#include <cfloat>

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

BoundingBox MakeBlockCollisionBounds(int bx, int by, int bz, BlockId id) {
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

BoundingBox GetBlockCollisionBounds(const World& world, int bx, int by, int bz) {
    return MakeBlockCollisionBounds(bx, by, bz, GetWorldBlock(world, bx, by, bz));
}

bool RayIntersectAABB(Ray ray, BoundingBox box, float maxDist, float& outDist, int& outFace) {
    // Face indices match FACE_DIRS: 0=+Y 1=-Y 2=+X 3=-X 4=+Z 5=-Z
    const float origins[3] = { ray.position.x, ray.position.y, ray.position.z };
    const float dirs[3]    = { ray.direction.x, ray.direction.y, ray.direction.z };
    const float bmin[3]    = { box.min.x, box.min.y, box.min.z };
    const float bmax[3]    = { box.max.x, box.max.y, box.max.z };
    const int faceMin[3] = { 3, 1, 5 }; // -X, -Y, -Z
    const int faceMax[3] = { 2, 0, 4 }; // +X, +Y, +Z

    const bool inside =
        origins[0] >= bmin[0] && origins[0] <= bmax[0] &&
        origins[1] >= bmin[1] && origins[1] <= bmax[1] &&
        origins[2] >= bmin[2] && origins[2] <= bmax[2];

    if (inside) {
        // Looking out from inside: pick the face the ray exits through.
        float tExit = maxDist;
        int exitFace = 0;
        for (int i = 0; i < 3; i++) {
            if (fabsf(dirs[i]) < 1e-8f) continue;
            float target = (dirs[i] > 0.0f) ? bmax[i] : bmin[i];
            float t = (target - origins[i]) / dirs[i];
            if (t >= 0.0f && t < tExit) {
                tExit = t;
                exitFace = (dirs[i] > 0.0f) ? faceMax[i] : faceMin[i];
            }
        }
        outDist = 0.0f;
        outFace = exitFace;
        return true;
    }

    float tmin = 0.0f;
    float tmax = maxDist;
    int hitFace = 0;

    for (int i = 0; i < 3; i++) {
        if (fabsf(dirs[i]) < 1e-8f) {
            if (origins[i] < bmin[i] || origins[i] > bmax[i]) return false;
            continue;
        }
        float inv = 1.0f / dirs[i];
        float t0 = (bmin[i] - origins[i]) * inv;
        float t1 = (bmax[i] - origins[i]) * inv;
        int f0 = faceMin[i];
        int f1 = faceMax[i];
        if (t0 > t1) {
            float tmp = t0; t0 = t1; t1 = tmp;
            int ft = f0; f0 = f1; f1 = ft;
        }
        if (t0 > tmin) {
            tmin = t0;
            hitFace = f0;
        }
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return false;
    }

    if (tmin < 0.0f || tmin > maxDist) return false;
    outDist = tmin;
    outFace = hitFace;
    return true;
}

#pragma once
#include "raylib.h"
#include "world.h"

enum HitType{
    Block, Entity, Player
};

enum Face{
    TOP_FACE,
    BOTTOM_FACE,
    RIGHT_FACE,
    LEFT_FACE,
    FRONT_FACE,
    BACK_FACE
};

struct RayHit{
    HitType hitType;
    Face faceHit;
    Vector3 position;  
    float distance;
    bool didHit;
};

RayHit RayCast(Ray ray, const World& world, float reachDistance);

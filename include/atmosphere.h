#pragma once
#include "raylib.h"
#include "raymath.h"
#include "constants.h"



float CalculateSunBrightness(float timeOfDay);
Color CalculateSkyColor(float timeOfDay);
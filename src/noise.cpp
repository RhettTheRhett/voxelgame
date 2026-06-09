#include "noise.h"
#include <cmath>

static float hash2D(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

static float hash3D(int x, int y, int z)
{
    int n = x + y * 57 + z * 131;
    n = (n << 13) ^ n;

    return 1.0f -
           ((n * (n * n * 15731 + 789221) + 1376312589)
            & 0x7fffffff)
           / 1073741824.0f;
}

// Smooth interpolation — smoother than linear, avoids visible grid lines
// Look up "smoothstep" — it's a cubic: 3t² - 2t³
static float smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

static float g_offsetX = 0.0f;
static float g_offsetZ = 0.0f;
static float g_offsetY = 0.0f;

void SetNoiseSeed(int seed) {
    // use the hash to derive offsets from the seed
    // multiply by a large number so small seeds still jump far
    g_offsetX = hash2D(seed, 0) * 10000.0f;
    g_offsetZ = hash2D(0, seed) * 10000.0f;
    g_offsetY = hash3D(seed,seed,seed) * 10000.0f;
}

// Interpolate between a and b using factor t (0..1)
static float lerp(float a, float b, float t) {
    return a + (b-a) * t;
}

float ValueNoise2D(float x, float y) {
    // 1. Find the integer grid cell containing (x, y)
    int leftBoundary = (int)floor(x);
    int bottomBoundary = (int)floor(y);
    int rightBoundary = leftBoundary + 1;
    int topBoundary = bottomBoundary + 1;

    // 2. Get fractional offset within that cell
    float offsetX = x - leftBoundary;
    float offsetY = y - bottomBoundary;

    // 3. Hash all 4 corners of the cell
    float bottomLeft = hash2D(leftBoundary, bottomBoundary);
    float bottomRight = hash2D(rightBoundary, bottomBoundary);
    float topLeft = hash2D(leftBoundary, topBoundary);
    float topRight = hash2D(rightBoundary, topBoundary);

    // 4. Bilinear interpolate using smoothstep-smoothed fractions
    float sx = smoothstep(offsetX);
    float sy = smoothstep(offsetY);

    float bottom = lerp(bottomLeft, bottomRight, sx);
    float top    = lerp(topLeft, topRight, sx);
    return lerp(bottom, top, sy);
}

float ValueNoise3D(float x, float y, float z)
{
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int z0 = (int)floor(z);

    int x1 = x0 + 1;
    int y1 = y0 + 1;
    int z1 = z0 + 1;

    float tx = x - x0;
    float ty = y - y0;
    float tz = z - z0;

    float c000 = hash3D(x0, y0, z0);
    float c100 = hash3D(x1, y0, z0);
    float c010 = hash3D(x0, y1, z0);
    float c110 = hash3D(x1, y1, z0);

    float c001 = hash3D(x0, y0, z1);
    float c101 = hash3D(x1, y0, z1);
    float c011 = hash3D(x0, y1, z1);
    float c111 = hash3D(x1, y1, z1);

    float sx = smoothstep(tx);
    float sy = smoothstep(ty);
    float sz = smoothstep(tz);

    float x00 = lerp(c000, c100, sx);
    float x10 = lerp(c010, c110, sx);
    float x01 = lerp(c001, c101, sx);
    float x11 = lerp(c011, c111, sx);

    float y00 = lerp(x00, x10, sy);
    float y11 = lerp(x01, x11, sy);

    return lerp(y00, y11, sz);
}

//fractional brownian motion 2d
float FBm2D(float x, float y, int octaves, float persistence) {
    float value    = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue  = 0.0f; // tracks max possible for normalization

    for (int i = 0; i < octaves; i++) {
        // sample noise at current frequency, scale by amplitude
        value += ValueNoise2D((x + g_offsetX) * frequency, (y + g_offsetZ) * frequency) * amplitude;
        // accumulate amplitude into maxValue
        maxValue += amplitude;
        // double frequency, multiply amplitude by persistence
        frequency *= 2.0f;
        amplitude *= persistence;
    }

    return value / maxValue; // normalize to [-1, 1]
}

float FBm3D(float x,float y, float z, int octaves, float persistence)
{
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++)
    {
        value += ValueNoise3D(
            (x + g_offsetX) * frequency,
            (y + g_offsetY) * frequency,
            (z + g_offsetZ) * frequency
        ) * amplitude;

        maxValue += amplitude;

        frequency *= 2.0f;
        amplitude *= persistence;
    }

    return (value / maxValue + 1.0f) * 0.5f;  // remap [-1,1] to [0,1]
}


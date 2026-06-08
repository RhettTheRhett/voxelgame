#pragma once

// Single-sample 2D value noise. Coords are float world-space.
// Returns value roughly in [-1, 1].
float ValueNoise2D(float x, float y);
float ValueNoise3D(float x, float y, float z);

// Layered octave noise (fBm). octaves=4, persistence=0.5 is a good start.
float FBm2D(float x, float y, int octaves, float persistence);
float FBm3D(float x,float y, float z, int octaves, float persistence);

void SetNoiseSeed(int seed);
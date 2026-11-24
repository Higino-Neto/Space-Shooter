#ifndef NOISE_H
#define NOISE_H

#include "raylib.h"

float Noise3D(Vector3 position);

float FBM(
    Vector3 position,
    int octaves,
    float lacunarity,
    float gain
);

float GetTerrainHeight(
    Vector3 direction,
    float seed,
    float amplitude
);

#endif
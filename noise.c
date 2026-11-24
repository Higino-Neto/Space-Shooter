#include "noise.h"
#include "raymath.h"
#include <math.h>

static float Hash3D(
    int x,
    int y,
    int z)
{
    unsigned int h =
        (unsigned int)x * 374761393u +
        (unsigned int)y * 668265263u +
        (unsigned int)z * 2147483647u;

    h =
        (h ^ (h >> 13)) *
        1274126177u;

    h ^= h >> 16;

    return (float)(h & 0x7fffffff) /
               1073741823.5f * 2.0f -
           1.0f;
}

static float NoiseFade(float t)
{
    return t * t * t *
           (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float NoiseLerp(
    float a,
    float b,
    float t)
{
    return a + (b - a) * t;
}

float Noise3D(Vector3 position)
{
    int x0 = (int)floorf(position.x);
    int y0 = (int)floorf(position.y);
    int z0 = (int)floorf(position.z);

    int x1 = x0 + 1;
    int y1 = y0 + 1;
    int z1 = z0 + 1;

    float tx =
        position.x - (float)x0;

    float ty =
        position.y - (float)y0;

    float tz =
        position.z - (float)z0;

    float sx = NoiseFade(tx);
    float sy = NoiseFade(ty);
    float sz = NoiseFade(tz);

    float n000 =
        Hash3D(x0, y0, z0);

    float n100 =
        Hash3D(x1, y0, z0);

    float n010 =
        Hash3D(x0, y1, z0);

    float n110 =
        Hash3D(x1, y1, z0);

    float n001 =
        Hash3D(x0, y0, z1);

    float n101 =
        Hash3D(x1, y0, z1);

    float n011 =
        Hash3D(x0, y1, z1);

    float n111 =
        Hash3D(x1, y1, z1);

    float nx00 =
        NoiseLerp(n000, n100, sx);

    float nx10 =
        NoiseLerp(n010, n110, sx);

    float nx01 =
        NoiseLerp(n001, n101, sx);

    float nx11 =
        NoiseLerp(n011, n111, sx);

    float nxy0 =
        NoiseLerp(nx00, nx10, sy);

    float nxy1 =
        NoiseLerp(nx01, nx11, sy);

    return NoiseLerp(
        nxy0,
        nxy1,
        sz);
}

float FBM(
    Vector3 position,
    int octaves,
    float lacunarity,
    float gain)
{
    float value = 0.0f;

    float amplitude = 2.0f;

    float frequency = 1.0f;

    float amplitudeSum = 0.0f;

    for (int i = 0; i < octaves; i++)
    {

        Vector3 sample =
            Vector3Scale(
                position,
                frequency);

        value +=
            Noise3D(sample) *
            amplitude;

        amplitudeSum += amplitude;

        frequency *= lacunarity;
        amplitude *= gain;
    }

    if (amplitudeSum == 0.0f)
    {
        return 0.0f;
    }

    return value / amplitudeSum;
}

float GetTerrainHeight(
    Vector3 direction,
    float seed,
    float amplitude)
{
    Vector3 sample =
        Vector3Scale(
            direction,
            2.0f);

    sample.x += seed;
    sample.y += seed * 1.37f;
    sample.z += seed * 2.71f;

    float value =
        FBM(
            sample,
            6,
            2.0f,
            0.5f);

    return value * amplitude;
}
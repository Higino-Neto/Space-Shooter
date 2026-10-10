#ifndef PLANET_H
#define PLANET_H

#include "raylib.h"

#define MAX_DEFORMATIONS 200
#define MIN_PLANET_RADIUS_RATIO 0.1f


typedef struct PlanetPatch {
    Mesh mesh;
    float *originalVertices;

    int face;

    float uMin;
    float uMax;

    float vMin;
    float vMax;
} PlanetPatch;

typedef struct TerrainDeformation {
    Vector3 direction;
    float radius;
    float strength;
} TerrainDeformation;

typedef struct Planet {
    float seed;
    float radius;
    float heightAmplitude;
    Material material;

    TerrainDeformation deformations[MAX_DEFORMATIONS];
    int deformationCount;

    int appliedDeformationCount;

    Vector3 position;

    PlanetPatch faces[6];

    int resolution;
} Planet;

void UpdatePlanet(
    Planet *planet,
    Vector3 cameraPosition
);

void DrawPlanet(
    Planet *planet,
    Material material
);


Vector3 CubePointUV(
    int face,
    float u,
    float v
);

Planet CreatePlanet(
    float radius,
    int resolution,
    float seed,
    float heightAmplitude,
    Material material
);


PlanetPatch CreatePlanetPatch(
    int face,
    float uMin,
    float uMax,
    float vMin,
    float vMax,
    int resolution,
    float radius,
    float seed,
    float heightAmplitude
);

void UpdatePlanetPatch(Planet *planet);

bool PlanetContainsSphere(
    const Planet *planet,
    Vector3 point,
    float objectRadius
);

void DamagePlanet(Planet *planet, Vector3 impactPoint, float damage);

void RecalculatePlanetPatchNormals(PlanetPatch *patch);

#endif
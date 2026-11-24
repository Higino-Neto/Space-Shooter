#ifndef TRAIL_H
#define TRAIL_H

#include "raylib.h"

#define TRAIL_MAX_POINTS 64

typedef struct {
    Vector3 position;
    float age;
} TrailPoint;

typedef struct {
    TrailPoint points[TRAIL_MAX_POINTS];
    int count;
    float sampleTimer;
} Trail;

typedef struct {
    float lifetime;
    float sampleInterval;

    float exhaustDistance;
    float exhaustSeparation;

    float width;
    float coreWidth;

    Color color;
    Color coreColor;

    float followSpeed;
} TrailConfig;

void UpdateTrail(
    Trail *trail,
    Vector3 position,
    float deltaTime,
    const TrailConfig *config
);

void DrawTrail(
    const Trail *trail,
    const TrailConfig *config,
    Vector3 up,
    Camera3D camera
);

void BeginTrailBatch(Camera3D camera);

void DrawTrailInBatch(
    const Trail *trail,
    const TrailConfig *config,
    Vector3 up
);

void DrawTrailGlow(
    Vector3 position,
    float radius,
    Color color
);

void EndTrailBatch(void);

#endif
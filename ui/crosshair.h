#ifndef CROSSHAIR_H
#define CROSSHAIR_H

#include "raylib.h"
#include "../camera.h"

typedef struct Crosshair {
    Vector2 aim;
    Vector3 aimDirection;
    float yawOffset;
    float pitchOffset;
} Crosshair;

void CrosshairInit(void);
void CrosshairShutdown(void);

Crosshair CreateCrosshair(void);

void CrosshairUpdateInput(Crosshair *crosshair);
void CrosshairSteerCamera(Crosshair *crosshair, CameraController *controller, float deltaTime);
void CrosshairProject(Crosshair *crosshair, const Camera3D *camera);

void CrosshairDraw(const Crosshair *crosshair);

#endif
#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "raylib.h"
#include "trail.h"

typedef struct {
    Vector3 position;
    Vector3 direction;

    float damage;
    float speed;
    float lifetime;

    int ownerId;
    bool active;

    Trail trail;
    TrailConfig trailConfig;

} Projectile;

Projectile CreateProjectile(
    Vector3 position,
    Vector3 direction,
    float damage,
    float speed,
    int ownerId
);

void UpdateProjectile(
    Projectile *projectile,
    float deltaTime
);

void DrawProjectile(
    Projectile *projectile,
    Camera3D camera
);

#endif
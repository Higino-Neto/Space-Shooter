#include "projectile.h"
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>

Projectile CreateProjectile(Vector3 position, Vector3 direction, float damage, float speed, int ownerId) {
    Projectile projectile = { 0 };

    projectile.position = position;
    projectile.direction = direction;
    projectile.damage = damage;
    projectile.speed = speed;
    projectile.ownerId = ownerId;

    projectile.active = true;
    projectile.lifetime = 10.0f;

    projectile.trailConfig = (TrailConfig) {
        .lifetime = 0.42f,
        .sampleInterval = 0.008f,

        .width = 2.40f,
        .coreWidth = 1.10f,

        .color = RED,
        .coreColor = RED
    };

    return projectile;
}

void UpdateProjectile(Projectile *projectile, float deltaTime) {

    if (projectile == NULL) {
        return;
    }
    
    if (projectile->active) {
         projectile->position =
            Vector3Add(
                projectile->position,
                Vector3Scale(
                    projectile->direction,
                    projectile->speed * deltaTime
                )
            );

        projectile->lifetime -= deltaTime;

        if (projectile->lifetime <= 0.0f)
        {
            projectile->active = false;
        }
    }
    
    UpdateTrail(
        &projectile->trail,
        projectile->position,
        deltaTime,
        &projectile->trailConfig
    );

}


void DrawProjectile(Projectile *projectile, Camera3D camera) {
    if (projectile == NULL)
        return;

    Vector3 up = {
        0.0f,
        1.0f,
        0.0f
    };

    DrawTrail(
        &projectile->trail,
        &projectile->trailConfig,
        up,
        camera
    );

    if (projectile->active) {
        DrawSphere(projectile->position, Vector3Length(projectile->direction) * 3, RED);
    }
}

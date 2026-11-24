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
    projectile.lifetime = 5.0f;

    projectile.trailConfig = (TrailConfig) {
        .lifetime = 0.30f,
        .sampleInterval = 0.012f,

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


void DrawProjectileTrail(Projectile *projectile) {
    if (projectile == NULL)
        return;

    Vector3 up = {
        0.0f,
        1.0f,
        0.0f
    };

    DrawTrailInBatch(
        &projectile->trail,
        &projectile->trailConfig,
        up
    );
}

void DrawProjectile(Projectile *projectile) {
    if (projectile == NULL || !projectile->active)
        return;

    DrawTrailGlow(
        projectile->position,
        Vector3Length(projectile->direction) * 3.0f,
        RED
    );
}

#ifndef TARGETING_H
#define TARGETING_H

#include "raylib.h"
#include "../camera.h"
#include "../enemy.h"
#include "crosshair.h"

#define MAX_TARGET_MARKERS 128
#define MAX_OFFSCREEN_MARKERS 8

typedef struct {
    int enemyIndex;
    Vector2 screenPosition;
    float boxSize;
    float distance;
    int health;
    int maxHealth;
    int shield;
    int maxShield;
    float damage;
    EnemyState state;
    float presence;
    bool selected;
} TargetMarker;

typedef struct {
    Vector2 screenPosition;
    Vector2 direction;
    float distance;
    float presence;
    int count;
    EnemyState state;
} OffscreenMarker;

typedef struct {
    TargetMarker visible[MAX_TARGET_MARKERS];
    int visibleCount;

    OffscreenMarker offscreen[MAX_OFFSCREEN_MARKERS];
    int offscreenCount;

    float presence[MAX_TARGET_MARKERS];

    bool hasSelection;
    int selectedIndex;
    Vector3 selectedPosition;
    float selectionTimer;
} TargetingSystem;

TargetingSystem CreateTargetingSystem(void);

void UpdateTargeting(
    TargetingSystem *system,
    const Enemy *enemies,
    int enemyCount,
    Vector3 shipPosition,
    const Camera3D *camera,
    CameraController *controller,
    Crosshair *crosshair,
    float deltaTime
);

bool TargetingResolveShot(
    const TargetingSystem *system,
    const Enemy *enemies,
    int enemyCount,
    Vector3 muzzle,
    float projectileSpeed,
    Vector3 *direction
);

void DrawTargeting(const TargetingSystem *system);

#endif

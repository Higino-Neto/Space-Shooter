#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "player.h"
#include "planet.h"

typedef enum {
    ENEMY_PATROL,
    ENEMY_ATTACK_PLANET,
    ENEMY_COMBAT,
    ENEMY_RETREAT
} EnemyState;

typedef struct Enemy {

    Vector3 position;
    Vector3 forward;

    Vector3 velocity;
    Vector3 acceleration;

    float maxSpeed;
    float maxAcceleration;
    float maxTurnRate;

    float strength;
    float collisionRadius;

    float shootCooldown;
    float shootTimer;

    int health;
    int maxHealth;

    bool wantsToShoot;
    EnemyState state;

    Vector3 planetTarget;

    float detectionRadius;
    float attackDistance;
    float retreatHealth;

    bool active;
    bool alive;

} Enemy;

static void UpdateOrientation(
    Enemy *enemy,
    Vector3 desiredForward,
    float deltaTime
);

static Vector3 GetCombatOrbitDirection(
    Vector3 targetDirection,
    Vector3 referenceUp
);

static Vector3 ChoosePlanetTarget(
    const Planet *planet,
    Vector3 enemyPosition
);

static bool CanEnemyShootPlanet(
    const Enemy *enemy
);

static bool CanEnemyShoot(
    const Enemy *enemy,
    const Player *player
);

Enemy CreateEnemy(
    Vector3 position,
    const Planet *planet
);

void UpdateEnemy(
    Enemy *enemy,
    const Planet *planet,
    const Player *player,
    float deltaTime
);

void DrawEnemy(
    const Enemy *enemy
);

void DamageEnemy(
    Enemy *enemy,
    float damage
);

#endif
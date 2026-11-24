#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "player.h"
#include "planet.h"

typedef enum {
    ENEMY_INTERCEPTOR,
    ENEMY_SHIELD,
    ENEMY_BOMBER,
    ENEMY_RAMMER,
    ENEMY_FIGHTER,
    ENEMY_SUPPORT,
    ENEMY_SPLITTER,
    ENEMY_PHANTOM,
    ENEMY_SNIPER,
    ENEMY_DREADNOUGHT,
    ENEMY_TYPE_COUNT
} EnemyType;

typedef struct EnemyAssets {
    Model ships[ENEMY_TYPE_COUNT];
    Texture2D texture;
} EnemyAssets;

EnemyAssets LoadEnemyAssets(void);
void UnloadEnemyAssets(EnemyAssets *assets);

typedef enum {
    ENEMY_PATROL,
    ENEMY_ATTACK_PLANET,
    ENEMY_COMBAT,
    ENEMY_RETREAT
} EnemyState;

typedef struct Enemy {

    const Model *visualModel;
    float modelScale;
    Color tint;

    EnemyType type;

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

    int projectileCount;
    float projectileSpeed;
    float burstSpread;

    int health;
    int maxHealth;

    int shield;
    int maxShield;
    float shieldRechargeDelay;
    float shieldRechargeTimer;
    float shieldRechargeRate;
    float shieldRechargeAccumulator;

    int xpReward;

    bool wantsToShoot;
    EnemyState state;

    Vector3 planetTarget;

    float detectionRadius;
    float attackDistance;
    float retreatHealth;

    bool splitOnDeath;
    int splitCount;

    bool cloaked;
    float cloakTimer;
    float cloakInterval;
    float cloakDuration;

    float healCooldown;
    float healTimer;
    float healRadius;
    int healAmount;
    bool wantsToHeal;
    float healPulse;

    float contactDamage;
    float contactCooldown;
    float contactTimer;

    float spawnDistance;

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

Enemy CreateEnemyType(
    EnemyType type,
    Vector3 position,
    const Planet *planet
);

void ScaleEnemyForWave(
    Enemy *enemy,
    int waveNumber
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
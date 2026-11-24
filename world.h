#ifndef WORLD_H
#define WORLD_H

#include "planet.h"
#include "player.h"
#include "projectile.h"
#include "camera.h"
#include "enemy.h"
#include "wave.h"
#include "camera.h"
#include "ui/crosshair.h"
#include "ui/targeting.h"

#define MAX_PLANETS 10
#define MAX_ENEMIES 100
#define MAX_PROJECTILES 1000
#define MAX_ENEMY_PROJECTILES 350
#define PROJECTILE_DESPAWN_DISTANCE 9000.0f
#define SPHERE_MIN_DISTANCE_FROM_PLANET 20.0f


typedef struct {
    Player player;
    
    Planet planets[MAX_PLANETS];
    int planetCount;

    Enemy enemies[MAX_ENEMIES];
    int enemyCount;

    Projectile projectiles[MAX_PROJECTILES];
    int projectileCount;

    int currentWave;
    float waveCooldown;
    float waveBannerTimer;
    bool waitingForNextWave;

    EnemyAssets *enemyAssets;

    GameCamera gameCamera;
    TargetingSystem targeting;
} World;

static float DistanceSquared(Vector3 a, Vector3 b);

// static float SafeLength(Vector3 vector);
// static void ResolvePlanetCollision(Player *sphere, const Planet *planet);


static void RemoveInactiveEnemies(World *world);
static void RemoveInactiveProjectiles(World *world);
static void UpdateWorldPlayer(
    World *world,
    CameraController *controller,
    float deltaTime
);
static void HandlePlayerShooting(
    World *world,
    const Crosshair *crosshair,
    const Camera *camera
);

static void HandleEnemyShooting(
    World *world
);

static void HandleEnemySupport(
    World *world
);

static void HandleEnemyContactDamage(
    World *world
);

static void SpawnSplitChildren(
    World *world,
    const Enemy *parent
);

static bool ProjectileHitsPlayer(
    World *world,
    Projectile *projectile
);

static void UpdateWorldEnemies(
    World *world,
    float deltaTime
);

static bool ProjectileHitsPlanet(
    World *world,
    Projectile *projectile
);

static bool ProjectileHitsEnemy(
    World *world,
    Projectile *projectile
);

static void UpdateWorldProjectiles(
    World *world,
    float deltaTime
);

static void CleanupWorld(
    World *world
);

World CreateWorld();

static void UpdateWorldWave(
    World *world,
    EnemyAssets *enemyAssets,
    float deltaTime
);

bool WorldAddPlayer(World *world, Player player);

bool WorldAddPlanet(World *world, Planet planet);

bool WorldAddEnemy(World *world, Enemy enemy);

bool WorldAddWave(World *world, EnemyWave wave);

bool WorldAddProjectile(World *world, Projectile projectile);

void UpdateWorld(
    World *world,
    CameraController *controller,
    Camera *camera,
    Crosshair *crosshair,
    EnemyAssets *enemyAssets
);

void DrawWorld(World *world, Camera3D camera);

#endif
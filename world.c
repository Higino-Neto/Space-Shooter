#include "world.h"
#include "camera.h"
#include "player.h"
#include "planet.h"
#include "wave.h"

#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <stdio.h>

static float DistanceSquared(Vector3 a, Vector3 b)
{
    Vector3 difference = Vector3Subtract(a, b);

    return
        difference.x * difference.x +
        difference.y * difference.y +
        difference.z * difference.z;
}


// static float SafeLength(Vector3 vector)
// {
//     return sqrtf(
//         vector.x * vector.x +
//         vector.y * vector.y +
//         vector.z * vector.z
//     );
// }

// static void ResolvePlanetCollision(Player *Sphere, const Planet *planet) {
//     Vector3 offset =
//         Vector3Subtract(
//             Sphere->position,
//             planet->position
//         );

//     float distance =
//         SafeLength(offset);

//     if (distance < 0.001f)
//     {
//         Sphere->position =
//             Vector3Add(
//                 planet->position,
//                 (Vector3){
//                     0.0f,
//                     planet->radius +
//                     SPHERE_MIN_DISTANCE_FROM_PLANET,
//                     0.0f
//                 }
//             );

//         Sphere->velocity =
//             Vector3Zero();

//         return;
//     }

//     float minimumDistance =
//         planet->radius +
//         Sphere->collisionRadius +
//         SPHERE_MIN_DISTANCE_FROM_PLANET;


//     if (distance < minimumDistance)
//     {
//         Vector3 normal =
//             Vector3Scale(
//                 offset,
//                 1.0f / distance
//             );

//         Sphere->position =
//             Vector3Add(
//                 planet->position,
//                 Vector3Scale(
//                     normal,
//                     minimumDistance
//                 )
//             );


//         float radialVelocity =
//             Vector3DotProduct(
//                 Sphere->velocity,
//                 normal
//             );

//         if (radialVelocity < 0.0f)
//         {
//             Sphere->velocity =
//                 Vector3Subtract(
//                     Sphere->velocity,
//                     Vector3Scale(
//                         normal,
//                         radialVelocity
//                     )
//                 );
//         }
//     }
// }






static void RemoveInactiveEnemies(World *world)
{
    int writeIndex = 0;

    for (int readIndex = 0;
         readIndex < world->enemyCount;
         readIndex++)
    {
        if (world->enemies[readIndex].active)
        {
            if (writeIndex != readIndex)
            {
                world->enemies[writeIndex] =
                    world->enemies[readIndex];
            }

            writeIndex++;
        }
    }

    world->enemyCount = writeIndex;
}


static void RemoveInactiveProjectiles(World *world)
{
    int writeIndex = 0;

    for (int readIndex = 0;
         readIndex < world->projectileCount;
         readIndex++)
    {
        if (world->projectiles[readIndex].active)
        {
            if (writeIndex != readIndex)
            {
                world->projectiles[writeIndex] =
                    world->projectiles[readIndex];
            }

            writeIndex++;
        }
    }

    world->projectileCount = writeIndex;
}



World CreateWorld() {
    World world = { 0 };

    world.gameCamera =
        CreateGameCamera();

    return world;
}

bool WorldAddPlayer(World *world, Player player) {
    world->player = player;

    return true;
}

bool WorldAddEnemy(World *world, Enemy enemy) {
    if (world->enemyCount >= MAX_ENEMIES) {
        return false;
    }

    world->enemies[world->enemyCount++] = enemy;

    return true;
}

bool WorldAddWave(World *world, EnemyWave enemyWave) {
    for(int groupIndex = 0; groupIndex < enemyWave.enemyGroupCount; groupIndex++) {
        EnemyGroup *group = &enemyWave.enemyGroup[groupIndex];

        Enemy enemy = group->model;
        for(int enemyIndex = 0; enemyIndex < group->enemyCount; enemyIndex++) {
            WorldAddEnemy(world, enemy);
        }
   }
}

bool WorldAddPlanet(World *world, Planet planet) {
    if (world->planetCount >= MAX_PLANETS) {
        return false;
    }

    world->planets[world->planetCount++] = planet;

    return true;
}

bool WorldAddProjectile(World *world, Projectile projectile) {
    if (world->projectileCount >= MAX_PROJECTILES) {
        return false;
    }

    world->projectiles[world->projectileCount++] = projectile;

    return true;
}

static void UpdateWorldPlayer(
    World *world,
    CameraController *controller,
    float deltaTime
)
{
    Vector3 forward =
        GetForwardDirection(controller);

    Vector3 horizontalForward =
        GetHorizontalForwardDirection(controller);

    Vector3 right = {
        -horizontalForward.z,
        0.0f,
        horizontalForward.x
    };

    Vector3 up = {
        0.0f,
        1.0f,
        0.0f
    };

    UpdatePlayer(
        &world->player,
        &world->planets[0],
        forward,
        right,
        up
    );

    UpdateShipTrails(
        &world->player,
        forward,
        right,
        up,
        deltaTime
    );
}

static void HandlePlayerShooting(
    World *world
)
{
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return;

    Projectile projectile =
        CreateProjectile(
            world->player.position,
            world->player.forward,
            10.0f,
            1500.0f,
            0
        );

    WorldAddProjectile(
        world,
        projectile
    );
}

static void HandleEnemyShooting(
    World *world
)
{
    for (int i = 0;
         i < world->enemyCount;
         i++)
    {
        Enemy *enemy =
            &world->enemies[i];

        if (!enemy->active)
            continue;

        if (!enemy->wantsToShoot)
            continue;

        Projectile projectile =
            CreateProjectile(
                enemy->position,
                enemy->forward,
                10.0f,
                1000.0f,
                1
            );

        WorldAddProjectile(
            world,
            projectile
        );

        enemy->wantsToShoot = false;
    }
}

static void UpdateWorldEnemies(
    World *world,
    float deltaTime
)
{
    for (int i = 0; i < world->enemyCount; i++) {
        Enemy *enemy =
            &world->enemies[i];

        if (!enemy->active)
            continue;

        UpdateEnemy(
            enemy,
            &world->planets[0],
            &world->player,
            deltaTime
        );
    }
}

static bool ProjectileHitsPlanet(
    World *world,
    Projectile *projectile
)
{
    for (int i = 0;
         i < world->planetCount;
         i++)
    {
        Planet *planet =
            &world->planets[i];


        if (!PlanetContainsSphere(
                planet,
                projectile->position,
                0.0f))
        {
            continue;
        }

        DamagePlanet(
            planet,
            projectile->position,
            20.0f
        );


        projectile->active = false;

        return true;
    }

    return false;
}

static bool ProjectileHitsPlayer(
    World *world,
    Projectile *projectile
)
{
    if (projectile->ownerId != 1)
        return false;

    Vector3 difference =
        Vector3Subtract(
            projectile->position,
            world->player.position
        );

    float distanceSquared =
        Vector3DotProduct(
            difference,
            difference
        );

    const float playerCollisionRadius = world->player.collisionRadius;

    if (
        distanceSquared <=
        playerCollisionRadius *
        playerCollisionRadius
    )
    {
        DamagePlayer(
            &world->player,
            projectile->damage
        );

        projectile->active = false;

        return true;
    }

    return false;
}

static bool ProjectileHitsEnemy(
    World *world,
    Projectile *projectile
)
{

    if (projectile->ownerId != 0)
        return false;


    for (int i = 0;
         i < world->enemyCount;
         i++)
    {
        Enemy *enemy =
            &world->enemies[i];

        if (!enemy->active)
            continue;


        float collisionDistance =
            enemy->collisionRadius;


        float distanceSquared =
            DistanceSquared(
                projectile->position,
                enemy->position
            );


        if (
            distanceSquared <=
            collisionDistance *
            collisionDistance
        )
        {
            DamageEnemy(
                enemy,
                projectile->damage
            );

            projectile->active = false;

            return true;
        }
    }

    return false;
}

static void UpdateWorldProjectiles(
    World *world,
    float deltaTime
)
{
    for (int i = 0;
         i < world->projectileCount;
         i++)
    {
        Projectile *projectile =
            &world->projectiles[i];

        if (!projectile->active)
            continue;

        UpdateProjectile(
            projectile,
            deltaTime
        );

        if (!projectile->active)
            continue;

        if (ProjectileHitsPlanet(
                world,
                projectile))
        {
            continue;
        }


        if (ProjectileHitsEnemy(
                world,
                projectile))
        {
            continue;
        }

        if (ProjectileHitsPlayer(
            world,
            projectile))
        {
            continue;
        }
    }
}

static void CleanupWorld(
    World *world
)
{
    RemoveInactiveEnemies(world);

    RemoveInactiveProjectiles(world);
}

void UpdateWorld(
    World *world,
    CameraController *controller,
    Camera *camera
)
{
    if (world == NULL ||
        controller == NULL ||
        camera == NULL)
    {
        return;
    }

    float deltaTime =
        GetFrameTime();

    if (deltaTime > 0.05f)
        deltaTime = 0.05f;

    UpdateCameraController(
        controller
    );

    UpdateWorldPlayer(
        world,
        controller,
        deltaTime
    );


    UpdateGameCamera(
        &world->gameCamera,
        camera,
        controller,
        &world->player,
        deltaTime
    );

    HandlePlayerShooting(
        world
    );


    UpdateWorldEnemies(
        world,
        deltaTime
    );

    HandleEnemyShooting(
        world
    );

    for (int i = 0;
         i < world->planetCount;
         i++)
    {
        UpdatePlanet(
            &world->planets[i],
            camera->position
        );
    }

    UpdateWorldProjectiles(
        world,
        deltaTime
    );

    CleanupWorld(
        world
    );
}


void DrawWorld(
    World *world,
    Camera3D camera
)
{
    if (world == NULL)
        return;

    for (int i = 0;
         i < world->planetCount;
         i++)
    {

        DrawPlanet(
            &world->planets[i],
            world->planets[i].material
        );
    }

    Vector3 horizontalForward =
        Vector3Normalize(
            (Vector3){
                world->player.forward.x,
                0.0f,
                world->player.forward.z
            }
        );

    Vector3 right = {
        -horizontalForward.z,
        0.0f,
        horizontalForward.x
    };

    Vector3 up = {
        0.0f,
        1.0f,
        0.0f
    };

    DrawPlayer(
        &world->player
    );

    DrawShipTrails(
        &world->player,
        world->player.forward,
        right,
        up,
        camera
    );

    for (int i = 0;
         i < world->enemyCount;
         i++)
    {
        DrawEnemy(
            &world->enemies[i]
        );
    }

    for (int i = 0;
         i < world->projectileCount;
         i++)
    {
       if (!world->projectiles[i].active)
           continue;

       DrawProjectile(
            &world->projectiles[i],
            camera
        );
    }

}

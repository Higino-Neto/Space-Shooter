#include "enemy.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>

#define ENEMY_MIN_DISTANCE_FROM_PLANET  20.0f
#define ENEMY_PATROL_ALTITUDE           8.0f

#define ENEMY_COMBAT_ENTER_FACTOR       0.80f
#define ENEMY_COMBAT_EXIT_FACTOR        1.35f

#define ENEMY_RETREAT_DISTANCE          45.0f
#define ENEMY_RETREAT_PLANET_DISTANCE   18.0f

#define ENEMY_ORBIT_STRENGTH            0.25f
#define ENEMY_APPROACH_STRENGTH         1.0f
#define ENEMY_AVOID_PLANET_STRENGTH     2.5f

#define ENEMY_CONTACT_DAMAGE_COOLDOWN   0.75f
#define ENEMY_CONTACT_DISTANCE_FACTOR   1.25f

#define ENEMY_MIN_DT                    0.0001f
#define ENEMY_MAX_DT                    0.05f



static float ClampFloat(float value, float min, float max)
{
    if (value < min)
        return min;

    if (value > max)
        return max;

    return value;
}


static float SafeLength(Vector3 vector)
{
    return sqrtf(
        vector.x * vector.x +
        vector.y * vector.y +
        vector.z * vector.z
    );
}


static Vector3 SafeNormalize(Vector3 vector)
{
    float length = SafeLength(vector);

    if (length < 0.00001f)
        return Vector3Zero();

    return Vector3Scale(vector, 1.0f / length);
}


static Vector3 ClampMagnitude(Vector3 vector, float maxMagnitude)
{
    float length = SafeLength(vector);

    if (length <= maxMagnitude)
        return vector;

    if (length < 0.00001f)
        return Vector3Zero();

    return Vector3Scale(vector, maxMagnitude / length);
}


static Vector3 GetPlanetDirection(
    const Planet *planet,
    Vector3 position
)
{
    return SafeNormalize(
        Vector3Subtract(position, planet->position)
    );
}


static Vector3 GetPlanetTangent(
    const Planet *planet,
    Vector3 position,
    Vector3 preferredDirection
)
{
    Vector3 normal = GetPlanetDirection(planet, position);

    Vector3 tangent = Vector3Subtract(
        preferredDirection,
        Vector3Scale(
            normal,
            Vector3DotProduct(preferredDirection, normal)
        )
    );

    if (SafeLength(tangent) < 0.001f)
    {
        Vector3 reference = (Vector3){ 0.0f, 1.0f, 0.0f };

        if (fabsf(Vector3DotProduct(reference, normal)) > 0.9f)
        {
            reference = (Vector3){ 1.0f, 0.0f, 0.0f };
        }

        tangent = Vector3CrossProduct(
            normal,
            reference
        );
    }

    return SafeNormalize(tangent);
}


static Vector3 GetOrbitDirection(
    const Planet *planet,
    Vector3 position,
    Vector3 targetDirection
)
{
    Vector3 normal = GetPlanetDirection(
        planet,
        position
    );

    Vector3 tangent = GetPlanetTangent(
        planet,
        position,
        targetDirection
    );

    Vector3 orbit = Vector3CrossProduct(
        normal,
        tangent
    );

    return SafeNormalize(orbit);
}

static Vector3 GetCombatOrbitDirection(
    Vector3 targetDirection,
    Vector3 referenceUp
)
{
    Vector3 orbitDirection =
        Vector3CrossProduct(
            referenceUp,
            targetDirection
        );

    return SafeNormalize(orbitDirection);
}

static Vector3 SteeringTowardsVelocity(
    Vector3 currentVelocity,
    Vector3 desiredVelocity,
    float maxAcceleration
)
{
    Vector3 steering = Vector3Subtract(
        desiredVelocity,
        currentVelocity
    );

    return ClampMagnitude(
        steering,
        maxAcceleration
    );
}

static Vector3 GetDistanceSteering(
    const Enemy *enemy,
    Vector3 targetPosition,
    float desiredDistance
)
{
    Vector3 offset = Vector3Subtract(
        targetPosition,
        enemy->position
  );

    float distance = SafeLength(offset);

    if (distance < 0.001f)
        return Vector3Zero();

    Vector3 direction = Vector3Scale(
        offset,
        1.0f / distance
    );

    float error = distance - desiredDistance;

    float deadZone = desiredDistance * 0.08f;

    if (fabsf(error) < deadZone)
        return Vector3Zero();

    float response = ClampFloat(
        error / desiredDistance,
        -1.0f,
        1.0f
    );

    return Vector3Scale(
        direction,
        response
    );
}

static Vector3 GetPlanetAvoidance(
    const Enemy *enemy,
    const Planet *planet
)
{
    Vector3 offset = Vector3Subtract(
        enemy->position,
        planet->position
    );

    float distance = SafeLength(offset);

    if (distance < 0.001f)
        return Vector3Zero();

    Vector3 normal = Vector3Scale(
        offset,
        1.0f / distance
    );

    float safeDistance =
        planet->radius +
        enemy->collisionRadius +
        ENEMY_MIN_DISTANCE_FROM_PLANET;

    if (distance >= safeDistance)
        return Vector3Zero();

    float penetration =
        safeDistance - distance;

    float strength =
        penetration *
        ENEMY_AVOID_PLANET_STRENGTH;

    return Vector3Scale(
        normal,
        strength
    );
}

static Vector3 ChoosePlanetTarget(
    const Planet *planet,
    Vector3 enemyPosition
)
{
    Vector3 direction = GetPlanetDirection(
        planet,
        enemyPosition
    );

    return Vector3Add(
        planet->position,
        Vector3Scale(
            direction,
            planet->radius
        )
    );
}


static Vector3 CalculatePlanetAttackAcceleration(
    Enemy *enemy,
    const Planet *planet
)
{

    Vector3 fromPlanet =
        Vector3Subtract(
            enemy->position,
            planet->position
        );

    float distance =
        Vector3Length(fromPlanet);

    if (distance < 0.001f)
        return Vector3Zero();

    Vector3 planetNormal =
        Vector3Scale(
            fromPlanet,
            1.0f / distance
        );



    float desiredDistance =
        planet->radius +
        enemy->attackDistance;



    float distanceError =
        distance - desiredDistance;

    Vector3 radialAcceleration =
        Vector3Scale(
            planetNormal,
            -distanceError * 0.8f
        );


    float radialVelocity =
        Vector3DotProduct(
            enemy->velocity,
            planetNormal
        );

    Vector3 tangentVelocity =
        Vector3Subtract(
            enemy->velocity,
            Vector3Scale(
                planetNormal,
                radialVelocity
            )
        );


    Vector3 tangentDirection =
        SafeNormalize(tangentVelocity);



    if (Vector3Length(tangentDirection) < 0.001f)
    {
        Vector3 reference =
            (fabsf(planetNormal.y) < 0.9f)
                ? (Vector3){ 0.0f, 1.0f, 0.0f }
                : (Vector3){ 1.0f, 0.0f, 0.0f };

        tangentDirection =
            SafeNormalize(
                Vector3CrossProduct(
                    planetNormal,
                    reference
                )
            );
    }


    Vector3 desiredTangentialVelocity =
        Vector3Scale(
            tangentDirection,
            enemy->maxSpeed * 0.35f
        );


    Vector3 tangentialAcceleration =
        Vector3Subtract(
            desiredTangentialVelocity,
            tangentVelocity
        );



    Vector3 toTarget =
        Vector3Subtract(
            enemy->planetTarget,
            enemy->position
        );

    float targetDistance =
        Vector3Length(toTarget);

    Vector3 targetDirection =
        SafeNormalize(toTarget);


    Vector3 targetAcceleration =
        Vector3Zero();

    float approachDistance =
        enemy->attackDistance * 1.25f;

    if (targetDistance > approachDistance)
    {
        targetAcceleration =
            Vector3Scale(
                targetDirection,
                enemy->maxAcceleration
            );
    }



    Vector3 acceleration =
        Vector3Add(
            radialAcceleration,
            tangentialAcceleration
        );

    acceleration =
        Vector3Add(
            acceleration,
            targetAcceleration
        );

    return acceleration;
}

static Vector3 GetPatrolAltitudeSteering(
    const Enemy *enemy,
    const Planet *planet
)
{
    Vector3 fromPlanet = Vector3Subtract(
        enemy->position,
        planet->position
    );

    float distance = SafeLength(fromPlanet);

    if (distance < 0.001f)
        return Vector3Zero();

    float desiredDistance =
        planet->radius +
        ENEMY_PATROL_ALTITUDE;

    float error =
        desiredDistance - distance;

    Vector3 normal = Vector3Scale(
        fromPlanet,
        1.0f / distance
    );

    return Vector3Scale(
        normal,
        error * 0.8f
    );
}



static void UpdateState(
    Enemy *enemy,
    const Planet *planet,
    const Player *player
)
{
    if (!enemy->active)
        return;

    float distanceToPlayer =
        Vector3Distance(
            enemy->position,
            player->position
        );

    if (enemy->state == ENEMY_RETREAT)
    {
        if (
            enemy->health > enemy->maxHealth * 0.45f &&
            distanceToPlayer >
                enemy->detectionRadius *
                ENEMY_COMBAT_EXIT_FACTOR
        )
        {
            enemy->state = ENEMY_ATTACK_PLANET;

            enemy->planetTarget =
                ChoosePlanetTarget(
                    planet,
                    enemy->position
                );
        }

        return;
    }

    if (
        enemy->health <=
        (int)(enemy->maxHealth * enemy->retreatHealth)
    )
    {
        enemy->state = ENEMY_RETREAT;
        return;
    }

    if (distanceToPlayer <= enemy->detectionRadius)
    {
        enemy->state = ENEMY_COMBAT;
        return;
    }

    if (enemy->state != ENEMY_ATTACK_PLANET)
    {
        enemy->state = ENEMY_ATTACK_PLANET;

        enemy->planetTarget =
            ChoosePlanetTarget(
                planet,
                enemy->position
            );
    }
}


static Vector3 CalculatePatrolAcceleration(
    const Enemy *enemy,
    const Planet *planet
)
{
    Vector3 normal = GetPlanetDirection(
        planet,
        enemy->position
    );

    Vector3 tangent = GetPlanetTangent(
        planet,
        enemy->position,
        enemy->forward
    );

    Vector3 desiredVelocity = Vector3Scale(
        tangent,
        enemy->maxSpeed * 0.55f
    );

    Vector3 acceleration = SteeringTowardsVelocity(
        enemy->velocity,
        desiredVelocity,
        enemy->maxAcceleration
    );

    Vector3 altitudeSteering =
        GetPatrolAltitudeSteering(
            enemy,
            planet
        );

    acceleration = Vector3Add(
        acceleration,
        altitudeSteering
    );

    acceleration = Vector3Add(
        acceleration,
        Vector3Scale(
            normal,
            0.15f
        )
    );

    return ClampMagnitude(
        acceleration,
        enemy->maxAcceleration
    );
}



static Vector3 CalculateCombatAcceleration(
    const Enemy *enemy,
    const Planet *planet,
    const Player *player
)
{
    Vector3 toPlayer =
        Vector3Subtract(
            player->position,
            enemy->position
        );

    float distance =
        SafeLength(toPlayer);

    if (distance < 0.001f)
        return Vector3Zero();

    Vector3 directionToPlayer =
        Vector3Scale(
            toPlayer,
            1.0f / distance
        );

    if (distance > enemy->attackDistance)
    {
        Vector3 desiredVelocity =
            Vector3Scale(
                directionToPlayer,
                enemy->maxSpeed
            );

        Vector3 acceleration =
            SteeringTowardsVelocity(
                enemy->velocity,
                desiredVelocity,
                enemy->maxAcceleration
            );

        return ClampMagnitude(
            acceleration,
            enemy->maxAcceleration
        );
    }


    Vector3 planetNormal =
        GetPlanetDirection(
            planet,
            enemy->position
        );

    Vector3 orbitDirection =
        GetCombatOrbitDirection(
            directionToPlayer,
            planetNormal
        );

    Vector3 orbitVelocity =
        Vector3Scale(
            orbitDirection,
            enemy->maxSpeed * ENEMY_ORBIT_STRENGTH
        );

    Vector3 desiredVelocity =
        orbitVelocity;

    desiredVelocity =
        ClampMagnitude(
            desiredVelocity,
            enemy->maxSpeed
        );

    Vector3 acceleration =
        SteeringTowardsVelocity(
            enemy->velocity,
            desiredVelocity,
            enemy->maxAcceleration
        );

    return ClampMagnitude(
        acceleration,
        enemy->maxAcceleration
    );
}

static Vector3 CalculateRetreatAcceleration(
    const Enemy *enemy,
    const Planet *planet,
    const Player *player
)
{
    Vector3 fromPlayer =
        Vector3Subtract(
            enemy->position,
            player->position
        );

    float playerDistance =
        SafeLength(fromPlayer);

    if (playerDistance < 0.001f)
    {
        fromPlayer =
            Vector3Scale(
                enemy->forward,
                -1.0f
            );

        playerDistance = 1.0f;
    }

    Vector3 awayFromPlayer =
        Vector3Scale(
            fromPlayer,
            1.0f / playerDistance
        );


    Vector3 toPlanet =
        Vector3Subtract(
            planet->position,
            enemy->position
        );

    float planetDistance =
        SafeLength(toPlanet);

    Vector3 planetDirection = Vector3Zero();

    if (planetDistance > 0.001f)
    {
        planetDirection =
            Vector3Scale(
                toPlanet,
                1.0f / planetDistance
            );
    }


    Vector3 desiredDirection =
        Vector3Add(
            Vector3Scale(
                awayFromPlayer,
                0.70f
            ),
            Vector3Scale(
                planetDirection,
                0.30f
            )
        );

    desiredDirection =
        SafeNormalize(
            desiredDirection
        );


    Vector3 desiredVelocity =
        Vector3Scale(
            desiredDirection,
            enemy->maxSpeed
        );


    Vector3 acceleration =
        SteeringTowardsVelocity(
            enemy->velocity,
            desiredVelocity,
            enemy->maxAcceleration
        );


    return ClampMagnitude(
        acceleration,
        enemy->maxAcceleration
    );
}



static void UpdateOrientation(
    Enemy *enemy,
    Vector3 desiredForward,
    float deltaTime
)
{
    desiredForward =
        SafeNormalize(desiredForward);

    if (SafeLength(desiredForward) < 0.001f)
        return;

    Vector3 currentForward =
        SafeNormalize(enemy->forward);

    if (SafeLength(currentForward) < 0.001f)
    {
        enemy->forward =
            desiredForward;

        return;
    }

    float maxAngle =
        enemy->maxTurnRate *
        deltaTime *
        DEG2RAD;

    float dot =
        ClampFloat(
            Vector3DotProduct(
                currentForward,
                desiredForward
            ),
            -1.0f,
            1.0f
        );

    float angle =
        acosf(dot);

    if (angle < 0.0001f)
    {
        enemy->forward =
            desiredForward;

        return;
    }

    float interpolation =
        ClampFloat(
            maxAngle / angle,
            0.0f,
            1.0f
        );

    Vector3 newForward =
        Vector3Lerp(
            currentForward,
            desiredForward,
            interpolation
        );

    enemy->forward =
        SafeNormalize(newForward);
}



static void IntegrateMovement(
    Enemy *enemy,
    float deltaTime
)
{
    enemy->velocity =
        Vector3Add(
            enemy->velocity,
            Vector3Scale(
                enemy->acceleration,
                deltaTime
            )
        );

    enemy->velocity =
        ClampMagnitude(
            enemy->velocity,
            enemy->maxSpeed
        );


    enemy->position =
        Vector3Add(
            enemy->position,
            Vector3Scale(
                enemy->velocity,
                deltaTime
            )
        );
}


static void ResolvePlanetCollision(Enemy *enemy, const Planet *planet) {
    Vector3 offset =
        Vector3Subtract(
            enemy->position,
            planet->position
        );

    float distance =
        SafeLength(offset);

    if (distance < 0.001f)
    {
        enemy->position =
            Vector3Add(
                planet->position,
                (Vector3){
                    0.0f,
                    planet->radius +
                    ENEMY_MIN_DISTANCE_FROM_PLANET,
                    0.0f
                }
            );

        enemy->velocity =
            Vector3Zero();

        return;
    }

    float minimumDistance =
        planet->radius +
        enemy->collisionRadius +
        ENEMY_MIN_DISTANCE_FROM_PLANET;


    if (distance < minimumDistance)
    {
        Vector3 normal =
            Vector3Scale(
                offset,
                1.0f / distance
            );

        enemy->position =
            Vector3Add(
                planet->position,
                Vector3Scale(
                    normal,
                    minimumDistance
                )
            );


        float radialVelocity =
            Vector3DotProduct(
                enemy->velocity,
                normal
            );

        if (radialVelocity < 0.0f)
        {
            enemy->velocity =
                Vector3Subtract(
                    enemy->velocity,
                    Vector3Scale(
                        normal,
                        radialVelocity
                    )
                );
        }
    }
}



Enemy CreateEnemy(Vector3 position, const Planet *planet) {
    Enemy enemy = { 0 };

    enemy.position = position;

    if (planet != NULL) {
        enemy.forward =
            GetPlanetDirection(
                planet,
                position
            );
    }
    else {
        enemy.forward =
            (Vector3){
                0.0f,
                0.0f,
                -1.0f
            };
    }

    enemy.velocity = Vector3Zero();
    enemy.acceleration = Vector3Zero();

    enemy.maxSpeed = 208.0f;
    enemy.maxAcceleration = 100.0f;
    enemy.maxTurnRate = 140.0f;

    enemy.strength = 10.0f;
    enemy.collisionRadius = 32.5f;
    enemy.shootCooldown = 0.3f;
    enemy.shootTimer = 0.0f;

    enemy.maxHealth = 30;
    enemy.health = enemy.maxHealth;

    enemy.state = ENEMY_PATROL;
    enemy.detectionRadius = 1500.0f;
    enemy.attackDistance = 1200.0f;
    enemy.retreatHealth = 0.25f;

    enemy.wantsToShoot = false;
    enemy.active = true;
    enemy.alive = true;

    return enemy;
}

static bool CanEnemyShootPlanet(
    const Enemy *enemy
)
{
    if (enemy->state != ENEMY_ATTACK_PLANET)
        return false;

    if (enemy->shootTimer > 0.0f)
        return false;

    Vector3 toTarget =
        Vector3Subtract(
            enemy->planetTarget,
            enemy->position
        );

    float distance =
        SafeLength(toTarget);

    if (distance < 0.001f)
        return false;

    if (distance > enemy->attackDistance)
        return false;

    Vector3 direction =
        Vector3Scale(
            toTarget,
            1.0f / distance
        );

    float alignment =
        Vector3DotProduct(
            enemy->forward,
            direction
        );

    if (alignment < 0.85f)
        return false;

    return true;
}

static bool CanEnemyShoot(
    const Enemy *enemy,
    const Player *player
)
{
    if (enemy->state != ENEMY_COMBAT)
        return false;

    if (enemy->shootTimer > 0.0f)
        return false;

    Vector3 toPlayer =
        Vector3Subtract(
            player->position,
            enemy->position
        );

    float distance =
        SafeLength(toPlayer);

    if (distance < 0.001f)
        return false;

    if (distance > enemy->attackDistance)
        return false;

    Vector3 directionToPlayer =
        Vector3Scale(
            toPlayer,
            1.0f / distance
        );

    float alignment =
        Vector3DotProduct(
            enemy->forward,
            directionToPlayer
        );

    if (alignment < 0.85f)
        return false;

    return true;
}


void UpdateEnemy(
    Enemy *enemy,
    const Planet *planet,
    const Player *player,
    float deltaTime
)
{
    if (enemy == NULL ||
        planet == NULL ||
        player == NULL)
    {
        return;
    }

    if (!enemy->active)
        return;

    if (deltaTime < ENEMY_MIN_DT)
        deltaTime = ENEMY_MIN_DT;

    if (deltaTime > ENEMY_MAX_DT)
        deltaTime = ENEMY_MAX_DT;


    enemy->shootTimer -= deltaTime;

    if (enemy->shootTimer < 0.0f)
        enemy->shootTimer = 0.0f;

    enemy->wantsToShoot = false;


    UpdateState(
        enemy,
        planet,
        player
    );


switch (enemy->state)
{
    case ENEMY_PATROL:
        enemy->acceleration =
            CalculatePatrolAcceleration(
                enemy,
                planet
            );
        break;

    case ENEMY_ATTACK_PLANET:
        enemy->acceleration =
            CalculatePlanetAttackAcceleration(
                enemy,
                planet
            );
        break;

    case ENEMY_COMBAT:
        enemy->acceleration =
            CalculateCombatAcceleration(
                enemy,
                planet,
                player
            );
        break;

    case ENEMY_RETREAT:
        enemy->acceleration =
            CalculateRetreatAcceleration(
                enemy,
                planet,
                player
            );
        break;

    default:
        enemy->acceleration = Vector3Zero();
        break;
}


    Vector3 planetAvoidance =
        GetPlanetAvoidance(
            enemy,
            planet
        );

    enemy->acceleration =
        Vector3Add(
            enemy->acceleration,
            planetAvoidance
        );

    enemy->acceleration =
        ClampMagnitude(
            enemy->acceleration,
            enemy->maxAcceleration
        );


    IntegrateMovement(
        enemy,
        deltaTime
    );


    ResolvePlanetCollision(
        enemy,
        planet
    );


    Vector3 desiredForward;

    switch (enemy->state)
    {
        case ENEMY_COMBAT:
        {
            Vector3 toPlayer =
                Vector3Subtract(
                    player->position,
                    enemy->position
                );

            desiredForward =
                SafeNormalize(toPlayer);

            break;
        }

        case ENEMY_ATTACK_PLANET:
        {
            Vector3 toTarget =
                Vector3Subtract(
                    enemy->planetTarget,
                    enemy->position
                );

            desiredForward =
                SafeNormalize(toTarget);

            break;
        }

        case ENEMY_PATROL:
        case ENEMY_RETREAT:
        default:
        {
            desiredForward =
                SafeNormalize(enemy->velocity);

            break;
        }
    }
    UpdateOrientation(
        enemy,
        desiredForward,
        deltaTime
    );

    if (enemy->state == ENEMY_COMBAT)
    {
        if (CanEnemyShoot(enemy, player))
        {
            enemy->wantsToShoot = true;
            enemy->shootTimer = enemy->shootCooldown;
        }
    }
    else if (enemy->state == ENEMY_ATTACK_PLANET)
    {
        if (CanEnemyShootPlanet(enemy))
        {
            enemy->wantsToShoot = true;
            enemy->shootTimer = enemy->shootCooldown;
        }
    }



    if (enemy->health <= 0)
    {
        enemy->health = 0;

        enemy->active = false;

        enemy->velocity =
            Vector3Zero();

        enemy->acceleration =
            Vector3Zero();
    }
}


void DamageEnemy(
    Enemy *enemy,
    float damage
)
{
    if (enemy == NULL)
        return;

    if (!enemy->active)
        return;

    if (damage <= 0.0f)
        return;


    enemy->health -=
        (int)ceilf(damage);


    if (enemy->health <= 0)
    {
        enemy->health = 0;
        enemy->active = false;
        enemy->alive = false;

        enemy->velocity =
            Vector3Zero();

        enemy->acceleration =
            Vector3Zero();
    }
}



void DrawEnemy(
    const Enemy *enemy
)
{
    // if (enemy == NULL)
    //     return;

    // if (!enemy->active)
    //     return;


    // Vector3 forward =
    //     SafeNormalize(
    //         enemy->forward
    //     );

    // if (SafeLength(forward) < 0.001f)
    // {
    //     forward =
    //         (Vector3){
    //             0.0f,
    //             0.0f,
    //             -1.0f
    //         };
    // }

    // DrawSphere(
    //     enemy->position,
    //     enemy->collisionRadius,
    //     RED
    // );


    // Vector3 nose =
    //     Vector3Add(
    //         enemy->position,
    //         Vector3Scale(
    //             forward,
    //             enemy->collisionRadius * 1.8f
    //         )
    //     );

    // DrawSphere(
    //     nose,
    //     enemy->collisionRadius * 0.55f,
    //     MAROON
    // );


    // DrawLine3D(
    //     enemy->position,
    //     Vector3Add(
    //         enemy->position,
    //         Vector3Scale(
    //             forward,
    //             enemy->collisionRadius * 3.0f
    //         )
    //     ),
    //     YELLOW
    // );

    // DrawSphere(
    //     Vector3Add(
    //         enemy->position,
    //         (Vector3){
    //             0.0f,
    //             enemy->collisionRadius * 1.5f,
    //             0.0f
    //         }
    //     ),
    //     enemy->collisionRadius * 0.15f,
    //     GREEN
    // );


    if (enemy == NULL || !enemy->active)
        return;

    Vector3 forward = Vector3Normalize(enemy->forward);

    if (Vector3Length(forward) < 0.001f)
        forward = (Vector3) { 0.0f, 0.0f, 1.0f };

    if (enemy->visualModel != NULL) {
        Vector3 baseForward = { 0.0f, 0.0f, -1.0f };

        float dot = Vector3DotProduct(baseForward, forward);
        dot = Clamp(dot, -1.0f, 1.0f);

        Vector3 axis = Vector3CrossProduct(baseForward, forward);
        float axisLength = Vector3Length(axis);
        float angle = acosf(dot) * RAD2DEG;

        if (axisLength > 0.001f) {
            axis = Vector3Scale(axis, 1.0f / axisLength);
        } else {
            axis = (Vector3) { 0.0f, 1.0f, 0.0f };
        }

        DrawModelEx(
            *enemy->visualModel,
            enemy->position,
            axis,
            angle,
            (Vector3){
                enemy->modelScale,
                enemy->modelScale,
                enemy->modelScale
            },
            WHITE
        );
    }
    else {
        DrawSphere(enemy->position, enemy->collisionRadius, RED);
    }
}

EnemyAssets LoadEnemyAssets(void) {
    EnemyAssets assets = {
        .bigShip = LoadModel("assets/enemy/smallShip/Destroyer_01.fbx"),
        .mediumShip = LoadModel("assets/enemy/smallShip/Destroyer_01.fbx"),
        .smallShip = LoadModel("assets/enemy/smallShip/Destroyer_01.fbx")
    };

    // TraceLog(
    //     LOG_INFO,
    //     "BigShip: %d meshes",
    //     assets.bigShip.meshCount
    // );

    // TraceLog(
    //     LOG_INFO,
    //     "MediumShip: %d meshes",
    //     assets.mediumShip.meshCount
    // );

    // TraceLog(
    //     LOG_INFO,
    //     "SmallShip: %d meshes",
    //     assets.smallShip.meshCount
    // );

    // Texture2D bigShipTexture =
    //     LoadTexture("assets/enemy/BigShip/spaceCraft3.png");

    // Texture2D mediumShipTexture =
    //     LoadTexture("assets/enemy/mediumShip/spaceCraft2.png");

    // Texture2D smallShipTexture =
    //     LoadTexture("assets/enemy/smallShip/spaceCraft1.png");

    // for (int i = 0; i < assets.bigShip.materialCount; i++) {
    //     SetMaterialTexture(
    //         &assets.bigShip.materials[i],
    //         MATERIAL_MAP_DIFFUSE,
    //         bigShipTexture
    //     );
    // }

    // for (int i = 0; i < assets.mediumShip.materialCount; i++) {
    //     SetMaterialTexture(
    //         &assets.mediumShip.materials[i],
    //         MATERIAL_MAP_DIFFUSE,
    //         mediumShipTexture
    //     );
    // }

    // for (int i = 0; i < assets.smallShip.materialCount; i++) {
    //     SetMaterialTexture(
    //         &assets.smallShip.materials[i],
    //         MATERIAL_MAP_DIFFUSE,
    //         smallShipTexture
    //     );
    // }

    return assets;
}

void UnloadEnemyAssets(EnemyAssets *assets) {
    UnloadModel(assets->bigShip);
    UnloadModel(assets->mediumShip);
    UnloadModel(assets->smallShip);
}
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

#define ENEMY_WAVE_HEALTH_GROWTH        0.18f
#define ENEMY_WAVE_DAMAGE_GROWTH        0.09f
#define ENEMY_WAVE_SPEED_GROWTH         0.025f
#define ENEMY_WAVE_SPEED_CAP            1.45f
#define ENEMY_WAVE_XP_GROWTH            0.10f
#define ENEMY_WAVE_SHIELD_FACTOR        0.80f
#define ENEMY_WAVE_HEAL_FACTOR          0.75f



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



typedef struct {
    float maxSpeed;
    float maxAcceleration;
    float maxTurnRate;

    float strength;
    float collisionRadius;

    float shootCooldown;
    int projectileCount;
    float projectileSpeed;
    float burstSpread;

    int maxHealth;
    int maxShield;
    float shieldRechargeDelay;
    float shieldRechargeRate;

    int xpReward;

    float detectionRadius;
    float attackDistance;
    float retreatHealth;

    bool splitOnDeath;
    int splitCount;

    float cloakInterval;
    float cloakDuration;

    float healCooldown;
    float healRadius;
    int healAmount;

    float contactDamage;
    float contactCooldown;

    float spawnDistance;
    float modelScale;
    Color tint;
} EnemyArchetype;

static const EnemyArchetype enemyArchetypes[ENEMY_TYPE_COUNT] = {
    [ENEMY_INTERCEPTOR] = {
        .maxSpeed = 290.0f,
        .maxAcceleration = 170.0f,
        .maxTurnRate = 230.0f,
        .strength = 6.0f,
        .collisionRadius = 42.0f,
        .shootCooldown = 0.22f,
        .projectileCount = 1,
        .projectileSpeed = 1200.0f,
        .maxHealth = 18,
        .xpReward = 20,
        .detectionRadius = 1800.0f,
        .attackDistance = 650.0f,
        .retreatHealth = 0.0f,
        .spawnDistance = 2600.0f,
        .modelScale = 0.032f,
        .tint = { 255, 255, 255, 255 }
    },
    [ENEMY_SHIELD] = {
        .maxSpeed = 185.0f,
        .maxAcceleration = 90.0f,
        .maxTurnRate = 110.0f,
        .strength = 9.0f,
        .collisionRadius = 58.0f,
        .shootCooldown = 0.55f,
        .projectileCount = 1,
        .projectileSpeed = 950.0f,
        .maxHealth = 45,
        .maxShield = 40,
        .shieldRechargeDelay = 3.5f,
        .shieldRechargeRate = 10.0f,
        .xpReward = 45,
        .detectionRadius = 1600.0f,
        .attackDistance = 1100.0f,
        .retreatHealth = 0.25f,
        .spawnDistance = 3000.0f,
        .modelScale = 0.038f,
        .tint = { 170, 220, 255, 255 }
    },
    [ENEMY_BOMBER] = {
        .maxSpeed = 145.0f,
        .maxAcceleration = 65.0f,
        .maxTurnRate = 75.0f,
        .strength = 16.0f,
        .collisionRadius = 95.0f,
        .shootCooldown = 1.4f,
        .projectileCount = 3,
        .projectileSpeed = 850.0f,
        .burstSpread = 0.14f,
        .maxHealth = 100,
        .xpReward = 80,
        .detectionRadius = 1700.0f,
        .attackDistance = 1400.0f,
        .retreatHealth = 0.15f,
        .spawnDistance = 3200.0f,
        .modelScale = 0.044f,
        .tint = { 255, 190, 190, 255 }
    },
    [ENEMY_RAMMER] = {
        .maxSpeed = 330.0f,
        .maxAcceleration = 220.0f,
        .maxTurnRate = 260.0f,
        .strength = 0.0f,
        .collisionRadius = 48.0f,
        .shootCooldown = 0.0f,
        .projectileCount = 0,
        .maxHealth = 35,
        .xpReward = 30,
        .detectionRadius = 2200.0f,
        .attackDistance = 0.0f,
        .retreatHealth = 0.0f,
        .contactDamage = 18.0f,
        .contactCooldown = 0.90f,
        .spawnDistance = 2800.0f,
        .modelScale = 0.033f,
        .tint = { 235, 140, 140, 255 }
    },
    [ENEMY_FIGHTER] = {
        .maxSpeed = 208.0f,
        .maxAcceleration = 100.0f,
        .maxTurnRate = 140.0f,
        .strength = 10.0f,
        .collisionRadius = 50.0f,
        .shootCooldown = 0.30f,
        .projectileCount = 1,
        .projectileSpeed = 1000.0f,
        .maxHealth = 30,
        .xpReward = 25,
        .detectionRadius = 1500.0f,
        .attackDistance = 1200.0f,
        .retreatHealth = 0.0f,
        .spawnDistance = 3000.0f,
        .modelScale = 0.040f,
        .tint = { 255, 255, 255, 255 }
    },
    [ENEMY_SUPPORT] = {
        .maxSpeed = 165.0f,
        .maxAcceleration = 85.0f,
        .maxTurnRate = 110.0f,
        .strength = 4.0f,
        .collisionRadius = 48.0f,
        .shootCooldown = 0.90f,
        .projectileCount = 1,
        .projectileSpeed = 800.0f,
        .maxHealth = 55,
        .xpReward = 60,
        .detectionRadius = 1600.0f,
        .attackDistance = 1150.0f,
        .retreatHealth = 0.30f,
        .healCooldown = 4.0f,
        .healRadius = 750.0f,
        .healAmount = 12,
        .spawnDistance = 3400.0f,
        .modelScale = 0.036f,
        .tint = { 160, 230, 255, 255 }
    },
    [ENEMY_SPLITTER] = {
        .maxSpeed = 195.0f,
        .maxAcceleration = 95.0f,
        .maxTurnRate = 140.0f,
        .strength = 8.0f,
        .collisionRadius = 52.0f,
        .shootCooldown = 0.55f,
        .projectileCount = 1,
        .projectileSpeed = 950.0f,
        .maxHealth = 60,
        .xpReward = 55,
        .detectionRadius = 1600.0f,
        .attackDistance = 1100.0f,
        .retreatHealth = 0.20f,
        .splitOnDeath = true,
        .splitCount = 2,
        .spawnDistance = 3100.0f,
        .modelScale = 0.038f,
        .tint = { 190, 225, 255, 255 }
    },
    [ENEMY_PHANTOM] = {
        .maxSpeed = 240.0f,
        .maxAcceleration = 140.0f,
        .maxTurnRate = 190.0f,
        .strength = 12.0f,
        .collisionRadius = 46.0f,
        .shootCooldown = 0.70f,
        .projectileCount = 1,
        .projectileSpeed = 1300.0f,
        .maxHealth = 40,
        .xpReward = 65,
        .detectionRadius = 1700.0f,
        .attackDistance = 900.0f,
        .retreatHealth = 0.0f,
        .cloakInterval = 5.0f,
        .cloakDuration = 2.5f,
        .spawnDistance = 3200.0f,
        .modelScale = 0.034f,
        .tint = { 200, 215, 235, 255 }
    },
    [ENEMY_SNIPER] = {
        .maxSpeed = 155.0f,
        .maxAcceleration = 75.0f,
        .maxTurnRate = 95.0f,
        .strength = 22.0f,
        .collisionRadius = 50.0f,
        .shootCooldown = 2.40f,
        .projectileCount = 1,
        .projectileSpeed = 1700.0f,
        .maxHealth = 45,
        .xpReward = 70,
        .detectionRadius = 2600.0f,
        .attackDistance = 2200.0f,
        .retreatHealth = 0.20f,
        .spawnDistance = 3600.0f,
        .modelScale = 0.036f,
        .tint = { 225, 225, 225, 255 }
    },
    [ENEMY_DREADNOUGHT] = {
        .maxSpeed = 115.0f,
        .maxAcceleration = 50.0f,
        .maxTurnRate = 55.0f,
        .strength = 20.0f,
        .collisionRadius = 110.0f,
        .shootCooldown = 1.50f,
        .projectileCount = 5,
        .projectileSpeed = 900.0f,
        .burstSpread = 0.11f,
        .maxHealth = 320,
        .xpReward = 300,
        .detectionRadius = 2200.0f,
        .attackDistance = 1600.0f,
        .retreatHealth = 0.0f,
        .spawnDistance = 3800.0f,
        .modelScale = 0.055f,
        .tint = { 255, 160, 160, 255 }
    }
};

Enemy CreateEnemyType(EnemyType type, Vector3 position, const Planet *planet) {
    Enemy enemy = { 0 };

    if (type < 0 || type >= ENEMY_TYPE_COUNT) {
        type = ENEMY_FIGHTER;
    }

    const EnemyArchetype *archetype = &enemyArchetypes[type];

    enemy.type = type;
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

    enemy.maxSpeed = archetype->maxSpeed;
    enemy.maxAcceleration = archetype->maxAcceleration;
    enemy.maxTurnRate = archetype->maxTurnRate;

    enemy.strength = archetype->strength;
    enemy.collisionRadius = archetype->collisionRadius;
    enemy.shootCooldown = archetype->shootCooldown;
    enemy.shootTimer = 0.0f;
    enemy.projectileCount = archetype->projectileCount;
    enemy.projectileSpeed = archetype->projectileSpeed;
    enemy.burstSpread = archetype->burstSpread;

    enemy.maxHealth = archetype->maxHealth;
    enemy.health = enemy.maxHealth;

    enemy.maxShield = archetype->maxShield;
    enemy.shield = enemy.maxShield;
    enemy.shieldRechargeDelay = archetype->shieldRechargeDelay;
    enemy.shieldRechargeTimer = 0.0f;
    enemy.shieldRechargeRate = archetype->shieldRechargeRate;
    enemy.shieldRechargeAccumulator = 0.0f;

    enemy.xpReward = archetype->xpReward;

    enemy.state = ENEMY_PATROL;
    enemy.detectionRadius = archetype->detectionRadius;
    enemy.attackDistance = archetype->attackDistance;
    enemy.retreatHealth = archetype->retreatHealth;

    enemy.splitOnDeath = archetype->splitOnDeath;
    enemy.splitCount = archetype->splitCount;

    enemy.cloakInterval = archetype->cloakInterval;
    enemy.cloakDuration = archetype->cloakDuration;
    enemy.cloakTimer = archetype->cloakInterval;
    enemy.cloaked = false;

    enemy.healCooldown = archetype->healCooldown;
    enemy.healTimer = archetype->healCooldown;
    enemy.healRadius = archetype->healRadius;
    enemy.healAmount = archetype->healAmount;
    enemy.wantsToHeal = false;
    enemy.healPulse = 0.0f;

    enemy.contactDamage = archetype->contactDamage;
    enemy.contactCooldown = archetype->contactCooldown;
    enemy.contactTimer = 0.0f;

    enemy.spawnDistance = archetype->spawnDistance;
    enemy.modelScale = archetype->modelScale;
    enemy.tint = archetype->tint;

    enemy.wantsToShoot = false;
    enemy.active = true;
    enemy.alive = true;

    return enemy;
}

void ScaleEnemyForWave(Enemy *enemy, int waveNumber) {
    if (enemy == NULL || waveNumber < 1) {
        return;
    }

    float wave = (float)(waveNumber - 1);

    float healthScale = 1.0f + ENEMY_WAVE_HEALTH_GROWTH * wave;
    float damageScale = 1.0f + ENEMY_WAVE_DAMAGE_GROWTH * wave;
    float speedScale = 1.0f + ENEMY_WAVE_SPEED_GROWTH * wave;
    float xpScale = 1.0f + ENEMY_WAVE_XP_GROWTH * wave;

    if (speedScale > ENEMY_WAVE_SPEED_CAP) {
        speedScale = ENEMY_WAVE_SPEED_CAP;
    }

    enemy->maxHealth = (int)ceilf(enemy->maxHealth * healthScale);
    enemy->health = enemy->maxHealth;

    enemy->strength *= damageScale;

    enemy->maxSpeed *= speedScale;
    enemy->maxAcceleration *= speedScale;
    enemy->maxTurnRate *= speedScale;

    if (enemy->maxTurnRate > 420.0f) {
        enemy->maxTurnRate = 420.0f;
    }

    enemy->xpReward = (int)(enemy->xpReward * xpScale);

    enemy->contactDamage *= damageScale;

    if (enemy->maxShield > 0) {
        enemy->maxShield = (int)ceilf(
            enemy->maxShield *
            healthScale *
            ENEMY_WAVE_SHIELD_FACTOR
        );

        enemy->shield = enemy->maxShield;
    }

    if (enemy->healAmount > 0) {
        enemy->healAmount = (int)ceilf(
            enemy->healAmount *
            healthScale *
            ENEMY_WAVE_HEAL_FACTOR
        );
    }
}

static bool CanEnemyShootPlanet(
    const Enemy *enemy
)
{
    if (enemy->cloaked)
        return false;

    if (enemy->shootCooldown <= 0.0f ||
        enemy->projectileCount <= 0)
        return false;

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
    if (enemy->cloaked)
        return false;

    if (enemy->shootCooldown <= 0.0f ||
        enemy->projectileCount <= 0)
        return false;

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

    if (enemy->contactTimer > 0.0f)
    {
        enemy->contactTimer -= deltaTime;

        if (enemy->contactTimer < 0.0f)
            enemy->contactTimer = 0.0f;
    }

    if (enemy->cloakInterval > 0.0f &&
        enemy->cloakDuration > 0.0f)
    {
        enemy->cloakTimer -= deltaTime;

        if (enemy->cloakTimer <= 0.0f)
        {
            enemy->cloaked = !enemy->cloaked;

            enemy->cloakTimer =
                enemy->cloaked
                    ? enemy->cloakDuration
                    : enemy->cloakInterval;
        }
    }

    enemy->wantsToHeal = false;

    if (enemy->healCooldown > 0.0f)
    {
        enemy->healTimer -= deltaTime;

        if (enemy->healTimer <= 0.0f)
        {
            enemy->healTimer = enemy->healCooldown;
            enemy->wantsToHeal = true;
            enemy->healPulse = 1.0f;
        }
    }

    if (enemy->healPulse > 0.0f)
    {
        enemy->healPulse -= deltaTime * 2.5f;

        if (enemy->healPulse < 0.0f)
            enemy->healPulse = 0.0f;
    }

    if (enemy->maxShield > 0)
    {
        if (enemy->shieldRechargeTimer > 0.0f)
        {
            enemy->shieldRechargeTimer -= deltaTime;
        }
        else if (enemy->shield < enemy->maxShield)
        {
            enemy->shieldRechargeAccumulator +=
                enemy->shieldRechargeRate * deltaTime;

            if (enemy->shieldRechargeAccumulator >= 1.0f)
            {
                int restored =
                    (int)enemy->shieldRechargeAccumulator;

                enemy->shieldRechargeAccumulator -= restored;
                enemy->shield += restored;

                if (enemy->shield > enemy->maxShield)
                    enemy->shield = enemy->maxShield;
            }
        }
    }

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

    enemy->cloaked = false;
    enemy->cloakTimer = enemy->cloakInterval;

    int amount = (int)ceilf(damage);

    if (enemy->maxShield > 0)
    {
        enemy->shieldRechargeTimer =
            enemy->shieldRechargeDelay;

        enemy->shieldRechargeAccumulator = 0.0f;
    }

    if (enemy->shield > 0)
    {
        int absorbed = enemy->shield < amount
            ? enemy->shield
            : amount;

        enemy->shield -= absorbed;
        amount -= absorbed;
    }

    if (amount > 0)
        enemy->health -= amount;


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
        Color tint = enemy->tint;

        if (enemy->cloaked)
            tint.a = 45;

        Vector3 baseForward = { -1.0f, 0.0f, 0.0f };

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

        Vector3 localOffset = {
            0.0f,
            -1000.0f,
            0.0f
        };

        Matrix rotation = MatrixRotate(axis, angle * DEG2RAD);

        Vector3 offset = Vector3Transform(
            Vector3Scale(localOffset, enemy->modelScale),
            rotation
        );

        Vector3 drawPosition = Vector3Add(
            enemy->position,
            offset
        );

        DrawModelEx(
            *enemy->visualModel,
            drawPosition,
            axis,
            angle,
            (Vector3) {
                enemy->modelScale,
                enemy->modelScale,
                enemy->modelScale
            },
            tint
        );
    }
    else {
        DrawSphere(
            enemy->position,
            enemy->collisionRadius,
            RED
        );
    }

    if (enemy->maxShield > 0 &&
        enemy->shield > 0)
    {
        float ratio = (float)enemy->shield /
                      (float)enemy->maxShield;

        Color shieldColor = {
            54,
            197,
            232,
            (unsigned char)(60.0f + 110.0f * ratio)
        };

        DrawSphereWires(
            enemy->position,
            enemy->collisionRadius * 2.4f,
            8,
            16,
            shieldColor
        );
    }

    if (enemy->healPulse > 0.0f &&
        enemy->healRadius > 0.0f)
    {
        float radius =
            enemy->healRadius *
            (0.35f + 0.65f * (1.0f - enemy->healPulse));

        Color pulseColor = {
            54,
            197,
            232,
            (unsigned char)(120.0f * enemy->healPulse)
        };

        DrawSphereWires(
            enemy->position,
            radius,
            8,
            24,
            pulseColor
        );
    }
}

static const char *enemyShipPaths[ENEMY_TYPE_COUNT] = {
    [ENEMY_INTERCEPTOR] = "assets/enemy/smallShip/enemyShip1.glb",
    [ENEMY_SHIELD] = "assets/enemy/mediumShip/enemyShip2.glb",
    [ENEMY_BOMBER] = "assets/enemy/mediumShip/enemyShip10.glb",
    [ENEMY_RAMMER] = "assets/enemy/mediumShip/enemyShip4.glb",
    [ENEMY_FIGHTER] = "assets/enemy/mediumShip/enemyShip5.glb",
    [ENEMY_SUPPORT] = "assets/enemy/mediumShip/enemyShip6.glb",
    [ENEMY_SPLITTER] = "assets/enemy/mediumShip/enemyShip7.glb",
    [ENEMY_PHANTOM] = "assets/enemy/mediumShip/enemyShip8.glb",
    [ENEMY_SNIPER] = "assets/enemy/mediumShip/enemyShip9.glb",
    [ENEMY_DREADNOUGHT] = "assets/enemy/bigShip/enemyShip3.glb"
};

EnemyAssets LoadEnemyAssets(void) {
    EnemyAssets assets = { 0 };

    assets.texture =
        LoadTexture("assets/enemy/smallShip/enemyShip1.png");

    for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
    {
        assets.ships[i] = LoadModel(enemyShipPaths[i]);

        for (int m = 0;
             m < assets.ships[i].materialCount;
             m++)
        {
            assets.ships[i]
                .materials[m]
                .maps[MATERIAL_MAP_DIFFUSE]
                .texture = assets.texture;
        }
    }

    return assets;
}

void UnloadEnemyAssets(EnemyAssets *assets) {
    if (assets == NULL)
        return;

    for (int i = 0; i < ENEMY_TYPE_COUNT; i++)
    {
        for (int m = 0;
             m < assets->ships[i].materialCount;
             m++)
        {
            assets->ships[i]
                .materials[m]
                .maps[MATERIAL_MAP_DIFFUSE]
                .texture = (Texture2D){ 0 };
        }

        UnloadModel(assets->ships[i]);
    }

    UnloadTexture(assets->texture);

    assets->texture = (Texture2D){ 0 };
}
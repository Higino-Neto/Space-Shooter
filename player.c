#include "raylib.h"
#include "raymath.h"
#include "player.h"
#include "planet.h"
#include "world.h"
#include <math.h>
#include <stdlib.h>


Player CreatePlayer()
{
    Player player = { 0 };

    player.position = (Vector3){
        0.0f,
        0.0f,
        0.0f
    };

    player.health = 100.0f;
    player.speed = 300.0f;
    player.collisionRadius = 40.0f;

    player.acceleration = 400.0f;
    player.deceleration = 100.0f;
    player.turnResponsiveness = 4.0f;

    player.model = LoadModel("assets/player/Ship/Ship.glb");

    Texture2D texture =
        LoadTexture("assets/player/Ship/Ship.png");

    for (int i = 0; i < player.model.materialCount; i++)
    {
        player.model.materials[i]
            .maps[MATERIAL_MAP_DIFFUSE]
            .texture = texture;
    }

    player.trailConfig = (TrailConfig) {
        .lifetime = 0.8f,
        .sampleInterval = 0.015f,

        .exhaustDistance = 4.0f,
        .exhaustSeparation = 3.2f,

        .width = 0.85f,
        .coreWidth = 0.22f,

        .color = (Color){ 0, 100, 255, 255 },
        .coreColor = (Color){ 150, 240, 255, 255 },

        .followSpeed = 15.0f
    };

    player.trailForward = (Vector3){
        0.0f,
        0.0f,
        -1.0f
    };

    player.trailRight = (Vector3){
        1.0f,
        0.0f,
        0.0f
    };

    player.trailUp = (Vector3){
        0.0f,
        1.0f,
        0.0f
    };

    return player;
}


void UpdatePlayer(
    Player *player,
    Planet *planet,
    Vector3 forward,
    Vector3 right,
    Vector3 up
)
{
    if (player == NULL)
        return;

    float deltaTime = GetFrameTime();

    // ResolvePlanetCollision(player, planet);

    Vector3 input = { 0 };

    if (IsKeyDown(KEY_W))
        input.z += 1.0f;

    if (IsKeyDown(KEY_S))
        input.z -= 1.0f;

    if (IsKeyDown(KEY_D))
        input.x += 1.0f;

    if (IsKeyDown(KEY_A))
        input.x -= 1.0f;

    if (IsKeyDown(KEY_SPACE))
        input.y += 1.0f;

    if (IsKeyDown(KEY_LEFT_SHIFT))
        input.y -= 1.0f;

    if (Vector3LengthSqr(input) > 0.0001f)
    {
        input = Vector3Normalize(input);
    }

    Vector3 desiredVelocity = { 0 };

    if (Vector3LengthSqr(input) > 0.0001f)
    {
        Vector3 movement = { 0 };

        movement = Vector3Add(
            movement,
            Vector3Scale(right, input.x)
        );

        movement = Vector3Add(
            movement,
            Vector3Scale(up, input.y)
        );

        movement = Vector3Add(
            movement,
            Vector3Scale(forward, input.z)
        );

        movement = Vector3Normalize(movement);

        desiredVelocity =
            Vector3Scale(
                movement,
                player->speed
            );
    }

    Vector3 velocityDifference =
        Vector3Subtract(
            desiredVelocity,
            player->velocity
        );


 
    float response;

    if (Vector3LengthSqr(desiredVelocity) > 0.0001f)
    {
        response = player->acceleration;
    }
    else
    {
        response = player->deceleration;
    }

    float maxChange =
        response * deltaTime;

    float differenceLength =
        Vector3Length(velocityDifference);

    if (differenceLength > maxChange)
    {
        velocityDifference =
            Vector3Scale(
                Vector3Normalize(
                    velocityDifference
                ),
                maxChange
            );
    }

    player->velocity =
        Vector3Add(
            player->velocity,
            velocityDifference
        );

    float lateralSpeed =
        Vector3DotProduct(
            player->velocity,
            right
        );

    float normalizedLateral =
        Clamp(
            lateralSpeed / player->speed,
            -1.0f,
            1.0f
        );

    UpdatePlayerManeuver(
        player,
        normalizedLateral,
        deltaTime
    );

    if (!player->rollManeuverActive)
    {
        const float normalRoll = 0.85f;

        player->targetRoll =
            normalizedLateral * normalRoll;

        float rollT =
            1.0f - expf(
                -player->turnResponsiveness *
                deltaTime
            );

        player->roll =
            Lerp(
                player->roll,
                player->targetRoll,
                rollT
            );
    }

    player->position =
        Vector3Add(
            player->position,
            Vector3Scale(
                player->velocity,
                deltaTime
            )
        );


    float turnT =
        1.0f - expf(
            -player->turnResponsiveness * deltaTime
        );

    player->forward =
        Vector3Normalize(
            Vector3Lerp(
                player->forward,
                forward,
                turnT
            )
        );
}

void UpdatePlayerManeuver(
    Player *player,
    float normalizedLateral,
    float deltaTime
)
{
    if (player == NULL)
        return;

    const float maneuverThreshold = 0.80f;
    const float maneuverDelay = 0.35f;
    const float maneuverSpeed = 8.0f;

    const float maneuverOvershoot = 0.20f;

    if (player->rollManeuverActive)
    {
        if (player->rollManeuverProgress <
            player->rollManeuverTarget)
        {
            float rollStep =
                maneuverSpeed * deltaTime;

            player->roll +=
                player->rollManeuverDirection *
                rollStep;

            player->rollManeuverProgress +=
                rollStep;

            if (player->rollManeuverProgress >=
                player->rollManeuverTarget)
            {
                player->rollManeuverProgress =
                    player->rollManeuverTarget;

                player->roll =
                    player->rollManeuverDirection *
                    maneuverOvershoot;

                player->rollManeuverActive = false;
                player->rollManeuverTimer = 0.0f;
            }
        }

        return;
    }

    if (fabsf(normalizedLateral) >= maneuverThreshold)
    {
        player->rollManeuverTimer += deltaTime;

        if (player->rollManeuverTimer >= maneuverDelay)
        {
            player->rollManeuverActive = true;

            player->rollManeuverProgress = 0.0f;

            player->rollManeuverDirection =
                normalizedLateral > 0.0f
                    ? 1.0f
                    : -1.0f;

            player->rollManeuverTarget =
                2.0f * PI -
                fabsf(player->roll);

            player->rollManeuverTarget +=
                maneuverOvershoot;
        }
    }
    else
    {
        player->rollManeuverTimer = 0.0f;
    }
}

void DrawPlayer(Player *player)
{
    if (player == NULL)
        return;

    Vector3 forward = Vector3Normalize(player->forward);

    Vector3 worldUp = (Vector3){ 0.0f, 1.0f, 0.0f };

    Vector3 right = Vector3Normalize(
        Vector3CrossProduct(worldUp, forward)
    );

    if (Vector3LengthSqr(right) < 0.0001f)
    {
        worldUp = (Vector3){ 0.0f, 0.0f, 1.0f };

        right = Vector3Normalize(
            Vector3CrossProduct(worldUp, forward)
        );
    }

    Vector3 up = Vector3Normalize(
        Vector3CrossProduct(forward, right)
    );

    float cosRoll = cosf(player->roll);
    float sinRoll = sinf(player->roll);

    Vector3 rolledRight =
        Vector3Add(
            Vector3Scale(right, cosRoll),
            Vector3Scale(up, sinRoll)
        );

    Vector3 rolledUp =
        Vector3Subtract(
            Vector3Scale(up, cosRoll),
            Vector3Scale(right, sinRoll)
        );

    Matrix rotation = {
        rolledRight.x,
        rolledUp.x,
        -forward.x,
        0.0f,

        rolledRight.y,
        rolledUp.y,
        -forward.y,
        0.0f,

        rolledRight.z,
        rolledUp.z,
        -forward.z,
        0.0f,

        0.0f,
        0.0f,
        0.0f,
        1.0f
    };

    player->model.transform = rotation;

    DrawModel(
        player->model,
        player->position,
        0.01f,
        WHITE
    );
}

void DamagePlayer(
    Player *player,
    float damage
)
{
    if (player == NULL)
        return;

    if (damage <= 0.0f)
        return;

    if (player->health <= 0)
        return;

    player->health -=
        (int)ceilf(damage);

    if (player->health < 0)
        player->health = 0;
}


void UnloadPlayer(Player *player)
{
    UnloadModel(player->model);
}

void DrawShipTrails(
    Player *player,
    Vector3 forward,
    Vector3 right,
    Vector3 up,
    Camera3D camera
)
{
    (void)forward;
    (void)right;

    if (player == NULL)
        return;

    DrawTrail(
        &player->trails[0],
        &player->trailConfig,
        up,
        camera
    );

    DrawTrail(
        &player->trails[1],
        &player->trailConfig,
        up,
        camera
    );
}

void UpdateShipTrails(
    Player *player,
    Vector3 forward,
    Vector3 right,
    Vector3 up,
    float deltaTime
)
{
    if (player == NULL)
        return;

    TrailConfig *config =
        &player->trailConfig;

    float t =
        1.0f - expf(
            -config->followSpeed * deltaTime
        );

    player->trailForward =
        Vector3Normalize(
            Vector3Lerp(
                player->trailForward,
                forward,
                t
            )
        );

    player->trailRight =
        Vector3Normalize(
            Vector3Lerp(
                player->trailRight,
                right,
                t
            )
        );

    player->trailUp =
        Vector3Normalize(
            Vector3Lerp(
                player->trailUp,
                up,
                t
            )
        );

    Vector3 rear =
        Vector3Subtract(
            player->position,
            Vector3Scale(
                player->trailForward,
                config->exhaustDistance
            )
        );

    Vector3 leftExhaust =
        Vector3Add(
            rear,
            Vector3Scale(
                player->trailRight,
                -config->exhaustSeparation
            )
        );

    Vector3 rightExhaust =
        Vector3Add(
            rear,
            Vector3Scale(
                player->trailRight,
                config->exhaustSeparation
            )
        );

    UpdateTrail(
        &player->trails[0],
        leftExhaust,
        deltaTime,
        config
    );

    UpdateTrail(
        &player->trails[1],
        rightExhaust,
        deltaTime,
        config
    );
}

#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "trail.h"
#include "planet.h"

#define PLAYER_MIN_DISTANCE_FROM_PLANET 20.0f

typedef struct {
    Model model;

    Vector3 position;
    Vector3 forward;
    Vector3 velocity;

    float speed;
    float acceleration;
    float deceleration;
    float turnResponsiveness;

    float roll;
    float targetRoll;

    float fireCooldown;
    float fireTimer;
    float damage;
    float xpMultiplier;

    bool rollManeuverActive;
    float rollManeuverProgress;
    float rollManeuverDirection;
    float rollManeuverTimer;
    float rollManeuverTarget;

    int health;
    int maxHealth;
    float collisionRadius;

    int level;
    int currentXP;
    int requiredXP;
    int pendingLevelUps;

    Trail trails[2];
    TrailConfig trailConfig;

    Vector3 trailForward;
    Vector3 trailRight;
    Vector3 trailUp;

} Player;

Player CreatePlayer();

void UpdatePlayer(
    Player *player,
    Planet *planet,
    Vector3 forward,
    Vector3 right,
    Vector3 up
);

void UpdatePlayerManeuver(
    Player *player,
    float normalizedLateral,
    float deltaTime
);

void DrawPlayer(Player *player);

void UpdateShipTrails(
    Player *player,
    Vector3 forward,
    Vector3 right,
    Vector3 up,
    float deltaTime
);

void DrawShipTrails(
    Player *player,
    Vector3 forward,
    Vector3 right,
    Vector3 up,
    Camera3D camera
);

// static float SafeLength(Vector3 vector);

// static void ResolvePlanetCollision(Player *player, const Planet *planet);

void DamagePlayer(
    Player *player,
    float damage
);

int PlayerRequiredXPForLevel(int level);

void PlayerAddXP(Player *, int);

void UnloadPlayer(Player *player);

#endif

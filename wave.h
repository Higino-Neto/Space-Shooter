#ifndef WAVE_H
#define WAVE_H
#include "enemy.h"
#include "raylib.h"

#define MAX_WAVE_ENEMIES 48
#define MAX_ENEMY_GROUP_QUANTITY 12

// typedef struct Portal {
//     Model model;
//     Color color;
//     float radius;
//     Vector3 position;
//     Vector3 forward;
// } Portal;

typedef struct EnemyGroup {
    Enemy model;
    int enemyCount;    
} EnemyGroup;


typedef struct EnemyWave {
    EnemyGroup enemyGroup[MAX_ENEMY_GROUP_QUANTITY]; 
    
    int enemyGroupCount;
    
    // Portal portal;
    
    bool active;
} EnemyWave;

// void DrawPortal(Portal portal);

Vector3 GetRandomSpawnPosition(float distanceFromPlanet);

EnemyWave CreateWave(
    Planet planet,
    const EnemyAssets *assets,
    int waveNumber
);

float GetWaveCooldown(int waveNumber);

// Portal CreatePortal();

void UpdateWave(EnemyWave *enemyWave);

void DrawWave(EnemyWave *enemyWave);


void UnloadEnemyWave(EnemyWave *enemyWave);

#endif

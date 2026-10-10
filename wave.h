#ifndef WAVE_H
#define WAVE_H
#include "enemy.h"
#include "raylib.h"

#define MAX_ENEMY_QUANTITY 20
#define MAX_ENEMY_GROUP_QUANTITY 5

typedef struct Portal {
    Model model;
    Color color;
    float radius;
    Vector3 position;
    Vector3 forward;
} Portal;


typedef struct EnemyGroup {
    Enemy model;
    int enemyCount;    
} EnemyGroup;

typedef struct EnemyWave {
    EnemyGroup enemyGroup[MAX_ENEMY_GROUP_QUANTITY]; 
    
    int enemyGroupCount;
    
    Portal portal;
    
    bool active;
} EnemyWave;

void DrawPortal(Portal portal);

EnemyWave CreateWave(Planet planet);

Portal CreatePortal();

void UpdateWave(EnemyWave *enemyWave);

void DrawWave(EnemyWave *enemyWave);


void UnloadEnemyWave(EnemyWave *enemyWave);

#endif

#include "wave.h"
#include "enemy.h"
#include <stdio.h>

EnemyWave CreateWave(Planet planet) {
   EnemyWave wave = { 0 };

   wave.enemyGroup[0].enemy[0] = CreateEnemy(
        (Vector3) {
            800.0f,
            800.0f,
            800.0f
        },
        &planet
    );
   
   wave.enemyGroup[0].enemy[1] = CreateEnemy(
      (Vector3) {
         500.0f,
         250.0f,
         500.0f
      },
      &planet
   );

   wave.enemyGroup[0].enemy[2] = CreateEnemy(
      (Vector3) {
         500.0f,
         250.0f,
         500.0f
      },
      &planet
   );

   wave.enemyGroup[0].enemy[3] = CreateEnemy(
      (Vector3) {
         500.0f,
         250.0f,
         500.0f
      },
      &planet
   );

   wave.enemyGroup[0].enemy[4] = CreateEnemy(
      (Vector3) {
         500.0f,
         250.0f,
         500.0f
      },
      &planet
   );

   wave.enemyGroup[0].enemyCount = 5;
   wave.enemyGroupCount = 1;

   
   wave.portal = CreatePortal();
   wave.active = true;

   printf("Enemy groups: %d\n", wave.enemyGroupCount);
   printf("Enemies: %d\n", wave.enemyGroup[0].enemyCount);

   return wave;
}

void UpdateWave(EnemyWave *enemyWave) {
   
}

void DrawWave(EnemyWave *enemyWave) {
   for(int groupIndex = 0; groupIndex < enemyWave->enemyGroupCount; groupIndex++) {
      EnemyGroup *group = &enemyWave->enemyGroup[groupIndex];

      Portal portal = CreatePortal();

      // DrawPortal(portal);

      for(int enemyIndex = 0; enemyIndex < group->enemyCount; enemyIndex++) {
         Enemy *enemy = &group->enemy[enemyIndex];
         DrawEnemy(enemy);
      }
   }
}

Portal CreatePortal() {
   Portal portal = { 0 };
   portal.color = BLUE;
   portal.position = (Vector3) { 300.0f, 300.0f, 300.f };
   portal.forward = (Vector3) { 0.0f, 0.0f, 0.0f };
   portal.radius = 10.0f;

   return portal;
}
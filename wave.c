#include "wave.h"
#include "enemy.h"
#include "raymath.h"
#include <stdio.h>


EnemyWave CreateWave(Planet planet, const EnemyAssets *assets) {
   EnemyWave wave = { 0 };
   
   Enemy smallShip = CreateEnemy((Vector3) { 500.0f, 500.0f, 500.0f }, &planet);
   smallShip.collisionRadius = 20.0f;
   smallShip.visualModel = &assets->smallShip;
   smallShip.modelScale = 0.04f;
   smallShip.xpReward = 25;

   Enemy bigShip = CreateEnemy((Vector3) { -500.0f, -500.0f, -500.0f }, &planet);
   bigShip.collisionRadius = 50.0f;
   bigShip.visualModel = &assets->bigShip;
   bigShip.modelScale = 0.04f;
   bigShip.xpReward = 75;

   wave.enemyGroup[0] = (EnemyGroup) {
      .model = smallShip,
      .enemyCount = 5
   };

   wave.enemyGroup[1] = (EnemyGroup) {
      .model = bigShip,
      .enemyCount = 5
   };

   wave.enemyGroupCount = 2;
   
   // wave.portal = CreatePortal();
   wave.active = true;

   printf("Enemy groups: %d\n", wave.enemyGroupCount);
   printf("Enemies: %d\n", wave.enemyGroup[0].enemyCount);

   return wave;
}

void UpdateWave(EnemyWave *enemyWave) {

}

Vector3 GetRandomSpawnPosition(float distanceFromPlanet) {
   Vector3 planetCenter = {
      0.0f,
      0.0f,
      0.0f
   };
   
   Vector3 direction = {
      (float) GetRandomValue(-1000, 1000),
      (float) GetRandomValue(-1000, 1000),
      (float) GetRandomValue(-1000, 1000)
   };

   direction = Vector3Normalize(direction);

   return Vector3Add(
      planetCenter,
      Vector3Scale(direction, distanceFromPlanet)
   );
}

void DrawWave(EnemyWave *enemyWave) {
   for(int groupIndex = 0; groupIndex < enemyWave->enemyGroupCount; groupIndex++) {
      EnemyGroup *group = &enemyWave->enemyGroup[groupIndex];

      // Portal portal = CreatePortal();

      // DrawPortal(portal);

      Enemy *enemy = &group->model;
      for(int enemyIndex = 0; enemyIndex < group->enemyCount; enemyIndex++) {
         DrawEnemy(enemy);
      }
   }
}

// Portal CreatePortal() {
//    Portal portal = { 0 };
//    portal.color = BLUE;
//    portal.position = (Vector3) { 300.0f, 300.0f, 300.f };
//    portal.forward = (Vector3) { 0.0f, 0.0f, 0.0f };
//    portal.radius = 10.0f;

//    return portal;
// }
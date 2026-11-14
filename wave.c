#include "wave.h"
#include "enemy.h"
#include "raymath.h"
#include <stddef.h>

static int WaveEnemyTotal(const EnemyWave *wave)
{
    int total = 0;

    for (int i = 0; i < wave->enemyGroupCount; i++)
        total += wave->enemyGroup[i].enemyCount;

    return total;
}

static void WaveAddGroup(
    EnemyWave *wave,
    EnemyType type,
    int count,
    int waveNumber,
    const Planet *planet,
    const EnemyAssets *assets
)
{
    if (count <= 0)
        return;

    if (wave->enemyGroupCount >= MAX_ENEMY_GROUP_QUANTITY)
        return;

    int total = WaveEnemyTotal(wave);

    if (total >= MAX_WAVE_ENEMIES)
        return;

    if (total + count > MAX_WAVE_ENEMIES)
        count = MAX_WAVE_ENEMIES - total;

    Enemy enemy = CreateEnemyType(
        type,
        (Vector3){ 0.0f, 0.0f, 0.0f },
        planet
    );

    if (assets != NULL)
        enemy.visualModel = &assets->ships[type];

    ScaleEnemyForWave(&enemy, waveNumber);

    EnemyGroup *group =
        &wave->enemyGroup[wave->enemyGroupCount++];

    group->model = enemy;
    group->enemyCount = count;
}

EnemyWave CreateWave(
    Planet planet,
    const EnemyAssets *assets,
    int waveNumber
) {
    EnemyWave wave = { 0 };

    if (waveNumber < 1)
        waveNumber = 1;

    WaveAddGroup(
        &wave,
        ENEMY_INTERCEPTOR,
        5 + (waveNumber - 1) / 2,
        waveNumber,
        &planet,
        assets
    );

    WaveAddGroup(
        &wave,
        ENEMY_FIGHTER,
        2 + (waveNumber - 1) / 3,
        waveNumber,
        &planet,
        assets
    );

    if (waveNumber >= 2)
    {
        WaveAddGroup(
            &wave,
            ENEMY_RAMMER,
            2 + (waveNumber - 2) / 4,
            waveNumber,
            &planet,
            assets
        );
    }

    if (waveNumber >= 3)
    {
        WaveAddGroup(
            &wave,
            ENEMY_BOMBER,
            1 + (waveNumber - 3) / 5,
            waveNumber,
            &planet,
            assets
        );
    }

    if (waveNumber >= 4)
    {
        WaveAddGroup(
            &wave,
            ENEMY_SNIPER,
            1 + (waveNumber - 4) / 6,
            waveNumber,
            &planet,
            assets
        );
    }

    if (waveNumber >= 5)
    {
        WaveAddGroup(
            &wave,
            ENEMY_SUPPORT,
            1 + (waveNumber - 5) / 8,
            waveNumber,
            &planet,
            assets
        );
    }

    if (waveNumber >= 6)
    {
        WaveAddGroup(
            &wave,
            ENEMY_SHIELD,
            1 + (waveNumber - 6) / 6,
            waveNumber,
            &planet,
            assets
        );
    }

    if (waveNumber >= 7)
    {
        WaveAddGroup(
            &wave,
            ENEMY_SPLITTER,
            1 + (waveNumber - 7) / 7,
            waveNumber,
            &planet,
            assets
        );
    }

    if (waveNumber >= 8)
    {
        WaveAddGroup(
            &wave,
            ENEMY_PHANTOM,
            1 + (waveNumber - 8) / 8,
            waveNumber,
            &planet,
            assets
        );
    }

    if (waveNumber % 5 == 0)
    {
        WaveAddGroup(
            &wave,
            ENEMY_DREADNOUGHT,
            waveNumber / 5,
            waveNumber,
            &planet,
            assets
        );
    }

    wave.active = true;

    return wave;
}

float GetWaveCooldown(int waveNumber) {
    if (waveNumber < 1)
        waveNumber = 1;

    float cooldown =
        4.5f - 0.06f * (float)(waveNumber - 1);

    if (cooldown < 2.5f)
        cooldown = 2.5f;

    if (waveNumber % 5 == 0)
        cooldown += 1.5f;

    return cooldown;
}

void UpdateWave(EnemyWave *enemyWave) {

}

Vector3 GetRandomSpawnPosition(float distanceFromPlanet) {
   Vector3 direction = {
      (float) GetRandomValue(-1000, 1000),
      (float) GetRandomValue(-1000, 1000),
      (float) GetRandomValue(-1000, 1000)
   };

   if (Vector3LengthSqr(direction) < 0.001f)
      direction = (Vector3){ 1.0f, 0.0f, 0.0f };

   direction = Vector3Normalize(direction);

   return Vector3Scale(direction, distanceFromPlanet);
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

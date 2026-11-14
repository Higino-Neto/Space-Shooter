
#include "raylib.h"
#include "camera.h"
#include "player.h"
#include "planet.h"
#include "world.h"
#include "skybox.h"
#include "wave.h"
#include "ui/ui.h"
#include "ui/levelup.h"
#include "ui/crosshair.h"
#include "ui/targeting.h"

#include <stdio.h>

#include "raymath.h"
#include "rlgl.h"

static const int screenWidth = 1800;
static const int screenHeight = 1000;

int main(void)
{
    InitWindow(screenWidth, screenHeight, "Space Shooter");

    UIInit();
    CrosshairInit();
    Crosshair crosshair = CreateCrosshair();
    LevelUpInit();

    SetTargetFPS(60);
    DisableCursor();

    EnemyAssets enemyAssets = LoadEnemyAssets();

    World world = CreateWorld();

    Skybox skybox = LoadSkybox(
        "assets/background/ablaze/cubemap.png");

    Player player = CreatePlayer();
    player.position.x = 1200;

    Camera3D camera = CreateCamera(player);
    CameraController controller = CreateCameraController();

    Shader planetShader = LoadShader(
        "assets/shaders/planet.vs",
        "assets/shaders/planet_professional.frag");

    int planetPositionLoc = GetShaderLocation(
        planetShader,
        "planetPosition");

    int planetRadiusLoc = GetShaderLocation(
        planetShader,
        "planetRadius");

    int cameraPositionLoc = GetShaderLocation(
        planetShader,
        "cameraPosition");

    int deformationCountLoc = GetShaderLocation(
        planetShader,
        "deformationCount");

    int deformationDataLoc = GetShaderLocation(
        planetShader,
        "deformationData");

    Material material = LoadMaterialDefault();
    material.shader = planetShader;

    Planet planet = CreatePlanet(
        1000.0f,
        64,
        2.3451f,
        30.0f,
        material);

    WorldAddPlayer(&world, player);
    WorldAddPlanet(&world, planet);

    LevelUpMenu levelMenu = {0};

    while (!WindowShouldClose())
    {
        if (!levelMenu.active &&
            world.player.pendingLevelUps > 0)
        {
            OpenLevelUpMenu(&levelMenu);
        }

        if (levelMenu.active)
        {
            if(IsCursorHidden()) {
                ShowCursor();
                SetMousePosition(40, 40);
            }

            UpgradeLevelUpMenu(
                &levelMenu,
                &world.player);
        }
        else
        {
            // DisableCursor();

            if (!IsCursorHidden()) {
                DisableCursor();
            }

            // CrosshairUpdate(&crosshair);

            UpdateWorld(
                &world,
                &controller,
                &camera,
                &crosshair,
                &enemyAssets);
        }

        BeginDrawing();

        ClearBackground(BLACK);

        rlSetClipPlanes(0.05f, 10000.0f);

        BeginMode3D(camera);

        DrawSkybox(&skybox, camera);

        Planet *activePlanet = &world.planets[0];

        SetShaderValue(
            planetShader,
            planetPositionLoc,
            &activePlanet->position,
            SHADER_UNIFORM_VEC3);

        SetShaderValue(
            planetShader,
            planetRadiusLoc,
            &activePlanet->radius,
            SHADER_UNIFORM_FLOAT);

        SetShaderValue(
            planetShader,
            cameraPositionLoc,
            &camera.position,
            SHADER_UNIFORM_VEC3);

        SetShaderValue(
            planetShader,
            deformationCountLoc,
            &activePlanet->deformationCount,
            SHADER_UNIFORM_INT);

        // const float aimDistance = 90.0f;
        // const float aimVerticalOffset = 27.0f;

        // Vector3 aimPosition = Vector3Add(
        //     world.player.position,
        //     Vector3Scale(
        //         world.player.forward,
        //         aimDistance));

        // aimPosition.y += aimVerticalOffset;

        // DrawSphere(
        //     aimPosition,
        //     0.8f,
        //     GREEN);

        DrawWorld(&world, camera);

        EndMode3D();

        UIDrawHUD(
            world.player.health,
            world.player.maxHealth,
            world.planets[0].health,
            world.planets[0].maxHealth,
            world.player.level,
            world.player.currentXP,
            world.player.requiredXP);

        UIDrawWaveBanner(
            world.currentWave,
            world.waveBannerTimer);

        DrawTargeting(&world.targeting);

        CrosshairDraw(&crosshair);

        if (levelMenu.active)
        {
            DrawLevelUpMenu(&levelMenu);
        }

        EndDrawing();
    }

    UnloadSkybox(&skybox);
    UnloadPlayer(&world.player);
    UnloadEnemyAssets(&enemyAssets);
    UnloadShader(planetShader);

    UIShutdown();
    LevelUpShutdown();

    CloseWindow();

    return 0;
}

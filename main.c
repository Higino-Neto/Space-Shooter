#include "raylib.h"
#include "camera.h"
#include "player.h"
#include "planet.h"
#include "world.h"
#include "skybox.h"
#include "wave.h"
#include <stdio.h>

#include "raymath.h"
#include "rlgl.h"

static const int screenWidth = 1800;
static const int screenHeight = 1000;

int main(void) {
    InitWindow(screenWidth, screenHeight, "Space Shooter");
    SetTargetFPS(60);

    World world = CreateWorld();

    Skybox skybox = LoadSkybox(
        "assets/background/blink/cubemap.png"
    );

    Player player = CreatePlayer();
    player.position.x = 1200;
    Camera3D camera = CreateCamera(player);
    CameraController controller = CreateCameraController();

    Shader planetShader = LoadShader(
         "assets/shaders/planet.vs",
         "assets/shaders/planet.fs"
     );



    int planetPositionLoc = GetShaderLocation(
        planetShader,
        "planetPosition"
    );

    int planetRadiusLoc = GetShaderLocation(
        planetShader,
        "planetRadius"
    );

    int cameraPositionLoc =
        GetShaderLocation(
            planetShader,
            "cameraPosition"
        );

    int lightDirectionLoc =
        GetShaderLocation(
            planetShader,
            "lightDirection"
        );

    int deformationCountLoc =
    GetShaderLocation(
        planetShader,
        "deformationCount"
    );

    int deformationDataLoc =
        GetShaderLocation(
            planetShader,
            "deformationData"
        );

    
    Material material = LoadMaterialDefault();
    material.shader = planetShader;

    Planet planet = CreatePlanet(
        1000.0f,
        62,
        2.3451f,
        30.0f,
        material
    );

    Portal portal = CreatePortal();

    EnemyWave wave = CreateWave(world.planets[0]);


    WorldAddPlayer(&world, player);
    WorldAddPlanet(&world, planet);
    WorldAddWave(&world, wave);


    while(!WindowShouldClose()) {

        UpdateWorld(&world, &controller, &camera);

        BeginDrawing();
            ClearBackground(BLACK);

            rlSetClipPlanes(0.05f, 10000.0f);

            BeginMode3D(camera);
                DrawSkybox(&skybox, camera);

                SetShaderValue(
                    planetShader,
                    planetPositionLoc,
                    &planet.position,
                    SHADER_UNIFORM_VEC3
                );

                SetShaderValue(
                    planetShader,
                    planetRadiusLoc,
                    &planet.radius,
                    SHADER_UNIFORM_FLOAT
                );

                Vector3 lightDirection =
                    Vector3Normalize(
                        (Vector3){
                            1.0f,
                            0.8f,
                            0.45f
                        }
                    );

                SetShaderValue(
                    planetShader,
                    lightDirectionLoc,
                    &lightDirection,
                    SHADER_UNIFORM_VEC3
                );

                SetShaderValue(
                    planetShader,
                    cameraPositionLoc,
                    &camera.position,
                    SHADER_UNIFORM_VEC3
                );

                const float aimDistance = 90.0f;
                const float aimVerticalOffset = 27.0f;

                Vector3 aimPosition =
                    Vector3Add(
                        world.player.position,
                        Vector3Scale(
                            world.player.forward,
                            aimDistance
                        )
                    );

                aimPosition.y += aimVerticalOffset;

                DrawSphere(
                    aimPosition,
                    0.8f,
                    GREEN
                );

                DrawWorld(&world, camera);

            EndMode3D();

            DrawFPS(10, 10);
            DrawText(TextFormat("Life: %d", world.player.health), 10, 40, 20, RED);
            DrawText(TextFormat("X: %.2f", world.player.position.x), 10, 80, 20, RED);
            DrawText(TextFormat("Y: %.2f", world.player.position.y), 10, 120, 20, RED);
            DrawText(TextFormat("Z: %.2f", world.player.position.z), 10, 160, 20, RED);
            DrawText(TextFormat("Planet Damage: %d", world.planets[0].deformationCount), 10, 200, 20, RED);
            DrawText(TextFormat("Enemies: %d", world.enemyCount), 10, 240, 20, RED);

        EndDrawing();
    }

    
    UnloadSkybox(&skybox);
    UnloadPlayer(&world.player);
    CloseWindow();

    return 0;
}

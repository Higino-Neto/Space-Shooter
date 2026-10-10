#include "skybox.h"
#include "rlgl.h"
#include "raymath.h"
#include <stdlib.h>

Skybox LoadSkybox(const char *cubemapPath)
{
    Skybox skybox = { 0 };

    Image image = LoadImage(cubemapPath);

    skybox.cubemap = LoadTextureCubemap(
        image,
        CUBEMAP_LAYOUT_CROSS_FOUR_BY_THREE
    );

    UnloadImage(image);

    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);

    skybox.model = LoadModelFromMesh(cube);

    skybox.shader = LoadShader(
        "assets/shaders/skybox.vs",
        "assets/shaders/skybox.fs"
    );

    skybox.model.materials[0].shader = skybox.shader;

    int environmentMapLoc = GetShaderLocation(
        skybox.shader,
        "environmentMap"
    );

    int mapType = MATERIAL_MAP_CUBEMAP;

    SetShaderValue(
        skybox.shader,
        environmentMapLoc,
        &mapType,
        SHADER_UNIFORM_INT
    );

    skybox.model.materials[0]
        .maps[MATERIAL_MAP_CUBEMAP]
        .texture = skybox.cubemap;

    return skybox;
}

void DrawSkybox(
    Skybox *skybox,
    Camera3D camera
)
{
    if (skybox == NULL)
        return;

    rlDisableBackfaceCulling();
    rlDisableDepthMask();

    skybox->model.transform = MatrixTranslate(
        camera.position.x,
        camera.position.y,
        camera.position.z
    );

    DrawModel(
        skybox->model,
        (Vector3){ 0.0f, 0.0f, 0.0f },
        1.0f,
        WHITE
    );

    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void UnloadSkybox(Skybox *skybox)
{
    if (skybox == NULL)
        return;

    UnloadShader(skybox->shader);
    UnloadTexture(skybox->cubemap);
    UnloadModel(skybox->model);
}
#ifndef SKYBOX_H
#define SKYBOX_H

#include "raylib.h"

typedef struct Skybox {
    Model model;
    TextureCubemap cubemap;
    Shader shader;
} Skybox;

Skybox LoadSkybox(const char *cubemapPath);

void DrawSkybox(
    Skybox *skybox,
    Camera3D camera
);

void UnloadSkybox(
    Skybox *skybox
);

#endif
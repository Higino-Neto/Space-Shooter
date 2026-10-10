#ifndef CAMERA_H
#define CAMERA_H

#include "raylib.h"
#include "player.h"

typedef struct {
    float yaw;
    float pitch;
    float sensitivity;
} CameraController;

typedef struct {
    float baseFov;
    float currentFov;

    float shakeStrength;
    float shakeTimer;

    float roll;
    float targetRoll;

    Vector3 position;
    Vector3 target;
} GameCamera;

Camera3D CreateCamera(Player player);

GameCamera CreateGameCamera();

CameraController CreateCameraController();

void UpdateGameCamera(
    GameCamera *gameCamera,
    Camera3D *camera,
    CameraController *controller,
    const Player *player,
    float deltaTime
);

void UpdateCameraController(
    CameraController *controller
);

Vector3 GetForwardDirection(
    CameraController *controller
);

Vector3 GetHorizontalForwardDirection(
    CameraController *controller
);

#endif
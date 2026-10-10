#include "camera.h"
#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>

Camera3D CreateCamera(Player player)
{
    Camera3D camera = { 0 };

    camera.position = (Vector3){
        0.0f,
        50.0f,
        300.0f
    };

    camera.target = player.position;

    camera.up = (Vector3){
        0.0f,
        1.0f,
        0.0f
    };

    camera.fovy = 110.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    return camera;
}

GameCamera CreateGameCamera()
{
    GameCamera camera = { 0 };

    camera.baseFov = 60.0f;
    camera.currentFov = camera.baseFov;

    camera.shakeStrength = 0.0f;
    camera.shakeTimer = 0.0f;

    camera.roll = 0.0f;
    camera.targetRoll = 0.0f;

    camera.position = (Vector3){ 0.0f };
    camera.target = (Vector3){ 0.0f };

    return camera;
}

CameraController CreateCameraController() {
    CameraController controller = {
        .pitch = 0.0f,
        .yaw = 0.0f,
        .sensitivity = 0.005f
    };

    return controller;
}

Vector3 GetForwardDirection(CameraController *controller) {
    

    float deltaX = sinf(controller->yaw);
    float deltaY = sinf(controller->pitch);
    float deltaZ = -cosf(controller->yaw);

    Vector3 forward = {
        deltaX,
        deltaY,
        deltaZ
    };

    return forward;
}

Vector3 GetHorizontalForwardDirection(CameraController *controller) {
    float yaw = controller->yaw;

    Vector3 horizontalForward = {
        sinf(yaw),
        0.0f,
        -cosf(yaw)
    };

    return horizontalForward;
}

void UpdateCameraController(CameraController *controller) {
    Vector2 mouseDelta = GetMouseDelta();
    controller->yaw += mouseDelta.x * controller->sensitivity;
    controller->pitch -= mouseDelta.y * controller->sensitivity;

    if (controller->pitch > PI / 2.0f)
        controller->pitch = PI / 2.0f;

    if (controller->pitch < -PI / 2.0f)
        controller->pitch = -PI / 2.0f;
}

void UpdateGameCamera(
    GameCamera *gameCamera,
    Camera3D *camera,
    CameraController *controller,
    const Player *player,
    float deltaTime
)
{
    if (gameCamera == NULL ||
        camera == NULL ||
        controller == NULL ||
        player == NULL)
    {
        return;
    }

    Vector3 forward =
        GetForwardDirection(controller);

    float playerSpeed =
        Vector3Length(player->velocity);

    float speedRatio =
        Clamp(
            playerSpeed / player->speed,
            0.0f,
            1.0f
        );

    float targetFov =
        gameCamera->baseFov +
        speedRatio * 38.0f;

    float fovT =
        1.0f - expf(
            -6.0f * deltaTime
        );

    gameCamera->currentFov =
        Lerp(
            gameCamera->currentFov,
            targetFov,
            fovT
        );

    camera->fovy =
        gameCamera->currentFov;

    const float distance = 100.0f;
    const float height = 30.0f;

    Vector3 desiredPosition =
        Vector3Add(
            player->position,
            Vector3Scale(
                forward,
                -distance
            )
        );

    desiredPosition.y += height;

    float positionT =
        1.0f - expf(
            -8.0f * deltaTime
        );

    gameCamera->position =
        Vector3Lerp(
            gameCamera->position,
            desiredPosition,
            positionT
        );

    float targetT =
        1.0f - expf(
            -12.0f * deltaTime
        );

    gameCamera->target =
        Vector3Lerp(
            gameCamera->target,
            player->position,
            targetT
        );


    camera->position =
        gameCamera->position;

    camera->target =
        gameCamera->target;
}

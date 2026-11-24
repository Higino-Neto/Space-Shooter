#include "crosshair.h"
#include "stdlib.h"
#include "raylib.h"
#include "raymath.h"

#define CROSSHAIR_SIZE 48.0f
#define CROSSHAIR_MARGIN 40.0f

#define CROSSHAIR_SENSITIVITY 0.005f
#define CROSSHAIR_DEAD_ZONE 0.2f
#define CROSSHAIR_STEER_GAIN 8.0f
#define CROSSHAIR_MAX_PITCH 3.8f

static Texture2D idleTexture;
static bool crosshairInitialized = false;

void CrosshairInit(void)
{
    idleTexture = LoadTexture("assets/crosshair/3.png");
    crosshairInitialized = true;
}

Crosshair CreateCrosshair(void)
{
    Crosshair crosshair = {0};

    crosshair.aim = (Vector2){
        GetScreenWidth() / 2.0f,
        GetScreenHeight() / 2.0f};

    crosshair.aimDirection = (Vector3){0.0f, 0.0f, -1.0f};

    return crosshair;
}

void CrosshairUpdateInput(Crosshair *crosshair)
{
    Vector2 delta = GetMouseDelta();

    crosshair->yawOffset += delta.x * CROSSHAIR_SENSITIVITY;
    crosshair->pitchOffset -= delta.y * CROSSHAIR_SENSITIVITY;

    if (crosshair->pitchOffset > CROSSHAIR_MAX_PITCH)
    {
        crosshair->pitchOffset = CROSSHAIR_MAX_PITCH;
    }

    if (crosshair->pitchOffset < -CROSSHAIR_MAX_PITCH)
    {
        crosshair->pitchOffset = -CROSSHAIR_MAX_PITCH;
    }
}

static void CrosshairSteerAxis(
    float *offset,
    float *cameraAngle,
    float deltaTime)
{

    float excess = fabsf(*offset) - CROSSHAIR_DEAD_ZONE;

    if (excess <= 0.0f)
        return;

    float steer = excess * CROSSHAIR_STEER_GAIN * deltaTime;

    if (steer > excess)
        steer = excess;

    float sign = (*offset > 0.0f) ? 1.0f : -1.0f;

    *cameraAngle += sign * steer;
    *offset -= sign * steer;

}

void CrosshairSteerCamera(
    Crosshair *crosshair,
    CameraController *controller,
    float deltaTime
) {
    CrosshairSteerAxis(
        &crosshair->yawOffset,
        &controller->yaw,
        deltaTime
    );

    CrosshairSteerAxis(
        &crosshair->pitchOffset,
        &controller->pitch,
        deltaTime
    );

    if (controller->pitch > PI/2.0f) controller->pitch = PI/2.0f;
    if (controller->pitch < -PI/2.0f) controller->pitch = -PI/2.0f;
}

void CrosshairProject(
    Crosshair *crosshair,
    const Camera3D *camera
) {
    Vector3 forward = Vector3Normalize(
        Vector3Subtract(camera->target, camera->position)
    );

    Vector3 right = Vector3Normalize(
        Vector3CrossProduct(forward, (Vector3) { 0.0f, 1.0f, 0.0f })
    );

    Vector3 yawed = Vector3RotateByAxisAngle(
        forward,
        (Vector3) { 0.0f, -1.0f, 0.0f },
        crosshair->yawOffset
    );

    crosshair->aimDirection = Vector3Normalize(
        Vector3RotateByAxisAngle(
            yawed,
            right,
            crosshair->pitchOffset
        )
    );

    Vector3 aimPoint = Vector3Add(
        camera->position,
        Vector3Scale(crosshair->aimDirection, 1000.0f)
    );

    Vector2 screen = GetWorldToScreen(aimPoint, *camera);

    if (Vector3DotProduct(crosshair->aimDirection, forward) <= 0.0f) 
    {
        screen = (Vector2) {
            (crosshair->yawOffset >= 0.0f) ?
            GetScreenWidth() - CROSSHAIR_MARGIN :
            CROSSHAIR_MARGIN,
            GetScreenHeight() / 2.0f
        };
    }

    screen.x = Clamp(screen.x, CROSSHAIR_MARGIN,
                    GetScreenWidth() - CROSSHAIR_MARGIN);
    screen.y = Clamp(screen.y, CROSSHAIR_MARGIN,
                    GetScreenHeight() - CROSSHAIR_MARGIN);
    
    crosshair->aim = screen;

}

void CrosshairDraw(const Crosshair *crosshair)
{
    if (!crosshairInitialized || crosshair == NULL)
    {
        return;
    }

    Rectangle source = {
        0, 0,
        (float)idleTexture.width,
        (float)idleTexture.height};

    Rectangle destination = {
        crosshair->aim.x,
        crosshair->aim.y,
        CROSSHAIR_SIZE,
        CROSSHAIR_SIZE};

    Vector2 origin = {
        CROSSHAIR_SIZE / 2.0f,
        CROSSHAIR_SIZE / 2.0f};

    DrawTexturePro(idleTexture, source, destination, 
        (Vector2) { CROSSHAIR_SIZE / 2.0f, CROSSHAIR_SIZE / 2.0f }
        , 0.0f, (Color) { 35, 215, 255, 100 });
}

void CrosshairShutdown(void)
{
    if (!crosshairInitialized)
    {
        return;
    }

    UnloadTexture(idleTexture);
    crosshairInitialized = false;
}
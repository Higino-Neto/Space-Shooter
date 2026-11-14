#include "targeting.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>

#define TARGETING_ACQUIRE_RADIUS 150.0f
#define TARGETING_RELEASE_RADIUS 230.0f
#define TARGETING_SWITCH_MARGIN 45.0f
#define TARGETING_LOCK_KEEP_DISTANCE 120.0f
#define TARGETING_LOCK_DELAY 0.18f

#define TARGETING_AIM_ASSIST_RADIUS 120.0f
#define TARGETING_AIM_ASSIST_PASSIVE 0.10f
#define TARGETING_AIM_ASSIST_FIRING 0.34f

#define TARGETING_CAMERA_ASSIST_RATE_PASSIVE 0.0f
#define TARGETING_CAMERA_ASSIST_RATE_FIRING 1.4f
#define TARGETING_CAMERA_ASSIST_INPUT_DEADZONE 2.0f

#define TARGETING_SHOT_CONE 0.985f
#define TARGETING_SHOT_LEAD 0.75f

#define TARGETING_BOX_MIN 24.0f
#define TARGETING_BOX_MAX 240.0f
#define TARGETING_BOX_PADDING 1.75f
#define TARGETING_BRACKET_RATIO 0.30f

#define TARGETING_OFFSCREEN_MARGIN 46.0f
#define TARGETING_OFFSCREEN_MERGE 72.0f
#define TARGETING_OFFSCREEN_RANGE 6000.0f

#define TARGETING_FADE_SPEED 9.0f
#define TARGETING_SCREEN_PADDING 10.0f
#define TARGETING_PANEL_WIDTH 158.0f
#define TARGETING_PANEL_HEIGHT 66.0f
#define TARGETING_PANEL_MARGIN 12.0f

static const Color TARGETING_BLUE = { 54, 197, 232, 255 };
static const Color TARGETING_GRAY = { 17, 29, 43, 255 };
static const Color TARGETING_RED = { 230, 41, 55, 255 };

static Color TargetingTint(Color color, unsigned char alpha)
{
    color.a = alpha;

    return color;
}

static float TargetingClamp(float value, float min, float max)
{
    if (value < min)
        return min;

    if (value > max)
        return max;

    return value;
}

static float AngleDelta(float target, float current)
{
    return atan2f(
        sinf(target - current),
        cosf(target - current)
    );
}

static void GetCameraBasis(
    const Camera3D *camera,
    Vector3 *forward,
    Vector3 *right,
    Vector3 *up
)
{
    *forward = Vector3Normalize(
        Vector3Subtract(camera->target, camera->position)
    );

    *right = Vector3CrossProduct(*forward, camera->up);

    if (Vector3LengthSqr(*right) < 0.0001f)
    {
        *right = (Vector3){ 1.0f, 0.0f, 0.0f };
    }

    *right = Vector3Normalize(*right);

    *up = Vector3Normalize(
        Vector3CrossProduct(*right, *forward)
    );
}

static const char *TargetingStateLabel(EnemyState state)
{
    switch (state)
    {
        case ENEMY_PATROL:
            return "PATROL";

        case ENEMY_ATTACK_PLANET:
            return "ASSAULT";

        case ENEMY_COMBAT:
            return "COMBAT";

        case ENEMY_RETREAT:
            return "RETREAT";

        default:
            return "";
    }
}

static Color TargetingStateColor(
    EnemyState state,
    unsigned char alpha
)
{
    if (state == ENEMY_COMBAT)
        return TargetingTint(TARGETING_RED, alpha);

    return TargetingTint(TARGETING_BLUE, alpha);
}

static Color TargetingHealthColor(
    float ratio,
    unsigned char alpha
)
{
    if (ratio <= 0.3f)
        return TargetingTint(TARGETING_RED, alpha);

    return TargetingTint(TARGETING_BLUE, alpha);
}

static void TargetingFormatDistance(
    char *buffer,
    int size,
    float distance
)
{
    if (distance >= 1000.0f)
    {
        snprintf(
            buffer,
            size,
            "%.1fkm",
            distance / 1000.0f
        );

        return;
    }

    snprintf(
        buffer,
        size,
        "%dm",
        (int)distance
    );
}

static float TargetingBoxSize(
    const Enemy *enemy,
    Vector2 screen,
    const Camera3D *camera,
    Vector3 cameraRight
)
{
    Vector3 edge = Vector3Add(
        enemy->position,
        Vector3Scale(cameraRight, enemy->collisionRadius)
    );

    Vector2 edgeScreen = GetWorldToScreen(edge, *camera);

    float radius = Vector2Distance(edgeScreen, screen);

    return TargetingClamp(
        radius * 2.0f * TARGETING_BOX_PADDING,
        TARGETING_BOX_MIN,
        TARGETING_BOX_MAX
    );
}

static Vector2 TargetingEdgeAnchor(
    Vector2 direction,
    float margin
)
{
    Vector2 center = {
        GetScreenWidth() * 0.5f,
        GetScreenHeight() * 0.5f
    };

    float halfWidth = center.x - margin;
    float halfHeight = center.y - margin;

    float scale = 1000000.0f;

    if (fabsf(direction.x) > 0.0001f)
    {
        scale = fminf(
            scale,
            halfWidth / fabsf(direction.x)
        );
    }

    if (fabsf(direction.y) > 0.0001f)
    {
        scale = fminf(
            scale,
            halfHeight / fabsf(direction.y)
        );
    }

    if (scale > 999999.0f)
        scale = 0.0f;

    return (Vector2){
        center.x + direction.x * scale,
        center.y + direction.y * scale
    };
}

static int TargetingFindMarker(
    const TargetingSystem *system,
    int enemyIndex
)
{
    for (int i = 0;
         i < system->visibleCount;
         i++)
    {
        if (system->visible[i].enemyIndex == enemyIndex)
            return i;
    }

    return -1;
}

static void TargetingSortOffscreen(
    TargetingSystem *system
)
{
    for (int i = 1;
         i < system->offscreenCount;
         i++)
    {
        OffscreenMarker marker = system->offscreen[i];

        int j = i - 1;

        while (j >= 0 &&
               system->offscreen[j].distance > marker.distance)
        {
            system->offscreen[j + 1] = system->offscreen[j];
            j--;
        }

        system->offscreen[j + 1] = marker;
    }
}

static void TargetingAddOffscreen(
    TargetingSystem *system,
    int enemyIndex,
    const Enemy *enemy,
    Vector2 direction,
    float distance
)
{
    Vector2 anchor = TargetingEdgeAnchor(
        direction,
        TARGETING_OFFSCREEN_MARGIN
    );

    float presence = system->presence[enemyIndex];

    for (int i = 0;
         i < system->offscreenCount;
         i++)
    {
        OffscreenMarker *marker = &system->offscreen[i];

        if (Vector2Distance(
                marker->screenPosition,
                anchor) > TARGETING_OFFSCREEN_MERGE)
        {
            continue;
        }

        marker->count++;

        if (distance < marker->distance)
        {
            marker->distance = distance;
            marker->state = enemy->state;
            marker->screenPosition = anchor;
            marker->direction = direction;
            marker->presence = presence;
        }

        return;
    }

    OffscreenMarker marker = {
        .screenPosition = anchor,
        .direction = direction,
        .distance = distance,
        .presence = presence,
        .count = 1,
        .state = enemy->state
    };

    if (system->offscreenCount < MAX_OFFSCREEN_MARKERS)
    {
        system->offscreen[system->offscreenCount++] = marker;

        return;
    }

    int farthest = 0;

    for (int i = 1;
         i < system->offscreenCount;
         i++)
    {
        if (system->offscreen[i].distance >
            system->offscreen[farthest].distance)
        {
            farthest = i;
        }
    }

    if (distance < system->offscreen[farthest].distance)
        system->offscreen[farthest] = marker;
}

TargetingSystem CreateTargetingSystem(void)
{
    TargetingSystem system = { 0 };

    system.selectedIndex = -1;

    return system;
}

void UpdateTargeting(
    TargetingSystem *system,
    const Enemy *enemies,
    int enemyCount,
    Vector3 shipPosition,
    const Camera3D *camera,
    CameraController *controller,
    Crosshair *crosshair,
    float deltaTime
)
{
    if (system == NULL ||
        enemies == NULL ||
        camera == NULL ||
        controller == NULL ||
        crosshair == NULL)
    {
        return;
    }

    system->visibleCount = 0;
    system->offscreenCount = 0;

    Vector3 forward;
    Vector3 right;
    Vector3 up;

    GetCameraBasis(
        camera,
        &forward,
        &right,
        &up
    );

    float fadeT =
        1.0f - expf(
            -TARGETING_FADE_SPEED * deltaTime);

    bool firing =
        IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int limit = enemyCount;

    if (limit > MAX_TARGET_MARKERS)
        limit = MAX_TARGET_MARKERS;

    for (int i = 0; i < limit; i++)
    {
        const Enemy *enemy = &enemies[i];

        if (!enemy->active)
            continue;

        system->presence[i] +=
            (1.0f - system->presence[i]) * fadeT;

        float distance = Vector3Distance(
            enemy->position,
            shipPosition
        );

        Vector3 toEnemy = Vector3Subtract(
            enemy->position,
            camera->position
        );

        bool inFront =
            Vector3DotProduct(toEnemy, forward) > 0.05f;

        Vector2 screen = GetWorldToScreen(
            enemy->position,
            *camera
        );

        bool onScreen =
            inFront &&
            screen.x >= -TARGETING_SCREEN_PADDING &&
            screen.x <= screenWidth + TARGETING_SCREEN_PADDING &&
            screen.y >= -TARGETING_SCREEN_PADDING &&
            screen.y <= screenHeight + TARGETING_SCREEN_PADDING;

        if (onScreen &&
            system->visibleCount < MAX_TARGET_MARKERS)
        {
            TargetMarker *marker =
                &system->visible[system->visibleCount++];

            marker->enemyIndex = i;
            marker->screenPosition = screen;
            marker->boxSize = TargetingBoxSize(
                enemy,
                screen,
                camera,
                right
            );
            marker->distance = distance;
            marker->health = enemy->health;
            marker->maxHealth = enemy->maxHealth;
            marker->damage = enemy->strength;
            marker->state = enemy->state;
            marker->presence = system->presence[i];
            marker->selected = false;

            continue;
        }

        if (distance > TARGETING_OFFSCREEN_RANGE)
            continue;

        Vector3 directionToEnemy = Vector3Normalize(toEnemy);

        Vector2 direction = {
            Vector3DotProduct(directionToEnemy, right),
            -Vector3DotProduct(directionToEnemy, up)
        };

        if (Vector2LengthSqr(direction) < 0.0001f)
        {
            direction = (Vector2){
                crosshair->yawOffset >= 0.0f ? 1.0f : -1.0f,
                0.0f
            };
        }

        direction = Vector2Normalize(direction);

        TargetingAddOffscreen(
            system,
            i,
            enemy,
            direction,
            distance
        );
    }

    TargetingSortOffscreen(system);

    int candidate = -1;
    float candidateDistance = TARGETING_ACQUIRE_RADIUS;

    for (int i = 0;
         i < system->visibleCount;
         i++)
    {
        float value = Vector2Distance(
            system->visible[i].screenPosition,
            crosshair->aim
        );

        if (value < candidateDistance)
        {
            candidateDistance = value;
            candidate = i;
        }
    }

    int selected = -1;

    if (system->hasSelection)
    {
        selected = TargetingFindMarker(
            system,
            system->selectedIndex
        );

        if (selected >= 0)
        {
            float screenDistance = Vector2Distance(
                system->visible[selected].screenPosition,
                crosshair->aim
            );

            float moved = Vector3Distance(
                system->selectedPosition,
                enemies[system->selectedIndex].position
            );

            if (screenDistance > TARGETING_RELEASE_RADIUS ||
                moved > TARGETING_LOCK_KEEP_DISTANCE)
            {
                selected = -1;
            }
            else if (candidate >= 0 &&
                     candidate != selected &&
                     candidateDistance +
                         TARGETING_SWITCH_MARGIN <
                     screenDistance)
            {
                selected = candidate;
            }
        }
    }

    if (selected < 0)
        selected = candidate;

    if (selected >= 0)
    {
        int enemyIndex = system->visible[selected].enemyIndex;

        if (!system->hasSelection ||
            system->selectedIndex != enemyIndex)
        {
            system->selectionTimer = 0.0f;
        }

        system->hasSelection = true;
        system->selectedIndex = enemyIndex;
        system->selectedPosition = enemies[enemyIndex].position;
        system->selectionTimer += deltaTime;
        system->visible[selected].selected = true;
    }
    else
    {
        system->hasSelection = false;
        system->selectionTimer = 0.0f;
    }

    if (selected < 0)
        return;

    TargetMarker *target = &system->visible[selected];

    float lockRatio = TargetingClamp(
        system->selectionTimer / TARGETING_LOCK_DELAY,
        0.0f,
        1.0f
    );

    float targetDistance = Vector2Distance(
        crosshair->aim,
        target->screenPosition
    );

    float proximity =
        1.0f -
        TargetingClamp(
            targetDistance / TARGETING_AIM_ASSIST_RADIUS,
            0.0f,
            1.0f);

    float aimAssist =
        firing
            ? TARGETING_AIM_ASSIST_FIRING
            : TARGETING_AIM_ASSIST_PASSIVE;

    crosshair->aim = Vector2Lerp(
        crosshair->aim,
        target->screenPosition,
        aimAssist * proximity * lockRatio
    );

    float cameraRate =
        firing
            ? TARGETING_CAMERA_ASSIST_RATE_FIRING
            : TARGETING_CAMERA_ASSIST_RATE_PASSIVE;

    bool steadyAim =
        Vector2Length(GetMouseDelta()) <
        TARGETING_CAMERA_ASSIST_INPUT_DEADZONE;

    if (cameraRate <= 0.0f || !steadyAim)
        return;

    Vector3 toTarget = Vector3Normalize(
        Vector3Subtract(
            enemies[system->selectedIndex].position,
            camera->position
        )
    );

    float targetYaw = atan2f(toTarget.x, -toTarget.z);

    float targetPitch = asinf(
        TargetingClamp(toTarget.y, -1.0f, 1.0f)
    );

    float step = cameraRate * deltaTime * lockRatio;

    controller->yaw += TargetingClamp(
        AngleDelta(targetYaw, controller->yaw),
        -step,
        step
    );

    controller->pitch += TargetingClamp(
        AngleDelta(targetPitch, controller->pitch),
        -step,
        step
    );

    controller->pitch = TargetingClamp(
        controller->pitch,
        -PI / 2.0f,
        PI / 2.0f
    );
}

bool TargetingResolveShot(
    const TargetingSystem *system,
    const Enemy *enemies,
    int enemyCount,
    Vector3 muzzle,
    float projectileSpeed,
    Vector3 *direction
)
{
    if (system == NULL ||
        enemies == NULL ||
        direction == NULL)
    {
        return false;
    }

    if (!system->hasSelection)
        return false;

    if (system->selectedIndex < 0 ||
        system->selectedIndex >= enemyCount)
    {
        return false;
    }

    const Enemy *target =
        &enemies[system->selectedIndex];

    if (!target->active)
        return false;

    if (projectileSpeed <= 0.0f)
        return false;

    Vector3 toTarget = Vector3Subtract(
        target->position,
        muzzle
    );

    float distance = Vector3Length(toTarget);

    if (distance < 0.001f)
        return false;

    Vector3 travel = Vector3Scale(
        target->velocity,
        distance /
            projectileSpeed *
            TARGETING_SHOT_LEAD
    );

    Vector3 predicted = Vector3Add(
        target->position,
        travel
    );

    Vector3 aim = Vector3Normalize(
        Vector3Subtract(predicted, muzzle)
    );

    if (Vector3DotProduct(aim, *direction) <
        TARGETING_SHOT_CONE)
    {
        return false;
    }

    *direction = aim;

    return true;
}

static void TargetingDrawBrackets(
    Vector2 center,
    float size,
    Color color
)
{
    float half = size * 0.5f;
    float arm = size * TARGETING_BRACKET_RATIO;

    Vector2 topLeft = {
        center.x - half,
        center.y - half
    };

    Vector2 topRight = {
        center.x + half,
        center.y - half
    };

    Vector2 bottomLeft = {
        center.x - half,
        center.y + half
    };

    Vector2 bottomRight = {
        center.x + half,
        center.y + half
    };

    DrawLineEx(
        topLeft,
        (Vector2){ topLeft.x + arm, topLeft.y },
        2.0f,
        color
    );

    DrawLineEx(
        topLeft,
        (Vector2){ topLeft.x, topLeft.y + arm },
        2.0f,
        color
    );

    DrawLineEx(
        topRight,
        (Vector2){ topRight.x - arm, topRight.y },
        2.0f,
        color
    );

    DrawLineEx(
        topRight,
        (Vector2){ topRight.x, topRight.y + arm },
        2.0f,
        color
    );

    DrawLineEx(
        bottomLeft,
        (Vector2){ bottomLeft.x + arm, bottomLeft.y },
        2.0f,
        color
    );

    DrawLineEx(
        bottomLeft,
        (Vector2){ bottomLeft.x, bottomLeft.y - arm },
        2.0f,
        color
    );

    DrawLineEx(
        bottomRight,
        (Vector2){ bottomRight.x - arm, bottomRight.y },
        2.0f,
        color
    );

    DrawLineEx(
        bottomRight,
        (Vector2){ bottomRight.x, bottomRight.y - arm },
        2.0f,
        color
    );
}

static void TargetingDrawHealthBar(
    Vector2 position,
    float width,
    float ratio,
    Color color
)
{
    ratio = TargetingClamp(ratio, 0.0f, 1.0f);

    float barWidth = width;

    if (barWidth < 36.0f)
        barWidth = 36.0f;

    float x = position.x - barWidth * 0.5f;
    float y = position.y;

    x = TargetingClamp(
        x,
        4.0f,
        GetScreenWidth() - barWidth - 4.0f
    );

    y = TargetingClamp(
        y,
        4.0f,
        GetScreenHeight() - 9.0f
    );

    DrawRectangle(
        (int)x,
        (int)y,
        (int)barWidth,
        5,
        TargetingTint(TARGETING_GRAY, 225)
    );

    DrawRectangle(
        (int)x,
        (int)y,
        (int)(barWidth * ratio),
        5,
        color
    );

    DrawRectangleLines(
        (int)x,
        (int)y,
        (int)barWidth,
        5,
        TargetingTint(TARGETING_BLUE, 80)
    );
}

static void TargetingDrawPanel(
    const TargetMarker *marker,
    float lockRatio,
    Color color
)
{
    float half = marker->boxSize * 0.5f;

    float x = marker->screenPosition.x + half +
              TARGETING_PANEL_MARGIN;

    if (x + TARGETING_PANEL_WIDTH >
        GetScreenWidth() - TARGETING_PANEL_MARGIN)
    {
        x = marker->screenPosition.x - half -
            TARGETING_PANEL_MARGIN -
            TARGETING_PANEL_WIDTH;
    }

    x = TargetingClamp(
        x,
        TARGETING_PANEL_MARGIN,
        GetScreenWidth() -
            TARGETING_PANEL_WIDTH -
            TARGETING_PANEL_MARGIN
    );

    float y = marker->screenPosition.y - half -
              TARGETING_PANEL_MARGIN;

    y = TargetingClamp(
        y,
        TARGETING_PANEL_MARGIN,
        GetScreenHeight() -
            TARGETING_PANEL_HEIGHT -
            TARGETING_PANEL_MARGIN
    );

    Rectangle panel = {
        x,
        y,
        TARGETING_PANEL_WIDTH,
        TARGETING_PANEL_HEIGHT
    };

    DrawRectangleRounded(
        panel,
        0.16f,
        6,
        TargetingTint(TARGETING_GRAY, 225)
    );

    DrawRectangleLinesEx(
        panel,
        1.0f,
        TargetingTint(color, 100)
    );

    char distanceText[24];

    TargetingFormatDistance(
        distanceText,
        sizeof(distanceText),
        marker->distance
    );

    DrawText(
        TextFormat(
            "HP %d/%d",
            marker->health,
            marker->maxHealth),
        (int)x + 10,
        (int)y + 7,
        14,
        TargetingTint(TARGETING_BLUE, 245)
    );

    DrawText(
        TextFormat(
            "DMG %.0f",
            marker->damage),
        (int)x + 10,
        (int)y + 25,
        12,
        TargetingTint(TARGETING_BLUE, 175)
    );

    DrawText(
        distanceText,
        (int)x + 10,
        (int)y + 42,
        12,
        color
    );

    DrawText(
        TargetingStateLabel(marker->state),
        (int)x + 78,
        (int)y + 42,
        12,
        color
    );

    if (lockRatio >= 1.0f)
    {
        DrawText(
            "LOCK",
            (int)(x + TARGETING_PANEL_WIDTH - 42),
            (int)y + 8,
            10,
            color
        );

        return;
    }

    DrawRectangle(
        (int)x + 10,
        (int)y + TARGETING_PANEL_HEIGHT - 7,
        (int)((TARGETING_PANEL_WIDTH - 20) * lockRatio),
        2,
        color
    );
}

static void TargetingDrawOffscreen(
    const OffscreenMarker *marker
)
{
    float presence = TargetingClamp(
        marker->presence,
        0.0f,
        1.0f
    );

    float distanceFade =
        1.0f -
        TargetingClamp(
            marker->distance / TARGETING_OFFSCREEN_RANGE,
            0.0f,
            1.0f);

    unsigned char alpha = (unsigned char)TargetingClamp(
        (110.0f + 120.0f * presence) *
            (0.55f + 0.45f * distanceFade),
        40.0f,
        255.0f
    );

    Color color = TargetingStateColor(
        marker->state,
        alpha
    );

    Vector2 direction = marker->direction;
    Vector2 perpendicular = {
        -direction.y,
        direction.x
    };

    Vector2 anchor = marker->screenPosition;

    Vector2 tip = Vector2Add(
        anchor,
        Vector2Scale(direction, 12.0f)
    );

    Vector2 shoulder = Vector2Subtract(
        anchor,
        Vector2Scale(direction, 8.0f)
    );

    Vector2 left = Vector2Add(
        shoulder,
        Vector2Scale(perpendicular, 8.0f)
    );

    Vector2 right = Vector2Subtract(
        shoulder,
        Vector2Scale(perpendicular, 8.0f)
    );

    DrawLineEx(tip, left, 2.0f, color);
    DrawLineEx(tip, right, 2.0f, color);

    DrawLineEx(
        shoulder,
        tip,
        1.0f,
        TargetingTint(
            color,
            (unsigned char)(alpha * 0.45f)
        )
    );

    char distanceText[24];

    TargetingFormatDistance(
        distanceText,
        sizeof(distanceText),
        marker->distance
    );

    char label[48];

    if (marker->count > 1)
    {
        snprintf(
            label,
            sizeof(label),
            "%s x%d",
            distanceText,
            marker->count
        );
    }
    else
    {
        snprintf(
            label,
            sizeof(label),
            "%s",
            distanceText
        );
    }

    int fontSize = 12;
    int textWidth = MeasureText(label, fontSize);

    Vector2 textPosition = Vector2Subtract(
        anchor,
        Vector2Scale(direction, 30.0f)
    );

    textPosition.x -= textWidth * 0.5f;
    textPosition.y -= 6.0f;

    textPosition.x = TargetingClamp(
        textPosition.x,
        6.0f,
        GetScreenWidth() - textWidth - 6.0f
    );

    textPosition.y = TargetingClamp(
        textPosition.y,
        4.0f,
        GetScreenHeight() - fontSize - 4.0f
    );

    DrawRectangleRounded(
        (Rectangle){
            textPosition.x - 5.0f,
            textPosition.y - 2.0f,
            textWidth + 10.0f,
            fontSize + 4.0f
        },
        0.3f,
        4,
        TargetingTint(TARGETING_GRAY, 190)
    );

    DrawRectangleLinesEx(
        (Rectangle){
            textPosition.x - 5.0f,
            textPosition.y - 2.0f,
            textWidth + 10.0f,
            fontSize + 4.0f
        },
        1.0f,
        TargetingTint(TARGETING_BLUE, 65)
    );

    DrawText(
        label,
        (int)textPosition.x,
        (int)textPosition.y,
        fontSize,
        color
    );
}

void DrawTargeting(const TargetingSystem *system)
{
    if (system == NULL)
        return;

    for (int i = 0;
         i < system->visibleCount;
         i++)
    {
        const TargetMarker *marker =
            &system->visible[i];

        float presence = TargetingClamp(
            marker->presence,
            0.0f,
            1.0f
        );

        float alphaValue =
            (marker->selected ? 235.0f : 120.0f) *
            presence;

        Color color = TargetingStateColor(
            marker->state,
            (unsigned char)TargetingClamp(
                alphaValue,
                20.0f,
                255.0f)
        );

        float size =
            marker->boxSize *
            (0.78f + 0.22f * presence);

        TargetingDrawBrackets(
            marker->screenPosition,
            size,
            color
        );

        if (marker->selected)
        {
            DrawRectangleLinesEx(
                (Rectangle){
                    marker->screenPosition.x - size * 0.5f,
                    marker->screenPosition.y - size * 0.5f,
                    size,
                    size
                },
                1.0f,
                TargetingTint(color, 55)
            );
        }

        float ratio = 0.0f;

        if (marker->maxHealth > 0)
        {
            ratio =
                (float)marker->health /
                (float)marker->maxHealth;
        }

        TargetingDrawHealthBar(
            (Vector2){
                marker->screenPosition.x,
                marker->screenPosition.y -
                    size * 0.5f -
                    12.0f
            },
            size,
            ratio,
            TargetingHealthColor(
                ratio,
                marker->selected ? 240 : 140
            )
        );

        if (!marker->selected)
            continue;

        float lockRatio = TargetingClamp(
            system->selectionTimer / TARGETING_LOCK_DELAY,
            0.0f,
            1.0f
        );

        TargetingDrawPanel(
            marker,
            lockRatio,
            color
        );
    }

    for (int i = 0;
         i < system->offscreenCount;
         i++)
    {
        TargetingDrawOffscreen(
            &system->offscreen[i]
        );
    }
}

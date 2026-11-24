#include "trail.h"
#include "raymath.h"
#include "rlgl.h"

#include <math.h>
#include <stdlib.h>


static Vector3 batchCameraDirection;
static bool batchActive = false;


static void AddTrailPoint(
    Trail *trail,
    Vector3 position
)
{
    if (trail->count >= TRAIL_MAX_POINTS)
    {
        for (int i = 1; i < trail->count; i++)
        {
            trail->points[i - 1] =
                trail->points[i];
        }

        trail->count--;
    }

    TrailPoint *point =
        &trail->points[trail->count++];

    point->position = position;
    point->age = 0.0f;
}


void UpdateTrail(
    Trail *trail,
    Vector3 position,
    float deltaTime,
    const TrailConfig *config
)
{
    if (trail == NULL || config == NULL)
        return;

    for (int i = 0; i < trail->count;)
    {
        trail->points[i].age += deltaTime;

        if (trail->points[i].age >= config->lifetime)
        {
            for (int j = i + 1;
                 j < trail->count;
                 j++)
            {
                trail->points[j - 1] =
                    trail->points[j];
            }

            trail->count--;
        }
        else
        {
            i++;
        }
    }

    trail->sampleTimer -= deltaTime;

    if (trail->sampleTimer <= 0.0f)
    {
        AddTrailPoint(
            trail,
            position
        );

        trail->sampleTimer +=
            config->sampleInterval;
    }
}


static Color FadeColor(
    Color color,
    float alpha
)
{
    color.a =
        (unsigned char)(
            255.0f *
            Clamp(alpha, 0.0f, 1.0f)
        );

    return color;
}


static void EmitVertex(Vector3 position)
{
    rlVertex3f(
        position.x,
        position.y,
        position.z
    );
}


static void EmitTriangle(
    Vector3 a,
    Vector3 b,
    Vector3 c,
    Color color
)
{
    rlColor4ub(
        color.r,
        color.g,
        color.b,
        color.a
    );

    EmitVertex(a);
    EmitVertex(b);
    EmitVertex(c);
}


void BeginTrailBatch(Camera3D camera)
{
    batchCameraDirection =
        Vector3Normalize(
            Vector3Subtract(
                camera.target,
                camera.position
            )
        );

    rlSetTexture(rlGetTextureIdDefault());

    BeginBlendMode(BLEND_ADDITIVE);

    rlBegin(RL_TRIANGLES);

    batchActive = true;
}


void DrawTrailInBatch(
    const Trail *trail,
    const TrailConfig *config,
    Vector3 up
)
{
    if (!batchActive ||
        trail == NULL ||
        config == NULL ||
        trail->count < 2)
    {
        return;
    }

    for (int i = 1;
         i < trail->count;
         i++)
    {
        TrailPoint a =
            trail->points[i - 1];

        TrailPoint b =
            trail->points[i];

        float ageA =
            a.age / config->lifetime;

        float ageB =
            b.age / config->lifetime;

        float alphaA =
            1.0f - ageA;

        float alphaB =
            1.0f - ageB;

        Vector3 tangent =
            Vector3Subtract(
                b.position,
                a.position
            );

        if (Vector3LengthSqr(tangent)
            < 0.000001f)
        {
            continue;
        }

        tangent =
            Vector3Normalize(tangent);

        Vector3 side =
            Vector3CrossProduct(
                tangent,
                batchCameraDirection
            );

        if (Vector3LengthSqr(side)
            < 0.000001f)
        {
            side =
                Vector3CrossProduct(
                    tangent,
                    up
                );
        }

        if (Vector3LengthSqr(side)
            < 0.000001f)
        {
            continue;
        }

        side =
            Vector3Normalize(side);

        float widthA =
            config->width * alphaA;

        float widthB =
            config->width * alphaB;

        Vector3 sideA =
            Vector3Scale(side, widthA);

        Vector3 sideB =
            Vector3Scale(side, widthB);

        Vector3 aLeft =
            Vector3Add(a.position, sideA);

        Vector3 aRight =
            Vector3Subtract(a.position, sideA);

        Vector3 bLeft =
            Vector3Add(b.position, sideB);

        Vector3 bRight =
            Vector3Subtract(b.position, sideB);

        Color outerA =
            FadeColor(config->color, alphaA * 0.30f);

        Color outerB =
            FadeColor(config->color, alphaB * 0.30f);

        EmitTriangle(aLeft, aRight, bLeft, outerA);
        EmitTriangle(aRight, bRight, bLeft, outerB);


        float coreWidthA =
            config->coreWidth * alphaA;

        float coreWidthB =
            config->coreWidth * alphaB;

        Vector3 coreSideA =
            Vector3Scale(side, coreWidthA);

        Vector3 coreSideB =
            Vector3Scale(side, coreWidthB);

        Vector3 aCoreLeft =
            Vector3Add(a.position, coreSideA);

        Vector3 aCoreRight =
            Vector3Subtract(a.position, coreSideA);

        Vector3 bCoreLeft =
            Vector3Add(b.position, coreSideB);

        Vector3 bCoreRight =
            Vector3Subtract(b.position, coreSideB);

        Color coreA =
            FadeColor(config->coreColor, alphaA * 0.85f);

        Color coreB =
            FadeColor(config->coreColor, alphaB * 0.85f);

        EmitTriangle(aCoreLeft, aCoreRight, bCoreLeft, coreA);
        EmitTriangle(aCoreRight, bCoreRight, bCoreLeft, coreB);
    }
}


void DrawTrailGlow(
    Vector3 position,
    float radius,
    Color color
)
{
    if (!batchActive || radius <= 0.0f)
        return;

    Vector3 right =
        Vector3CrossProduct(
            batchCameraDirection,
            (Vector3){ 0.0f, 1.0f, 0.0f }
        );

    if (Vector3LengthSqr(right) < 0.000001f)
    {
        right = (Vector3){ 1.0f, 0.0f, 0.0f };
    }

    right = Vector3Normalize(right);

    Vector3 up =
        Vector3Normalize(
            Vector3CrossProduct(
                right,
                batchCameraDirection
            )
        );

    Color outer =
        FadeColor(color, 0.18f);

    Color inner =
        FadeColor(color, 0.90f);

    Vector3 outerRight =
        Vector3Scale(right, radius * 1.6f);

    Vector3 outerUp =
        Vector3Scale(up, radius * 1.6f);

    Vector3 outerTopLeft =
        Vector3Add(
            Vector3Subtract(position, outerRight),
            outerUp);

    Vector3 outerTopRight =
        Vector3Add(
            Vector3Add(position, outerRight),
            outerUp);

    Vector3 outerBottomLeft =
        Vector3Subtract(
            Vector3Subtract(position, outerRight),
            outerUp);

    Vector3 outerBottomRight =
        Vector3Subtract(
            Vector3Add(position, outerRight),
            outerUp);

    EmitTriangle(
        outerTopLeft,
        outerBottomLeft,
        outerTopRight,
        outer);

    EmitTriangle(
        outerTopRight,
        outerBottomLeft,
        outerBottomRight,
        outer);

    Vector3 innerRight =
        Vector3Scale(right, radius);

    Vector3 innerUp =
        Vector3Scale(up, radius);

    Vector3 innerTopLeft =
        Vector3Add(
            Vector3Subtract(position, innerRight),
            innerUp);

    Vector3 innerTopRight =
        Vector3Add(
            Vector3Add(position, innerRight),
            innerUp);

    Vector3 innerBottomLeft =
        Vector3Subtract(
            Vector3Subtract(position, innerRight),
            innerUp);

    Vector3 innerBottomRight =
        Vector3Subtract(
            Vector3Add(position, innerRight),
            innerUp);

    EmitTriangle(
        innerTopLeft,
        innerBottomLeft,
        innerTopRight,
        inner);

    EmitTriangle(
        innerTopRight,
        innerBottomLeft,
        innerBottomRight,
        inner);
}


void EndTrailBatch(void)
{
    if (!batchActive)
        return;

    rlEnd();

    EndBlendMode();

    batchActive = false;
}


void DrawTrail(
    const Trail *trail,
    const TrailConfig *config,
    Vector3 up,
    Camera3D camera
)
{
    BeginTrailBatch(camera);
    DrawTrailInBatch(trail, config, up);
    EndTrailBatch();
}

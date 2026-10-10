#include "trail.h"
#include "raymath.h"

#include <math.h>
#include <stdlib.h>


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


static void DrawTrailRibbon(
    const Trail *trail,
    const TrailConfig *config,
    Vector3 up,
    Camera3D camera
)
{
    if (trail == NULL ||
        config == NULL ||
        trail->count < 2)
    {
        return;
    }

    Vector3 cameraDirection =
        Vector3Normalize(
            Vector3Subtract(
                camera.target,
                camera.position
            )
        );

    BeginBlendMode(
        BLEND_ADDITIVE
    );

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
                cameraDirection
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

        Vector3 aLeft =
            Vector3Add(
                a.position,
                Vector3Scale(
                    side,
                    widthA
                )
            );

        Vector3 aRight =
            Vector3Subtract(
                a.position,
                Vector3Scale(
                    side,
                    widthA
                )
            );

        Vector3 bLeft =
            Vector3Add(
                b.position,
                Vector3Scale(
                    side,
                    widthB
                )
            );

        Vector3 bRight =
            Vector3Subtract(
                b.position,
                Vector3Scale(
                    side,
                    widthB
                )
            );

        Color outerA =
            FadeColor(
                config->color,
                alphaA * 0.30f
            );

        Color outerB =
            FadeColor(
                config->color,
                alphaB * 0.30f
            );

        Color coreA =
            FadeColor(
                config->coreColor,
                alphaA * 0.85f
            );

        Color coreB =
            FadeColor(
                config->coreColor,
                alphaB * 0.85f
            );

        DrawTriangle3D(
            aLeft,
            aRight,
            bLeft,
            outerA
        );

        DrawTriangle3D(
            aRight,
            bRight,
            bLeft,
            outerB
        );


        float coreAWidth =
            config->coreWidth * alphaA;

        float coreBWidth =
            config->coreWidth * alphaB;

        Vector3 aCoreLeft =
            Vector3Add(
                a.position,
                Vector3Scale(
                    side,
                    coreAWidth
                )
            );

        Vector3 aCoreRight =
            Vector3Subtract(
                a.position,
                Vector3Scale(
                    side,
                    coreAWidth
                )
            );

        Vector3 bCoreLeft =
            Vector3Add(
                b.position,
                Vector3Scale(
                    side,
                    coreBWidth
                )
            );

        Vector3 bCoreRight =
            Vector3Subtract(
                b.position,
                Vector3Scale(
                    side,
                    coreBWidth
                )
            );

        DrawTriangle3D(
            aCoreLeft,
            aCoreRight,
            bCoreLeft,
            coreA
        );

        DrawTriangle3D(
            aCoreRight,
            bCoreRight,
            bCoreLeft,
            coreB
        );
    }

    EndBlendMode();
}


void DrawTrail(
    const Trail *trail,
    const TrailConfig *config,
    Vector3 up,
    Camera3D camera
)
{
    DrawTrailRibbon(
        trail,
        config,
        up,
        camera
    );
}
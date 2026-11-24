#include "planet.h"
#include "raylib.h"
#include "raymath.h"
#include "noise.h"
#include <stdio.h>

Planet CreatePlanet(
    float radius,
    int resolution,
    float seed,
    float heightAmplitude,
    Material material)
{
    Planet planet = {0};

    planet.radius = radius;

    planet.heightAmplitude = heightAmplitude;

    planet.material = material;

    planet.position = (Vector3){
        0.0f,
        0.0f,
        0.0f};
    planet.seed = seed;

    planet.resolution = resolution;

    planet.maxHealth = 15000;
    planet.health = planet.maxHealth;

    for (int face = 0; face < 6; face++)
    {
        planet.faces[face] =
            CreatePlanetPatch(
                face,
                -1.0f,
                1.0f,
                -1.0f,
                1.0f,
                resolution,
                radius,
                seed,
                heightAmplitude);
    }

    return planet;
}

Vector3 CubePointUV(
    int face,
    float u,
    float v)
{
    Vector3 point;

    switch (face)
    {

    // +X
    case 0:
        point = (Vector3){
            1.0f,
            v,
            -u};
        break;

    // -X
    case 1:
        point = (Vector3){
            -1.0f,
            v,
            u};
        break;

    // +Y
    case 2:
        point = (Vector3){
            u,
            1.0f,
            v};
        break;

    // -Y
    case 3:
        point = (Vector3){
            u,
            -1.0f,
            -v};
        break;

    // +Z
    case 4:
        point = (Vector3){
            u,
            v,
            1.0f};
        break;

    // -Z
    case 5:
        point = (Vector3){
            -u,
            v,
            -1.0f};
        break;

    default:
        return (Vector3){0};
    }

    return Vector3Normalize(point);
}

PlanetPatch CreatePlanetPatch(
    int face,
    float uMin,
    float uMax,
    float vMin,
    float vMax,
    int resolution,
    float radius,
    float seed,
    float heightAmplitude)
{
    PlanetPatch patch = {0};

    patch.face = face;

    patch.uMin = uMin;
    patch.uMax = uMax;

    patch.vMin = vMin;
    patch.vMax = vMax;

    patch.centerDirection =
        CubePointUV(
            face,
            (uMin + uMax) * 0.5f,
            (vMin + vMax) * 0.5f);

    patch.angularRadius = 0.0f;

    float cornerU[2] = { uMin, uMax };
    float cornerV[2] = { vMin, vMax };

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            Vector3 corner =
                CubePointUV(
                    face,
                    cornerU[i],
                    cornerV[j]);

            float dot = Clamp(
                Vector3DotProduct(
                    patch.centerDirection,
                    corner),
                -1.0f,
                1.0f);

            float angle = acosf(dot);

            if (angle > patch.angularRadius)
                patch.angularRadius = angle;
        }
    }

    int vertexCount =
        resolution * resolution;

    int quadCount =
        (resolution - 1) *
        (resolution - 1);

    int triangleCount =
        quadCount * 2;

    int indexCount =
        triangleCount * 3;

    patch.mesh.vertexCount =
        vertexCount;

    patch.mesh.triangleCount =
        triangleCount;

    patch.mesh.vertices =
        MemAlloc(
            vertexCount * 3 * sizeof(float));

    patch.mesh.normals =
        MemAlloc(
            vertexCount * 3 * sizeof(float));

    patch.mesh.indices =
        MemAlloc(
            indexCount * sizeof(unsigned short));

    patch.mesh.texcoords =
        MemAlloc(
            vertexCount * 2 * sizeof(float));

    patch.directions =
        MemAlloc(
            vertexCount * sizeof(Vector3));

    for (int y = 0; y < resolution; y++)
    {
        for (int x = 0; x < resolution; x++)
        {

            float tx =
                (float)x / (resolution - 1);

            float ty =
                (float)y / (resolution - 1);

            float u =
                uMin +
                (uMax - uMin) * tx;

            float v =
                vMin +
                (vMax - vMin) * ty;

            int index = y * resolution + x;

            Vector3 direction =
                CubePointUV(
                    face,
                    u,
                    v);

            float height =
                GetTerrainHeight(
                    direction,
                    seed,
                    heightAmplitude);

            Vector3 position =
                Vector3Scale(
                    direction,
                    radius + height);

            patch.mesh.vertices[index * 3 + 0] =
                position.x;

            patch.mesh.vertices[index * 3 + 1] =
                position.y;

            patch.mesh.vertices[index * 3 + 2] =
                position.z;

            patch.directions[index] =
                direction;

            patch.mesh.normals[index * 3 + 0] =
                direction.x;

            patch.mesh.normals[index * 3 + 1] =
                direction.y;

            patch.mesh.normals[index * 3 + 2] =
                direction.z;

            patch.mesh.texcoords[index * 2 + 0] = tx;
            patch.mesh.texcoords[index * 2 + 1] = ty;
        }
    }

    int indexOffset = 0;

    for (int y = 0; y < resolution - 1; y++)
    {
        for (int x = 0; x < resolution - 1; x++)
        {

            unsigned short topLeft =
                y * resolution + x;

            unsigned short topRight =
                topLeft + 1;

            unsigned short bottomLeft =
                topLeft + resolution;

            unsigned short bottomRight =
                bottomLeft + 1;

            Vector3 A = {
                patch.mesh.vertices[topLeft * 3 + 0],
                patch.mesh.vertices[topLeft * 3 + 1],
                patch.mesh.vertices[topLeft * 3 + 2]};

            Vector3 B = {
                patch.mesh.vertices[bottomLeft * 3 + 0],
                patch.mesh.vertices[bottomLeft * 3 + 1],
                patch.mesh.vertices[bottomLeft * 3 + 2]};

            Vector3 C = {
                patch.mesh.vertices[topRight * 3 + 0],
                patch.mesh.vertices[topRight * 3 + 1],
                patch.mesh.vertices[topRight * 3 + 2]};

            Vector3 AB =
                Vector3Subtract(B, A);

            Vector3 AC =
                Vector3Subtract(C, A);

            Vector3 normal =
                Vector3CrossProduct(AB, AC);

            float orientation =
                Vector3DotProduct(normal, A);

            if (orientation > 0.0f)
            {

                patch.mesh.indices[indexOffset++] =
                    topLeft;

                patch.mesh.indices[indexOffset++] =
                    bottomLeft;

                patch.mesh.indices[indexOffset++] =
                    topRight;
            }
            else
            {

                patch.mesh.indices[indexOffset++] =
                    topLeft;

                patch.mesh.indices[indexOffset++] =
                    topRight;

                patch.mesh.indices[indexOffset++] =
                    bottomLeft;
            }

            A = (Vector3){
                patch.mesh.vertices[topRight * 3 + 0],
                patch.mesh.vertices[topRight * 3 + 1],
                patch.mesh.vertices[topRight * 3 + 2]};

            B = (Vector3){
                patch.mesh.vertices[bottomLeft * 3 + 0],
                patch.mesh.vertices[bottomLeft * 3 + 1],
                patch.mesh.vertices[bottomLeft * 3 + 2]};

            C = (Vector3){
                patch.mesh.vertices[bottomRight * 3 + 0],
                patch.mesh.vertices[bottomRight * 3 + 1],
                patch.mesh.vertices[bottomRight * 3 + 2]};

            AB =
                Vector3Subtract(B, A);

            AC =
                Vector3Subtract(C, A);

            normal =
                Vector3CrossProduct(AB, AC);

            orientation =
                Vector3DotProduct(normal, A);

            if (orientation > 0.0f)
            {

                patch.mesh.indices[indexOffset++] =
                    topRight;

                patch.mesh.indices[indexOffset++] =
                    bottomLeft;

                patch.mesh.indices[indexOffset++] =
                    bottomRight;
            }
            else
            {

                patch.mesh.indices[indexOffset++] =
                    topRight;

                patch.mesh.indices[indexOffset++] =
                    bottomRight;

                patch.mesh.indices[indexOffset++] =
                    bottomLeft;
            }
        }
    }

    UploadMesh(
        &patch.mesh,
        false);

    return patch;
}

// bool PlanetContainsSphere(
//     const Planet *planet,
//     Vector3 point,
//     float objectRadius
// ) {
//     Vector3 offset = Vector3Subtract(point, planet->position);
//     float distance = Vector3Length(offset);

//     if (distance == 0.0f) {
//         return true;
//     }

//     Vector3 direction = Vector3Normalize(offset);

//     float height = GetTerrainHeight(
//         direction,
//         planet->seed,
//         planet->heightAmplitude
//     );

//     float surfaceRadius = planet->radius + height;

//     return distance < surfaceRadius + objectRadius;
// }

bool PlanetContainsSphere(
    const Planet *planet,
    Vector3 point,
    float objectRadius)
{
    if (planet == NULL)
        return false;

    Vector3 offset = Vector3Subtract(
        point,
        planet->position);

    float distance = Vector3Length(offset);

    if (distance < 0.00001f)
        return true;

    float maxSurfaceRadius =
        planet->radius +
        planet->heightAmplitude;

    if (distance > maxSurfaceRadius + objectRadius)
        return false;

    Vector3 direction = Vector3Scale(
        offset,
        1.0f / distance);

    float height = GetTerrainHeight(
        direction,
        planet->seed,
        planet->heightAmplitude);

    float surfaceRadius = planet->radius + height;

    float minRadius =
        planet->radius * MIN_PLANET_RADIUS_RATIO;

    for (int i = 0; i < planet->deformationCount; i++)
    {
        const TerrainDeformation *deformation =
            &planet->deformations[i];

        if (deformation->radius <= 0.00001f)
            continue;

        float dot = Vector3DotProduct(
            direction,
            deformation->direction);

        if (dot < deformation->cosRadius)
            continue;

        float angle = acosf(
            Clamp(dot, -1.0f, 1.0f));

        float t = angle / deformation->radius;

        float falloff =
            1.0f - t * t * (3.0f - 2.0f * t);

        surfaceRadius -=
            deformation->strength * falloff;

        if (surfaceRadius < minRadius)
            surfaceRadius = minRadius;
    }

    return distance < surfaceRadius + objectRadius;
}



void UpdatePlanet(
    Planet *planet,
    Vector3 cameraPosition)
{
    if (
        planet->deformationCount >
        planet->appliedDeformationCount)
    {
        UpdatePlanetPatch(planet);
    }
}

void DrawPlanet(
    Planet *planet,
    Material material)
{
    if (planet == NULL)
        return;

    Shader shader = material.shader;

    int deformationCountLoc =
        GetShaderLocation(
            shader,
            "deformationCount");

    int deformationDataLoc =
        GetShaderLocation(
            shader,
            "deformationData");

    int deformationCutoffLoc =
        GetShaderLocation(
            shader,
            "deformationCutoff");

    if (deformationCutoffLoc < 0)
        deformationCutoffLoc =
            GetShaderLocation(
                shader,
                "deformationCutoff[0]");

    SetShaderValue(
        shader,
        deformationCountLoc,
        &planet->deformationCount,
        SHADER_UNIFORM_INT);

    Vector4 deformationData[MAX_DEFORMATIONS] = {0};
    float deformationCutoff[MAX_DEFORMATIONS] = {0};

    for (
        int i = 0;
        i < planet->deformationCount;
        i++)
    {
        TerrainDeformation *deformation =
            &planet->deformations[i];

        deformationData[i] =
            (Vector4){
                deformation->direction.x,
                deformation->direction.y,
                deformation->direction.z,
                deformation->radius};

        deformationCutoff[i] =
            deformation->cosCutoff;
    }

    if (planet->deformationCount > 0)
    {
        SetShaderValueV(
            shader,
            deformationDataLoc,
            deformationData,
            SHADER_UNIFORM_VEC4,
            planet->deformationCount);

        SetShaderValueV(
            shader,
            deformationCutoffLoc,
            deformationCutoff,
            SHADER_UNIFORM_FLOAT,
            planet->deformationCount);
    }

    for (int face = 0; face < 6; face++)
    {
        DrawMesh(
            planet->faces[face].mesh,
            material,
            MatrixTranslate(
                planet->position.x,
                planet->position.y,
                planet->position.z));
    }
}

void DamagePlanet(Planet *planet, Vector3 impactPoint, float damage)
{
    if (planet == NULL || damage <= 0.0f)
    {
        return;
    }

    if (planet->health >= damage) {
        planet->health -= damage;
    } else {
        planet->health = 0;
    }

    if (planet->deformationCount >= MAX_DEFORMATIONS)
    {
        return;
    }

    Vector3 impactDirection = Vector3Normalize(
        Vector3Subtract(
            impactPoint,
            planet->position));

    float radius = 0.01f * damage;
    float cutoffAngle = radius * 1.45f;

    if (cutoffAngle > 3.14159265f)
        cutoffAngle = 3.14159265f;

    TerrainDeformation deformation = {
        .direction = impactDirection,
        .radius = radius,
        .strength = damage,
        .cosRadius = cosf(radius),
        .cosCutoff = cosf(cutoffAngle)};

    planet->deformations[planet->deformationCount++] = deformation;
}

// void UpdatePlanetPatch(Planet *planet) {

//     double start;
//     double end;

//     start = GetTime();

//     for(int face = 0; face < 6; face++) {
//         PlanetPatch *patch = &planet->faces[face];

//         int vertexCount = patch->mesh.vertexCount;

//         for(int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {

//                 Vector3 originalPosition = {
//                     patch->originalVertices[vertexIndex * 3 + 0],
//                     patch->originalVertices[vertexIndex * 3 + 1],
//                     patch->originalVertices[vertexIndex * 3 + 2]
//                 };

//                 Vector3 vertexDirection = Vector3Normalize(originalPosition);

//                 Vector3 currentPosition = originalPosition;

//                 for(int deformationIndex = 0; deformationIndex < planet->deformationCount; deformationIndex++) {

//                     TerrainDeformation deformation = planet->deformations[deformationIndex];

//                     float dot = Vector3DotProduct(deformation.direction, vertexDirection);

//                     float angle = acosf(
//                         Clamp(dot, -1.0f, 1.0f)
//                     );

//                     if (angle > deformation.radius) {
//                         continue;
//                     }

//                     float t = angle / deformation.radius;

//                     float falloff = 1.0f - t * t * (3.0f - 2.0f * t);

//                     float displacement = deformation.strength * falloff;

//                     float currentRadius = Vector3Length(currentPosition);

//                     float minRadius = planet->radius * MIN_PLANET_RADIUS_RATIO;

//                     float maxDisplacement =
//                         currentRadius - minRadius;

//                     if (maxDisplacement <= 0.0f) {
//                         break;
//                     }

//                     float actualDisplacement =
//                         fminf(
//                             displacement,
//                             maxDisplacement
//                         );

//                     currentPosition = Vector3Subtract(
//                         currentPosition,
//                         Vector3Scale(
//                             vertexDirection,
//                             actualDisplacement
//                         )
//                     );
//             }

//             patch->mesh.vertices[vertexIndex * 3 + 0] = currentPosition.x;
//             patch->mesh.vertices[vertexIndex * 3 + 1] = currentPosition.y;
//             patch->mesh.vertices[vertexIndex * 3 + 2] = currentPosition.z;

//         }

//         RecalculatePlanetPatchNormals(patch);

//         UpdateMeshBuffer(
//             patch->mesh,
//             0,
//             patch->mesh.vertices,
//             vertexCount * 3 * sizeof(float),
//             0
//         );

//         UpdateMeshBuffer(
//             patch->mesh,
//             2,
//             patch->mesh.normals,
//             vertexCount * 3 * sizeof(float),
//             0
//         );

//     }

//     end = GetTime();

// }

void UpdatePlanetPatch(Planet *planet)
{
    if (planet == NULL)
        return;

    int firstNew =
        planet->appliedDeformationCount;

    int lastNew =
        planet->deformationCount;

    if (firstNew < 0)
        firstNew = 0;

    if (lastNew > MAX_DEFORMATIONS)
        lastNew = MAX_DEFORMATIONS;

    if (lastNew > firstNew + MAX_DEFORMATIONS_PER_FRAME)
        lastNew = firstNew + MAX_DEFORMATIONS_PER_FRAME;

    if (firstNew >= lastNew)
        return;

    planet->appliedDeformationCount = lastNew;

    const float minRadius =
        planet->radius * MIN_PLANET_RADIUS_RATIO;

    for (int face = 0; face < 6; face++)
    {
        PlanetPatch *patch = &planet->faces[face];

        int vertexCount = patch->mesh.vertexCount;
        bool changed = false;

        for (int d = firstNew; d < lastNew; d++)
        {
            const TerrainDeformation *deformation =
                &planet->deformations[d];

            float craterRadius = deformation->radius;

            if (craterRadius <= 0.00001f)
                continue;

            float faceCutoff = cosf(
                fminf(
                    craterRadius + patch->angularRadius,
                    3.14159265f));

            if (Vector3DotProduct(
                    patch->centerDirection,
                    deformation->direction) < faceCutoff)
            {
                continue;
            }

            float minDot = deformation->cosRadius;

            for (int i = 0; i < vertexCount; i++)
            {
                int offset = i * 3;

                Vector3 direction =
                    patch->directions[i];

                float dot = Vector3DotProduct(
                    deformation->direction,
                    direction);

                if (dot < minDot)
                    continue;

                float angle = acosf(
                    Clamp(dot, -1.0f, 1.0f));

                float t = angle / craterRadius;

                float falloff =
                    1.0f - t * t * (3.0f - 2.0f * t);

                float displacement =
                    deformation->strength * falloff;

                Vector3 currentPosition = {
                    patch->mesh.vertices[offset],
                    patch->mesh.vertices[offset + 1],
                    patch->mesh.vertices[offset + 2]};

                float currentRadius =
                    Vector3Length(currentPosition);

                float maxDisplacement =
                    currentRadius - minRadius;

                if (maxDisplacement <= 0.0f)
                    continue;

                if (displacement > maxDisplacement)
                    displacement = maxDisplacement;

                currentPosition = Vector3Subtract(
                    currentPosition,
                    Vector3Scale(direction, displacement));

                patch->mesh.vertices[offset] =
                    currentPosition.x;

                patch->mesh.vertices[offset + 1] =
                    currentPosition.y;

                patch->mesh.vertices[offset + 2] =
                    currentPosition.z;

                changed = true;
            }
        }

        if (!changed)
            continue;

        RecalculatePlanetPatchNormals(patch);

        UpdateMeshBuffer(
            patch->mesh,
            0,
            patch->mesh.vertices,
            vertexCount * 3 * sizeof(float),
            0);

        UpdateMeshBuffer(
            patch->mesh,
            2,
            patch->mesh.normals,
            vertexCount * 3 * sizeof(float),
            0);
    }
}

void RecalculatePlanetPatchNormals(PlanetPatch *patch)
{
    int vertexCount = patch->mesh.vertexCount;
    int indexCount = patch->mesh.triangleCount * 3;

    for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
    {
        patch->mesh.normals[vertexIndex * 3 + 0] = 0.0f;
        patch->mesh.normals[vertexIndex * 3 + 1] = 0.0f;
        patch->mesh.normals[vertexIndex * 3 + 2] = 0.0f;
    }

    for (int i = 0; i < indexCount; i += 3)
    {

        unsigned short indexA =
            patch->mesh.indices[i + 0];

        unsigned short indexB =
            patch->mesh.indices[i + 1];

        unsigned short indexC =
            patch->mesh.indices[i + 2];

        Vector3 A = {
            patch->mesh.vertices[indexA * 3 + 0],
            patch->mesh.vertices[indexA * 3 + 1],
            patch->mesh.vertices[indexA * 3 + 2]};

        Vector3 B = {
            patch->mesh.vertices[indexB * 3 + 0],
            patch->mesh.vertices[indexB * 3 + 1],
            patch->mesh.vertices[indexB * 3 + 2]};

        Vector3 C = {
            patch->mesh.vertices[indexC * 3 + 0],
            patch->mesh.vertices[indexC * 3 + 1],
            patch->mesh.vertices[indexC * 3 + 2]};

        Vector3 AB =
            Vector3Subtract(B, A);

        Vector3 AC =
            Vector3Subtract(C, A);

        Vector3 normal =
            Vector3CrossProduct(AB, AC);

        patch->mesh.normals[indexA * 3 + 0] += normal.x;
        patch->mesh.normals[indexA * 3 + 1] += normal.y;
        patch->mesh.normals[indexA * 3 + 2] += normal.z;

        patch->mesh.normals[indexB * 3 + 0] += normal.x;
        patch->mesh.normals[indexB * 3 + 1] += normal.y;
        patch->mesh.normals[indexB * 3 + 2] += normal.z;

        patch->mesh.normals[indexC * 3 + 0] += normal.x;
        patch->mesh.normals[indexC * 3 + 1] += normal.y;
        patch->mesh.normals[indexC * 3 + 2] += normal.z;
    }

    for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
    {

        Vector3 normal = {
            patch->mesh.normals[vertexIndex * 3 + 0],
            patch->mesh.normals[vertexIndex * 3 + 1],
            patch->mesh.normals[vertexIndex * 3 + 2]};

        normal = Vector3Normalize(normal);

        patch->mesh.normals[vertexIndex * 3 + 0] = normal.x;
        patch->mesh.normals[vertexIndex * 3 + 1] = normal.y;
        patch->mesh.normals[vertexIndex * 3 + 2] = normal.z;
    }
}
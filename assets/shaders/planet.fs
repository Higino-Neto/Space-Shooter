#version 330

in vec3 fragWorldPosition;
in vec3 fragWorldNormal;

uniform vec3 planetPosition;
uniform float planetRadius;

#define MAX_DEFORMATIONS 200

uniform int deformationCount;
uniform vec4 deformationData[MAX_DEFORMATIONS];

out vec4 finalColor;


// =========================================================
// ILUMINAÇÃO
// =========================================================

const vec3 SUN_DIRECTION =
    normalize(vec3(1.0, 1.0, 0.8));

const vec3 SUN_COLOR =
    vec3(1.0, 0.92, 0.78);

const vec3 SKY_LIGHT =
    vec3(0.10, 0.16, 0.28);

const vec3 NIGHT_LIGHT =
    vec3(0.008, 0.015, 0.035);


// =========================================================
// HASH
// =========================================================

float hash(vec3 p)
{
    p = fract(
        p * 0.3183099 +
        vec3(0.1, 0.2, 0.3)
    );

    p *= 17.0;

    return fract(
        p.x *
        p.y *
        p.z *
        (p.x + p.y + p.z)
    );
}


// =========================================================
// VALUE NOISE 3D
// =========================================================

float noise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);

    f = f * f * (3.0 - 2.0 * f);

    float n000 =
        hash(i + vec3(0.0, 0.0, 0.0));

    float n100 =
        hash(i + vec3(1.0, 0.0, 0.0));

    float n010 =
        hash(i + vec3(0.0, 1.0, 0.0));

    float n110 =
        hash(i + vec3(1.0, 1.0, 0.0));

    float n001 =
        hash(i + vec3(0.0, 0.0, 1.0));

    float n101 =
        hash(i + vec3(1.0, 0.0, 1.0));

    float n011 =
        hash(i + vec3(0.0, 1.0, 1.0));

    float n111 =
        hash(i + vec3(1.0, 1.0, 1.0));

    float nx00 =
        mix(n000, n100, f.x);

    float nx10 =
        mix(n010, n110, f.x);

    float nx01 =
        mix(n001, n101, f.x);

    float nx11 =
        mix(n011, n111, f.x);

    float nxy0 =
        mix(nx00, nx10, f.y);

    float nxy1 =
        mix(nx01, nx11, f.y);

    return mix(
        nxy0,
        nxy1,
        f.z
    );
}


// =========================================================
// FRACTAL BROWNIAN MOTION
// =========================================================

float fbm(vec3 p)
{
    float value = 0.0;
    float amplitude = 0.5;

    value += noise(p) * amplitude;

    p *= 2.0;
    amplitude *= 0.5;

    value += noise(p) * amplitude;

    p *= 2.0;
    amplitude *= 0.5;

    value += noise(p) * amplitude;

    p *= 2.0;
    amplitude *= 0.5;

    value += noise(p) * amplitude;

    p *= 2.0;
    amplitude *= 0.5;

    value += noise(p) * amplitude;

    return value;
}


// =========================================================
// TERRAIN
// =========================================================

float terrainNoise(vec3 direction)
{
    float largeScale =
        fbm(direction * 3.0);

    float mediumScale =
        fbm(
            direction * 7.0 +
            vec3(17.3, 4.1, 9.7)
        );

    float smallScale =
        fbm(
            direction * 15.0 +
            vec3(31.7, 12.4, 5.8)
        );

    return
        largeScale * 0.65 +
        mediumScale * 0.25 +
        smallScale * 0.10;
}


// =========================================================
// CRATERAS
// =========================================================

vec3 ApplyCraterColor(
    vec3 surfaceColor,
    vec3 surfaceDirection
)
{
    vec3 result = surfaceColor;


    // =====================================================
    // PROCESSA CADA CRATERA
    // =====================================================

    for (int i = 0; i < MAX_DEFORMATIONS; i++)
    {
        if (i >= deformationCount)
            break;


        // =================================================
        // DADOS DA CRATERA
        // =================================================

        vec3 craterDirection =
            normalize(
                deformationData[i].xyz
            );

        float craterRadius =
            deformationData[i].w * 0.5;


        if (craterRadius <= 0.0001)
            continue;


        // =================================================
        // DISTÂNCIA ANGULAR
        // =================================================

        float d =
            dot(
                surfaceDirection,
                craterDirection
            );

        d =
            clamp(
                d,
                -1.0,
                1.0
            );

        float angle =
            acos(d);


        // =================================================
        // FORA DA CRATERA
        // =================================================

        if (angle >= craterRadius)
            continue;


        // =================================================
        // COORDENADA NORMALIZADA
        // =================================================

        float t =
            angle /
            craterRadius;


        // =================================================
        // PERFIL DA CRATERA
        // =================================================
        //
        // 0.00
        //   │
        //   │ fundo
        //   │
        // 0.45
        //   │
        //   │ parede
        //   │
        // 0.75
        //   │
        //   │ borda
        //   │
        // 1.00
        //
        // =================================================


        // -------------------------------------------------
        // FUNDO
        // -------------------------------------------------

        float floorMask =
            1.0 -
            smoothstep(
                0.00,
                0.42,
                t
            );


        // -------------------------------------------------
        // PAREDE INTERNA
        // -------------------------------------------------

        float wallMask =
            smoothstep(
                0.30,
                0.55,
                t
            ) *
            (
                1.0 -
                smoothstep(
                    0.55,
                    0.78,
                    t
                )
            );


        // -------------------------------------------------
        // BORDA
        // -------------------------------------------------
        //
        // Extremamente localizada.
        //
        // Não existe alteração depois de ~0.90.
        //
        // -------------------------------------------------

        float rimMask =
            smoothstep(
                0.70,
                0.82,
                t
            ) *
            (
                1.0 -
                smoothstep(
                    0.82,
                    0.94,
                    t
                )
            );


        // =================================================
        // CORES
        // =================================================

        vec3 floorColor =
            vec3(
                0.055,
                0.048,
                0.040
            );

        vec3 wallColor =
            vec3(
                0.115,
                0.105,
                0.090
            );

        vec3 rimColor =
            vec3(
                0.145,
                0.135,
                0.120
            );


        // =================================================
        // INTENSIDADE
        // =================================================

        //
        // Mantemos cada componente relativamente fraco.
        //
        // A iluminação do planeta continua sendo responsável
        // pela aparência final.
        //


        // -------------------------------------------------
        // FUNDO
        // -------------------------------------------------

        result =
            mix(
                result,
                floorColor,
                floorMask * 0.72
            );


        // -------------------------------------------------
        // PAREDE
        // -------------------------------------------------

        result =
            mix(
                result,
                wallColor,
                wallMask * 0.35
            );


        // -------------------------------------------------
        // BORDA
        // -------------------------------------------------

        result =
            mix(
                result,
                rimColor,
                rimMask * 0.18
            );
    }


    return result;
}


// =========================================================
// CLOUD NOISE
// =========================================================

float cloudNoise(vec3 direction)
{
    float large =
        fbm(
            direction * 4.0 +
            vec3(41.0, 17.0, 23.0)
        );

    float medium =
        fbm(
            direction * 9.0 +
            vec3(12.0, 31.0, 8.0)
        );

    return
        large * 0.70 +
        medium * 0.30;
}


// =========================================================
// MAIN
// =========================================================

void main()
{
    // =====================================================
    // DIREÇÃO DA SUPERFÍCIE
    // =====================================================

    vec3 direction =
        normalize(
            fragWorldPosition -
            planetPosition
        );

    vec3 normal =
        normalize(
            fragWorldNormal
        );


    // =====================================================
    // TERRENO PROCEDURAL
    // =====================================================

    float terrain =
        terrainNoise(direction);


    // =====================================================
    // CONTINENTE
    // =====================================================

    float continent =
        smoothstep(
            0.47,
            0.53,
            terrain
        );


    // =====================================================
    // ALTITUDE
    // =====================================================

    float altitude =
        smoothstep(
            0.50,
            0.78,
            terrain
        );


    // =====================================================
    // CORES DO OCEANO
    // =====================================================

    vec3 deepOcean =
        vec3(
            0.005,
            0.025,
            0.085
        );

    vec3 ocean =
        vec3(
            0.015,
            0.13,
            0.28
        );

    vec3 shallowOcean =
        vec3(
            0.025,
            0.22,
            0.36
        );


    // =====================================================
    // OCEANO
    // =====================================================

    float oceanDepth =
        smoothstep(
            0.18,
            0.47,
            terrain
        );

    vec3 oceanColor =
        mix(
            deepOcean,
            ocean,
            oceanDepth
        );

    float shallowWater =
        smoothstep(
            0.40,
            0.50,
            terrain
        );

    oceanColor =
        mix(
            oceanColor,
            shallowOcean,
            shallowWater
        );


    // =====================================================
    // CORES DA TERRA
    // =====================================================

    vec3 beach =
        vec3(
            0.65,
            0.50,
            0.25
        );

    vec3 grass =
        vec3(
            0.16,
            0.38,
            0.10
        );

    vec3 forest =
        vec3(
            0.035,
            0.19,
            0.055
        );

    vec3 rock =
        vec3(
            0.30,
            0.29,
            0.27
        );

    vec3 snow =
        vec3(
            0.88,
            0.91,
            0.94
        );


    // =====================================================
    // BIOMAS
    // =====================================================

    float lowLand =
        smoothstep(
            0.50,
            0.58,
            terrain
        );

    float highLand =
        smoothstep(
            0.58,
            0.68,
            terrain
        );

    float mountain =
        smoothstep(
            0.68,
            0.78,
            terrain
        );

    float peak =
        smoothstep(
            0.78,
            0.90,
            terrain
        );


    vec3 landColor =
        mix(
            beach,
            grass,
            lowLand
        );

    landColor =
        mix(
            landColor,
            forest,
            highLand
        );

    landColor =
        mix(
            landColor,
            rock,
            mountain
        );

    landColor =
        mix(
            landColor,
            snow,
            peak
        );


    // =====================================================
    // VARIAÇÃO DE TERRENO
    // =====================================================

    float variation =
        fbm(
            direction * 25.0 +
            vec3(8.2, 13.7, 21.4)
        );

    variation =
        variation * 0.10 -
        0.05;

    landColor +=
        variation;


    // =====================================================
    // COR DA SUPERFÍCIE
    // =====================================================

    vec3 surfaceColor =
        mix(
            oceanColor,
            landColor,
            continent
        );


    // =====================================================
    // CRATERAS
    // =====================================================

    surfaceColor =
        ApplyCraterColor(
            surfaceColor,
            direction
        );


    // =====================================================
    // LUZ DO SOL
    // =====================================================

    vec3 lightDirection =
        normalize(
            SUN_DIRECTION
        );

    float NdotL =
        dot(
            normal,
            lightDirection
        );


    // =====================================================
    // DIA / NOITE
    // =====================================================

    float day =
        smoothstep(
            -0.20,
            0.35,
            NdotL
        );


    // =====================================================
    // LUZ DIRETA
    // =====================================================

    float directLight =
        max(
            NdotL,
            0.0
        );

    directLight =
        smoothstep(
            0.0,
            0.85,
            directLight
        );

    vec3 directLighting =
        SUN_COLOR *
        directLight *
        0.85;


    // =====================================================
    // LUZ AMBIENTE
    // =====================================================

    float skyFactor =
        smoothstep(
            -0.35,
            0.85,
            normal.y
        );

    vec3 ambientColor =
        mix(
            NIGHT_LIGHT,
            SKY_LIGHT,
            skyFactor
        );

    float ambientStrength =
        mix(
            0.45,
            0.85,
            day
        );

    vec3 ambientLighting =
        ambientColor *
        ambientStrength;


    // =====================================================
    // ILUMINAÇÃO FINAL DA SUPERFÍCIE
    // =====================================================

    vec3 lighting =
        ambientLighting +
        directLighting;

    vec3 finalSurface =
        surfaceColor *
        lighting;


    // =====================================================
    // OCEAN SPECULAR
    // =====================================================

    float oceanMask =
        1.0 - continent;

    float oceanSpecular =
        pow(
            max(
                NdotL,
                0.0
            ),
            24.0
        );

    oceanSpecular *=
        oceanMask *
        day;

    finalSurface +=
        vec3(
            0.20,
            0.42,
            0.65
        ) *
        oceanSpecular *
        0.35;


    // =====================================================
    // CLOUDS
    // =====================================================

    float clouds =
        cloudNoise(direction);

    float cloudMask =
        smoothstep(
            0.58,
            0.73,
            clouds
        );

    cloudMask *=
        continent *
        day *
        0.25;

    vec3 cloudColor =
        vec3(
            0.92,
            0.94,
            0.96
        );

    finalSurface =
        mix(
            finalSurface,
            cloudColor * lighting,
            cloudMask
        );


    // =====================================================
    // ATMOSFERA / FRESNEL
    // =====================================================

    float rim =
        pow(
            1.0 -
            abs(
                dot(
                    normal,
                    normalize(-direction)
                )
            ),
            3.0
        );


    // =====================================================
    // ATMOSFERA AZUL
    // =====================================================

    vec3 atmosphereColor =
        vec3(
            0.04,
            0.22,
            0.70
        );

    float atmosphereStrength =
        rim *
        (
            0.25 +
            day * 0.75
        );

    finalSurface +=
        atmosphereColor *
        atmosphereStrength *
        0.30;


    // =====================================================
    // BRILHO QUENTE NO TERMINADOR
    // =====================================================

    float terminator =
        smoothstep(
            -0.20,
            0.15,
            NdotL
        ) *
        (
            1.0 -
            smoothstep(
                0.15,
                0.55,
                NdotL
            )
        );

    finalSurface +=
        vec3(
            1.0,
            0.35,
            0.08
        ) *
        terminator *
        rim *
        0.12;


    // =====================================================
    // CONTRASTE FINAL
    // =====================================================

    finalSurface =
        max(
            finalSurface,
            vec3(0.0)
        );


    // =====================================================
    // GAMMA
    // =====================================================

    finalSurface =
        pow(
            finalSurface,
            vec3(1.0 / 2.2)
        );


    // =====================================================
    // RESULTADO
    // =====================================================

    finalColor =
        vec4(
            finalSurface,
            1.0
        );
}

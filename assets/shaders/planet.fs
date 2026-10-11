#version 330

in vec3 fragWorldPosition;
in vec3 fragWorldNormal;

uniform vec3 planetPosition;
uniform float planetRadius;

#define MAX_DEFORMATIONS 1000

uniform int deformationCount;
uniform vec4 deformationData[MAX_DEFORMATIONS];

out vec4 finalColor;

// Iluminação


const vec3 SUN_DIRECTION =
    normalize(vec3(1.0, 1.0, 0.8));

const vec3 SUN_COLOR =
    vec3(1.0, 0.92, 0.78);

const vec3 SKY_LIGHT =
    vec3(0.10, 0.16, 0.28);

const vec3 NIGHT_LIGHT =
    vec3(0.008, 0.015, 0.035);

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



// Terreno

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



// Cratera



vec3 ApplyCraterColor(
    vec3 surfaceColor,
    vec3 surfaceDirection
)
{
    vec3 result = surfaceColor;

    for (int i = 0; i < deformationCount; i++)
    {
        vec4 data = deformationData[i];

        // A direção já deve estar normalizada no C.
        vec3 craterDirection = data.xyz;
        float craterRadius = data.w * 0.5;

        if (craterRadius <= 0.0001)
            continue;

        float d = dot(surfaceDirection, craterDirection);

        if (d < cos(craterRadius))
            continue;

        float angle = acos(clamp(d, -1.0, 1.0));
        float t = angle / craterRadius;

        float floorMask =
            1.0 - smoothstep(0.00, 0.42, t);

        float wallMask =
            smoothstep(0.30, 0.55, t) *
            (1.0 - smoothstep(0.55, 0.78, t));

        float rimMask =
            smoothstep(0.70, 0.82, t) *
            (1.0 - smoothstep(0.82, 0.94, t));

        result = mix(
            result,
            vec3(0.055, 0.048, 0.040),
            floorMask * 0.72
        );

        result = mix(
            result,
            vec3(0.115, 0.105, 0.090),
            wallMask * 0.35
        );

        result = mix(
            result,
            vec3(0.145, 0.135, 0.120),
            rimMask * 0.18
        );
    }

    return result;
}

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


void main()
{
    
    // Direção da superficie
    

    vec3 direction =
        normalize(
            fragWorldPosition -
            planetPosition
        );

    vec3 normal =
        normalize(
            fragWorldNormal
        );

    float terrain =
        terrainNoise(direction);

    float continent =
        smoothstep(
            0.47,
            0.53,
            terrain
        );

    float altitude =
        smoothstep(
            0.50,
            0.78,
            terrain
        );

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

    vec3 surfaceColor =
        mix(
            oceanColor,
            landColor,
            continent
        );

    surfaceColor =
        ApplyCraterColor(
            surfaceColor,
            direction
        );

    vec3 lightDirection =
        normalize(
            SUN_DIRECTION
        );

    float NdotL =
        dot(
            normal,
            lightDirection
        );

    float day =
        smoothstep(
            -0.20,
            0.35,
            NdotL
        );

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

    vec3 lighting =
        ambientLighting +
        directLighting;

    vec3 finalSurface =
        surfaceColor *
        lighting;

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




    float cloudMask = 0.0;

    if (day > 0.05 && continent > 0.05)
    {
        float clouds = cloudNoise(direction);

        cloudMask =
            smoothstep(0.58, 0.73, clouds) *
            continent *
            day *
            0.25;
    }

    vec3 cloudColor = vec3(0.92, 0.94, 0.96);

    finalSurface = mix(
        finalSurface,
        cloudColor * lighting,
        cloudMask
    );




    finalSurface =
        mix(
            finalSurface,
            cloudColor * lighting,
            cloudMask
        );

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

    finalSurface =
        max(
            finalSurface,
            vec3(0.0)
        );

    finalSurface =
        pow(
            finalSurface,
            vec3(1.0 / 2.2)
        );

    finalColor =
        vec4(
            finalSurface,
            1.0
        );
}

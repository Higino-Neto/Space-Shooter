#version 330 core

// -----------------------------------------------------------------------------
// Planet surface fragment shader
// GLSL 3.30 core
//
// Mantém as entradas e os dados de deformação do shader original. A única
// uniform nova é cameraPosition, usada para Fresnel/atmosfera e reflexo da água.
//
// deformationData[i].xyz : direção normalizada do centro da cratera
// deformationData[i].w   : DIÂMETRO angular da cratera, em radianos
//                          (o raio continua sendo data.w * 0.5, como no original)
// -----------------------------------------------------------------------------

in vec3 fragWorldPosition;
in vec3 fragWorldNormal;

uniform vec3 planetPosition;
uniform float planetRadius;
uniform vec3 cameraPosition;

#define MAX_DEFORMATIONS 1000
uniform int deformationCount;
uniform vec4 deformationData[MAX_DEFORMATIONS];

out vec4 finalColor;

const float PI = 3.14159265359;

// Direção e cor artística da iluminação principal.
const vec3 SUN_DIRECTION = vec3(0.615457, 0.615457, 0.492366);
const vec3 SUN_COLOR = vec3(1.0, 0.94, 0.82);
const vec3 SKY_LIGHT = vec3(0.105, 0.145, 0.205);
const vec3 NIGHT_LIGHT = vec3(0.009, 0.014, 0.030);

// -----------------------------------------------------------------------------
// Noise 3D contínuo: evita as emendas que apareceriam com coordenadas 2D numa
// esfera. O fade quintic deixa a transição entre células mais suave.
// -----------------------------------------------------------------------------

float hash31(vec3 p)
{
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise3D(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    float n000 = hash31(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));

    float x00 = mix(n000, n100, f.x);
    float x10 = mix(n010, n110, f.x);
    float x01 = mix(n001, n101, f.x);
    float x11 = mix(n011, n111, f.x);

    float y0 = mix(x00, x10, f.y);
    float y1 = mix(x01, x11, f.y);
    return mix(y0, y1, f.z);
}

float fbm(vec3 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    float totalAmplitude = 0.0;

    for (int octave = 0; octave < 3; ++octave)
    {
        value += noise3D(p) * amplitude;
        totalAmplitude += amplitude;
        p = p * 2.03 + vec3(13.1, 7.7, 19.3);
        amplitude *= 0.5;
    }

    return value / max(totalAmplitude, 0.0001);
}

// -----------------------------------------------------------------------------
// Terreno e biomas
// -----------------------------------------------------------------------------

float terrainNoise(vec3 direction)
{
    float largeScale = fbm(direction * 3.1);
    float mediumScale = fbm(direction * 7.4 + vec3(17.3, 4.1, 9.7));
    float smallScale = fbm(direction * 15.5 + vec3(31.7, 12.4, 5.8));

    return largeScale * 0.64 + mediumScale * 0.26 + smallScale * 0.10;
}

vec3 getPlanetAlbedo(vec3 direction, float terrain, out float landMask)
{
    // Continentes com transição costeira um pouco mais orgânica.
    landMask = smoothstep(0.485, 0.545, terrain);

    float oceanDepth = smoothstep(0.13, 0.48, terrain);
    vec3 deepOcean = vec3(0.004, 0.018, 0.060);
    vec3 ocean = vec3(0.010, 0.085, 0.205);
    vec3 shallowOcean = vec3(0.025, 0.205, 0.285);

    vec3 oceanColor = mix(deepOcean, ocean, oceanDepth);
    oceanColor = mix(oceanColor, shallowOcean, smoothstep(0.405, 0.505, terrain));

    // Faixas de altitude para os biomas terrestres.
    float lowland = smoothstep(0.495, 0.585, terrain);
    float upland = smoothstep(0.565, 0.655, terrain);
    float mountain = smoothstep(0.655, 0.765, terrain);
    float peak = smoothstep(0.755, 0.895, terrain);

    vec3 beach = vec3(0.57, 0.43, 0.235);
    vec3 grass = vec3(0.105, 0.285, 0.075);
    vec3 forest = vec3(0.025, 0.115, 0.045);
    vec3 rock = vec3(0.245, 0.225, 0.195);
    vec3 snow = vec3(0.79, 0.84, 0.89);

    vec3 landColor = mix(beach, grass, lowland);
    landColor = mix(landColor, forest, upland * 0.78);
    landColor = mix(landColor, rock, mountain);

    // Neve em grandes altitudes e em regiões polares.
    float latitude = abs(direction.y);
    float polarSnow = smoothstep(0.73, 0.96, latitude) * 0.82;
    float mountainSnow = peak * 0.92;
    landColor = mix(landColor, snow, max(polarSnow, mountainSnow));

    float detail = fbm(direction * 27.0 + vec3(8.2, 13.7, 21.4));
    landColor *= 0.94 + detail * 0.13;

    return mix(oceanColor, landColor, landMask);
}

float cloudNoise(vec3 direction)
{
    float broad = fbm(direction * 4.2 + vec3(41.0, 17.0, 23.0));
    float detail = fbm(direction * 10.5 + vec3(12.0, 31.0, 8.0));
    return broad * 0.72 + detail * 0.28;
}


// -----------------------------------------------------------------------------
// Microdetalhe de superfície
//
// Três frequências dão textura em escalas diferentes. As derivadas de tela
// convertem a altura procedural em uma normal perturbada (bump mapping),
// sem exigir subdivisões adicionais na malha.
// -----------------------------------------------------------------------------

float surfaceMicroHeight(vec3 direction)
{
    float footprint = max(length(dFdx(direction)), length(dFdy(direction)));

    float fadeLow  = 1.0 - smoothstep(0.35, 0.95, footprint * 72.0);
    float fadeMid  = 1.0 - smoothstep(0.35, 0.95, footprint * 168.0);
    float fadeHigh = 1.0 - smoothstep(0.30, 0.85, footprint * 340.0);

    float low  = noise3D(direction * 72.0 + vec3(4.7, 18.2, 9.1));
    float mid  = noise3D(direction * 168.0 + vec3(21.3, 5.8, 14.6));
    float high = noise3D(direction * 340.0 + vec3(7.2, 29.1, 3.4));

    return (low - 0.5) * 0.010 * fadeLow
         + (mid - 0.5) * 0.0045 * fadeMid
         + (high - 0.5) * 0.0018 * fadeHigh;
}

vec3 applyMicroNormal(vec3 position, vec3 normal, float height)
{
    vec3 dpdx = dFdx(position);
    vec3 dpdy = dFdy(position);
    vec3 r1 = cross(dpdy, normal);
    vec3 r2 = cross(normal, dpdx);
    float determinant = dot(dpdx, r1);

    if (abs(determinant) < 0.000001)
        return normal;

    vec3 gradient = dFdx(height) * r1 + dFdy(height) * r2;
    gradient *= sign(determinant);

    return normalize(abs(determinant) * normal - gradient * 1.65);
}

// -----------------------------------------------------------------------------
// Crateras
//
// Em vez de pintar um disco, cada cratera possui:
//   - fundo relativamente plano e escuro;
//   - parede inclinada;
//   - borda elevada e iluminável;
//   - depósito externo de material ejetado;
//   - normal analítica para dar leitura de relevo mesmo em malhas pouco densas.
//
// A normal analítica afeta apenas a iluminação do fragmento. Ela não desloca a
// malha nem altera a silhueta do planeta.
// -----------------------------------------------------------------------------

float craterHeightProfile(float t, float radius)
{
    // Depressão central em formato de taça.
    float bowl = 1.0 - smoothstep(0.0, 1.0, t);

    // Lábio muito discreto, sem formar uma borda exagerada.
    float lipX = (t - 0.94) / 0.13;
    float lip = exp(-lipX * lipX);

    return radius * (
        -0.085 * bowl +
         0.006 * lip
    );
}

void applyCraters(
    vec3 direction,
    float landMask,
    inout vec3 surfaceColor,
    inout vec3 surfaceNormal
)
{
    float floorAmount = 0.0;
    float wallAmount = 0.0;
    float lipAmount = 0.0;
    float ejectaAmount = 0.0;

    vec3 craterNormalSum = vec3(0.0);
    float craterNormalWeight = 0.0;

    int count = clamp(deformationCount, 0, MAX_DEFORMATIONS);

    for (int i = 0; i < count; ++i)
    {
        vec4 data = deformationData[i];

        float directionLength = length(data.xyz);
        float radius = data.w * 0.5;

        if (directionLength < 0.0001 || radius < 0.0001)
            continue;

        vec3 craterDirection = data.xyz / directionLength;

        float d = clamp(
            dot(direction, craterDirection),
            -1.0,
            1.0
        );

        if (d < cos(radius * 1.4))
            continue;

        float t = acos(d) / radius;

        // Pequenas irregularidades naturais no contorno.
        float broadNoise = noise3D(
            direction * 24.0 + craterDirection * 8.0
        );

        float fineNoise = noise3D(
            direction * 65.0 - craterDirection * 11.0
        );

        float edgeNoise =
            (broadNoise - 0.5) * 0.055 +
            (fineNoise - 0.5) * 0.025;

        float shapedT = t +
            edgeNoise * smoothstep(0.2, 0.85, t);

        // Fundo relativamente plano e irregular.
        float floorMask =
            1.0 - smoothstep(0.20, 0.52, shapedT);

        // Paredes inclinadas.
        float wallMask =
            smoothstep(0.30, 0.48, shapedT) *
            (1.0 - smoothstep(0.76, 1.0, shapedT));

        // Lábio quase imperceptível.
        float lipX = (shapedT - 0.94) / 0.13;

        float lipMask =
            exp(-lipX * lipX) * 0.22;

        // Material ejetado irregular, sem formar uma auréola.
        float ejectaNoise = noise3D(
            direction * 38.0 + craterDirection * 17.0
        );

        float ejectaPatches =
            smoothstep(0.48, 0.78, ejectaNoise);

        float ejectaFalloff =
            1.0 - smoothstep(1.0, 1.45, shapedT);

        float ejectaMask =
            ejectaPatches *
            ejectaFalloff *
            smoothstep(0.96, 1.08, shapedT) *
            0.24;

        floorAmount = max(floorAmount, floorMask);
        wallAmount = max(wallAmount, wallMask);
        lipAmount = max(lipAmount, lipMask);
        ejectaAmount = max(ejectaAmount, ejectaMask);

        // Normal analítica da depressão.
        const float stepSize = 0.006;

        float h1 = craterHeightProfile(
            shapedT + stepSize,
            radius
        );

        float h2 = craterHeightProfile(
            shapedT - stepSize,
            radius
        );

        float slope = (h1 - h2) /
            (2.0 * stepSize * radius);

        vec3 tangent =
            direction - craterDirection * d;

        float tangentLength = length(tangent);

        if (tangentLength > 0.00001)
        {
            vec3 tangentDirection =
                tangent / tangentLength;

            vec3 localNormal = normalize(
                direction - tangentDirection * slope
            );

            float weight = max(floorMask, wallMask);
            weight = max(weight, lipMask * 0.2);

            craterNormalSum += localNormal * weight;
            craterNormalWeight += weight;
        }
    }

    // Cores de solo terrestre: terra escura, rocha e sedimentos.
    vec3 floorColor = vec3(0.105, 0.078, 0.055);
    vec3 wallColor  = vec3(0.235, 0.177, 0.118);
    vec3 lipColor   = vec3(0.265, 0.205, 0.142);
    vec3 ejectaColor = vec3(0.195, 0.145, 0.095);

    // Variação mineral e de sedimentos.
    float floorVariation =
        0.88 + noise3D(direction * 57.0) * 0.24;

    float wallVariation =
        0.86 + noise3D(direction * 43.0 + vec3(4.2, 8.1, 2.3)) * 0.28;

    floorColor *= floorVariation;
    wallColor *= wallVariation;
    lipColor *= wallVariation;

    // Mistura progressiva com o terreno original.
    surfaceColor = mix(
        surfaceColor,
        floorColor,
        floorAmount * landMask * 0.78
    );

    surfaceColor = mix(
        surfaceColor,
        wallColor,
        wallAmount * landMask * 0.55
    );

    surfaceColor = mix(
        surfaceColor,
        lipColor,
        lipAmount * landMask * 0.28
    );

    surfaceColor = mix(
        surfaceColor,
        ejectaColor,
        ejectaAmount * landMask
    );

    // Integração da normal da cratera com a normal original.
    if (craterNormalWeight > 0.0001)
    {
        vec3 craterNormal = normalize(
            craterNormalSum / craterNormalWeight
        );

        float blend = clamp(
            craterNormalWeight * 0.65,
            0.0,
            0.72
        );

        surfaceNormal = normalize(
            mix(surfaceNormal, craterNormal, blend)
        );
    }
}

// -----------------------------------------------------------------------------
// Iluminação
// -----------------------------------------------------------------------------

void main()
{
    vec3 radial = fragWorldPosition - planetPosition;
    vec3 direction = normalize(radial);

    vec3 geometricNormal = normalize(fragWorldNormal);
    vec3 surfaceNormal = geometricNormal;

    float terrain = terrainNoise(direction);
    float landMask;
    vec3 surfaceColor = getPlanetAlbedo(direction, terrain, landMask);

    // As crateras alteram albedo e normal de iluminação, não a posição da malha.
    applyCraters(direction, landMask, surfaceColor, surfaceNormal);

    // Granulação mineral e variação fina de cor, com atenuação automática
    // quando o detalhe fica menor que um pixel.
    float microHeight = surfaceMicroHeight(direction);
    float microColor = noise3D(direction * 112.0 + vec3(16.4, 3.7, 25.1));
    float mineralVariation = (microColor - 0.5) * 0.075;
    surfaceColor *= 1.0 + mineralVariation;
    surfaceNormal = applyMicroNormal(
        fragWorldPosition,
        surfaceNormal,
        microHeight
    );

    vec3 lightDirection = normalize(SUN_DIRECTION);
    vec3 viewDirection = normalize(cameraPosition - fragWorldPosition);

    float nDotL = dot(surfaceNormal, lightDirection);
    float diffuse = max(nDotL, 0.0);
    float daylight = smoothstep(-0.18, 0.20, dot(direction, lightDirection));

    // Luz ambiente hemisférica: preserva detalhe no lado noturno sem apagar
    // completamente o contraste entre dia e noite.
    float skyHemisphere = smoothstep(-0.35, 0.78, surfaceNormal.y);
    vec3 ambient = mix(NIGHT_LIGHT, SKY_LIGHT, skyHemisphere);
    ambient *= mix(0.58, 1.0, daylight);

    vec3 direct = SUN_COLOR * diffuse * 0.92;
    vec3 lighting = ambient + direct;
    vec3 finalSurface = surfaceColor * lighting;

    // Reflexo especular da água dependente da câmera, restrito aos oceanos.
    float oceanMask = 1.0 - landMask;
    vec3 halfDirection = normalize(lightDirection + viewDirection);
    float waterSpecular = pow(max(dot(surfaceNormal, halfDirection), 0.0), 88.0);
    float waterFresnel = 0.025 + 0.22 * pow(1.0 - max(dot(surfaceNormal, viewDirection), 0.0), 5.0);
    finalSurface += vec3(0.22, 0.43, 0.62) * waterSpecular * waterFresnel * oceanMask * daylight;

    // Nuvens em toda a superfície, não apenas sobre os continentes.
    float clouds = cloudNoise(direction);
    float cloudMask = smoothstep(0.585, 0.715, clouds);
    cloudMask *= 0.42;

    // Sombra atmosférica sutil projetada pelas nuvens sobre a superfície.
    finalSurface *= 1.0 - cloudMask * diffuse * 0.20;
    vec3 cloudLighting = (ambient + SUN_COLOR * diffuse * 0.82) * vec3(0.96, 0.975, 1.0);
    finalSurface = mix(finalSurface, vec3(0.91, 0.94, 0.98) * cloudLighting, cloudMask);

    // Atmosfera Fresnel: depende da posição real da câmera, ao contrário de
    // uma aproximação baseada apenas na normal e no eixo do planeta.
    float nDotV = max(dot(surfaceNormal, viewDirection), 0.0);
    float fresnel = pow(1.0 - nDotV, 3.4);
    float atmosphereDay = 0.20 + 0.80 * max(dot(direction, lightDirection), 0.0);
    vec3 atmosphereColor = mix(
        vec3(0.012, 0.030, 0.105),
        vec3(0.035, 0.205, 0.66),
        atmosphereDay
    );
    finalSurface += atmosphereColor * fresnel * 0.42;

    // Brilho quente muito discreto no terminador, para dar separação entre dia
    // e noite sem transformar toda a borda do planeta em um contorno laranja.
    float terminator = smoothstep(-0.16, 0.02, nDotL) *
                       (1.0 - smoothstep(0.02, 0.28, nDotL));
    finalSurface += vec3(1.0, 0.31, 0.10) * terminator * fresnel * 0.055;

    // Tone floor + conversão aproximada de linear para display sRGB.
    finalSurface = max(finalSurface, vec3(0.0));
    finalSurface = pow(finalSurface, vec3(1.0 / 2.2));

    finalColor = vec4(finalSurface, 1.0);
}

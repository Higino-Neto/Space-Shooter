#include "ui.h"

#include <stdio.h>

static Texture2D healthBarTexture;
static Texture2D maxHealthBarTexture;

static Texture2D planetBarTexture;
static Texture2D maxPlanetBarTexture;

static Texture2D xpBarTexture;
static Texture2D maxXpBarTexture;

static bool uiInitialized = false;

static const Color UI_BLUE = { 54, 197, 232, 255 };
static const Color UI_GRAY = { 17, 29, 43, 255 };
static const Color UI_RED = { 230, 41, 55, 255 };

void UIInit(void)
{
    // healthBarTexture = LoadTexture(
    //     "assets/ui/Bars/Blue/x1/Asset 6.png");

    // maxHealthBarTexture = LoadTexture(
    //     "assets/ui/Bars/Blue/x1/Asset 1.png");

    // planetBarTexture = LoadTexture(
    //     "assets/ui/Bars/Purple/Bar 2/0.5x/Asset 11@0.5x.png");

    // maxPlanetBarTexture = LoadTexture(
    //     "assets/ui/Bars/Purple/Bar 2/0.5x/Asset 7@0.5x.png");

    // xpBarTexture = LoadTexture(
    //     "assets/ui/Bars/Green/x1/Asset 11 - Copy.png");

    // maxXpBarTexture = LoadTexture(
    //     "assets/ui/Bars/Green/x1/Asset 7 - Copy.png");

    healthBarTexture = LoadTexture(
        "assets/ui/Internal/playerEmptyHealthBar.png");

    maxHealthBarTexture = LoadTexture(
        "assets/ui/Internal/playerFullHealthBar.png");

    planetBarTexture = LoadTexture(
        "assets/ui/Internal/planetEmptyHealthBar.png");

    maxPlanetBarTexture = LoadTexture(
        "assets/ui/Internal/planetFullHealthBar.png");

    xpBarTexture = LoadTexture(
        "assets/ui/Internal/levelEmptyBarV2.png");

    maxXpBarTexture = LoadTexture(
        "assets/ui/Internal/levelFullBarV2.png");

    uiInitialized = true;
}

static float UIGetHeight(Texture2D texture, float width)
{
    float ratio = (float)texture.width / (float)texture.height;
    return width / ratio;
}

static void UIDrawBar(
    Texture2D emptyTexture,
    Texture2D fullTexture,
    float x,
    float y,
    float width,
    float height,
    float current,
    float maximum)
{
    if (maximum <= 0.0f)
        return;

    float ratio = current / maximum;

    if (ratio < 0.0f)
        ratio = 0.0f;

    if (ratio > 1.0f)
        ratio = 1.0f;

    Rectangle destination = {
        x, y, width, height};

    DrawTexturePro(
        emptyTexture,
        (Rectangle){
            0, 0,
            (float)emptyTexture.width,
            (float)emptyTexture.height},
        destination,
        (Vector2){0, 0},
        0.0f,
        WHITE);

    DrawTexturePro(
        fullTexture,
        (Rectangle){
            0, 0,
            fullTexture.width * ratio,
            (float)fullTexture.height},
        (Rectangle){
            x, y,
            width * ratio,
            height},
        (Vector2){0, 0},
        0.0f,
        WHITE);
}

void UIDrawHUD(
    float playerHealth,
    float playerMaxHealth,
    float planetHealth,
    float planetMaxHealth,
    int level,
    float currentXP,
    float requiredXP)
{
    if (!uiInitialized)
        return;

    int planetWidth = 620;
    int healthWidth = 480;
    int xpWidth = 360;

    UIDrawBar(
        planetBarTexture,
        maxPlanetBarTexture,
        30, 30,
        planetWidth,
        UIGetHeight(planetBarTexture, planetWidth),
        planetHealth,
        planetMaxHealth);

    UIDrawBar(
        healthBarTexture,
        maxHealthBarTexture,
        30, 130,
        healthWidth,
        UIGetHeight(healthBarTexture, healthWidth),
        playerHealth,
        playerMaxHealth);

    UIDrawBar(
        xpBarTexture,
        maxXpBarTexture,
        30, 250,
        xpWidth,
        UIGetHeight(xpBarTexture, xpWidth),
        currentXP,
        requiredXP);

    DrawText(
        TextFormat("Level %d", level),
        72, 328,
        16,
        WHITE);
}

void UIDrawWaveBanner(
    int waveNumber,
    float timer)
{
    if (waveNumber <= 0 || timer <= 0.0f)
        return;

    float fade = timer < 1.0f ? timer : 1.0f;

    unsigned char textAlpha =
        (unsigned char)(255.0f * fade);

    unsigned char panelAlpha =
        (unsigned char)(205.0f * fade);

    bool bossWave = waveNumber % 5 == 0;

    Color accent = bossWave ? UI_RED : UI_BLUE;
    const char *subtitle = bossWave
        ? "BOSS"
        : "ENEMIES";

    char title[32];
    snprintf(title, sizeof(title), "WAVE %d", waveNumber);

    float width = 420.0f;
    float height = 92.0f;
    float x = (GetScreenWidth() - width) * 0.5f;
    float y = 90.0f;

    Rectangle panel = { x, y, width, height };

    DrawRectangleRounded(
        panel,
        0.18f,
        8,
        (Color){
            UI_GRAY.r,
            UI_GRAY.g,
            UI_GRAY.b,
            panelAlpha
        });

    DrawRectangleLinesEx(
        panel,
        1.0f,
        (Color){
            accent.r,
            accent.g,
            accent.b,
            (unsigned char)(110.0f * fade)
        });

    int titleWidth = MeasureText(title, 38);

    DrawText(
        title,
        (int)(x + (width - titleWidth) * 0.5f),
        (int)y + 15,
        38,
        (Color){
            UI_BLUE.r,
            UI_BLUE.g,
            UI_BLUE.b,
            textAlpha
        });

    int subtitleWidth = MeasureText(subtitle, 12);

    DrawText(
        subtitle,
        (int)(x + (width - subtitleWidth) * 0.5f),
        (int)y + 64,
        12,
        (Color){
            accent.r,
            accent.g,
            accent.b,
            (unsigned char)(220.0f * fade)
        });
}

void UIShutdown(void)
{
    if (!uiInitialized)
        return;

    UnloadTexture(healthBarTexture);
    UnloadTexture(planetBarTexture);
    UnloadTexture(xpBarTexture);

    uiInitialized = false;
}

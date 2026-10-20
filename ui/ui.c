#include "ui.h"

#include <stdio.h>

static Texture2D healthBarTexture;
static Texture2D maxHealthBarTexture;

static Texture2D planetBarTexture;
static Texture2D maxPlanetBarTexture;

static Texture2D xpBarTexture;
static Texture2D maxXpBarTexture;

static bool uiInitialized = false;

void UIInit(void) {
    healthBarTexture = LoadTexture(
        "assets/ui/Bars/Blue/x1/Asset 6.png"
    );

    maxHealthBarTexture = LoadTexture(
        "assets/ui/Bars/Blue/x1/Asset 1.png"
    );


    planetBarTexture = LoadTexture(
        "assets/ui/Bars/Purple/Bar 2/0.5x/Asset 11@0.5x.png"
    );

    maxPlanetBarTexture = LoadTexture(
        "assets/ui/Bars/Purple/Bar 2/0.5x/Asset 7@0.5x.png"
    );

    xpBarTexture = LoadTexture(
        "assets/ui/Bars/Green/x1/Asset 11 - Copy.png"
    );

    maxXpBarTexture = LoadTexture(
        "assets/ui/Bars/Green/x1/Asset 7 - Copy.png"
    );

    uiInitialized = true;
}

static void UIDrawBar(
    Texture2D emptyTexture,
    Texture2D fullTexture,
    float x,
    float y,
    float width,
    float height,
    float current,
    float maximum
)
{
    if (maximum <= 0.0f)
        return;

    float ratio = current / maximum;

    if (ratio < 0.0f)
        ratio = 0.0f;

    if (ratio > 1.0f)
        ratio = 1.0f;

    Rectangle destination = {
        x, y, width, height
    };

    DrawTexturePro(
        emptyTexture,
        (Rectangle){
            0, 0,
            (float)emptyTexture.width,
            (float)emptyTexture.height
        },
        destination,
        (Vector2){0, 0},
        0.0f,
        WHITE
    );

    DrawTexturePro(
        fullTexture,
        (Rectangle){
            0, 0,
            fullTexture.width * ratio,
            (float)fullTexture.height
        },
        (Rectangle){
            x, y,
            width * ratio,
            height
        },
        (Vector2){0, 0},
        0.0f,
        WHITE
    );
}

void UIDrawHUD(
    float playerHealth,
    float playerMaxHealth,
    float planetHealth,
    float planetMaxHealth,
    int level,
    float currentXP,
    float requiredXP
) {
    if (!uiInitialized)
        return;
    
    UIDrawBar(
        healthBarTexture,
        maxHealthBarTexture,
        30, 30,
        390, 110,
        playerHealth,
        playerMaxHealth
    );

    //  UIDrawBar(
    //     planetBarTexture,
    //     maxPlanetBarTexture,
    //     30, 1000,
    //     1700, 15,
    //     planetHealth,
    //     planetMaxHealth
    // );

    DrawText(
        TextFormat("Level %d", level),
        50, 170,
        20,
        WHITE
    );

    UIDrawBar(
        xpBarTexture,
        maxXpBarTexture,
        30, 195,
        320, 30,
        currentXP,
        requiredXP
    );
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

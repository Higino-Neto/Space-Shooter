
#ifndef UI_H
#define UI_H

#include "raylib.h"

void UIInit(void);

static float UIGetHeight(Texture2D texture, float width);

void UIDrawHUD(
    float playerHealth,
    float playerMaxHealth,
    float planetHealth,
    float planetMaxHealth,
    int level,
    float currentXP,
    float requiredXP
);

void UIDrawWaveBanner(
    int waveNumber,
    float timer
);

void UIShutdown(void);

#endif

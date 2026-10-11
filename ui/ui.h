
#ifndef UI_H
#define UI_H

#include "raylib.h"

void UIInit(void);
void UIDrawHUD(
    float playerHealth,
    float playerMaxHealth,
    float planetHealth,
    float planetMaxHealth,
    int level,
    float currentXP,
    float requiredXP
);
void UIShutdown(void);

#endif

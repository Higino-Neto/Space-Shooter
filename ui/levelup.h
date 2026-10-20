#ifndef LEVELUP_H
#define LEVELUP_H

#include "upgrade.h"

typedef struct {
    bool active;
    Upgrade choices[3];
    int selected;
} LevelUpMenu;

void OpenLevelUpMenu(LevelUpMenu *menu);
void UpgradeLevelUpMenu(LevelUpMenu *menu, Player *player);
void DrawLevelUpMenu(const LevelUpMenu *menu);

#endif
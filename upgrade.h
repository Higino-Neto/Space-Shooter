#ifdef UPGRADE_H
#define UPGRADE_H

#include "player.h"

typedef enum { 
    UPGRADE_SPEED, UPGRADE_FIRE_RATE, UPGRADE_MAX_HEALTH,
    UPGRADE_DAMAGE, UPGRADE_HEAL, UPGRADE_COUNT 
} UpgradeType;

typedef struct {
    UpgradeType type;
    const char *name;
    const char *description;
    void (*apply)(Player *player);
} Upgrade;

const Upgrade *GetUpgradePool(int *count);
void RollUpgradeChoices(Upgrade outChoices[3], int choiceCount);

#endif

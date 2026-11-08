#include "upgrade.h"
#include "player.h"
#include "raylib.h"
#include "raymath.h"

static void ApplySpeed(Player *p) {
    p->speed *= 1.25f;
}

static void ApplyFireRate(Player *p) {
    p->fireCooldown *= 0.85f;
}

static void ApplyMaxHealth(Player *p) {
    p->maxHealth *= 1.25f;
}

static void ApplyDamage(Player *p) {
    p->damage += 10.f;
}

static void ApplyHeal(Player *p) {
    int heal = p->maxHealth / 2;

    if (heal + p->health >= p->maxHealth)
        p->health = p->maxHealth;
    else
        p->health += heal;
   
}

static const Upgrade upgrades[UPGRADE_COUNT] = {
    [UPGRADE_SPEED] = {
        .type = UPGRADE_SPEED,
        .name = "Speed",
        .description = "Increase your speed by 25%",
        .apply = ApplySpeed
    },
    [UPGRADE_FIRE_RATE] = {
        .type = UPGRADE_FIRE_RATE,
        .name = "Fire Rate",
        .description = "Reduce fire cooldown by 15%",
        .apply = ApplyFireRate
    },
    [UPGRADE_MAX_HEALTH] = {
        .type = UPGRADE_MAX_HEALTH,
        .name = "Max Health",
        .description = "Increase your max health by 25%",
        .apply = ApplyMaxHealth
    },
    [UPGRADE_DAMAGE] = {
        .type = UPGRADE_DAMAGE,
        .name = "Damage",
        .description = "Increase your damage by 10",
        .apply = ApplyDamage
    },
    [UPGRADE_HEAL] = {
        .type = UPGRADE_HEAL,
        .name = "Heal",
        .description = "Increase your life by a half",
        .apply = ApplyHeal
    }
};


const Upgrade *GetUpgradePool(int *count) {
    *count = UPGRADE_COUNT; 
    return upgrades;
}

void GetRandomUpgrades(Upgrade choices[UPGRADE_CHOICES]) {
    int count = 0;

    while (count < UPGRADE_CHOICES) {
        int choice = GetRandomValue(0, UPGRADE_COUNT - 1);

        bool alreadyChosen = false;

        for(int i = 0; i < count; i++) {
            if (choices[i].type == choice) {
                alreadyChosen = true;
                break;
            }
        }

        if (!alreadyChosen) {
            choices[count] = upgrades[choice];
            count++;
        }
    }
}
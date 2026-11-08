#include "levelup.h"
#include "raylib.h"
#include "stdio.h"
#include "string.h"

#define CARD_WIDTH 320
#define CARD_HEIGHT 420
#define CARD_GAP 30

static Rectangle GetCardRect(int index) {
    int  totalWidth = 3 * CARD_WIDTH + 2 * CARD_GAP;
    int startX = (GetScreenWidth() - totalWidth) / 2;
    int startY = (GetScreenHeight() - CARD_HEIGHT) / 2;

    return (Rectangle) {
        (float) (startX + index * (CARD_WIDTH + CARD_GAP)),
        (float) startY,
        (float) CARD_WIDTH,
        (float) CARD_HEIGHT
    };
}

static Texture2D cardTexture;

void LevelUpInit(void)
{
    cardTexture = LoadTexture("assets/ui/Internal/levelUpCard.png");
}

void LevelUpShutdown(void)
{
    UnloadTexture(cardTexture);
}

void OpenLevelUpMenu(LevelUpMenu *menu)
{
    GetRandomUpgrades(menu->choices);
    SetMousePosition(40, 40);
    menu->active = true;
    menu->selected = -1;
}

void UpgradeLevelUpMenu(LevelUpMenu *menu, Player *player)
{
    if (!menu->active)
        return;

    Vector2 mouse = GetMousePosition();
    int hovered = -1;

    for(int i = 0; i < 3; i++) {
        if (CheckCollisionPointRec(mouse, GetCardRect(i))) {
            hovered = i;
            break;
        }
    }

    menu->selected = hovered;

    int chosen = -1;

    if (IsKeyPressed(KEY_ONE)) chosen = 0;
    else if (IsKeyPressed(KEY_TWO)) chosen = 1;
    else if (IsKeyPressed(KEY_THREE)) chosen = 2;

    if (chosen < 0 &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        hovered >= 0) {
            chosen = hovered;
        }
    
    if (chosen < 0) return;

    menu->choices[chosen].apply(player);
    player->pendingLevelUps--;

    if (player->pendingLevelUps > 0) {
        GetRandomUpgrades(menu->choices);
        menu->selected = -1;
    } else {
        menu->active = false;
    }
}

static void DrawTextCenteredIn(
    const char *text,
    Rectangle area,
    int fontSize,
    Color color)
{
    int w = MeasureText(text, fontSize);
    int x = (int)(area.x + (area.width - w) / 2.0f);
    int y = (int)(area.y + (area.height - fontSize) / 2.0f);
    DrawText(text, x, y, fontSize, color);
}

static void DrawTextWrappedIn(
    const char *text,
    Rectangle area,
    int fontSize,
    int lineHeight,
    Color color)
{
    char buffer[256];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char line[256] = "";
    float y = area.y;

    char *word = strtok(buffer, " ");
    while (word != NULL)
    {
        char test[256];

        if (line[0] == '\0')
            snprintf(test, sizeof(test), "%s", word);
        else
            snprintf(test, sizeof(test), "%s %s", line, word);

        if (MeasureText(test, fontSize) > area.width && line[0] != '\0')
        {
            if (y + lineHeight > area.y + area.height)
                break;

            DrawText(line, (int)area.x, (int)y, fontSize, color);
            y += lineHeight;
            snprintf(line, sizeof(line), "%s", word);
        }
        else
        {
            snprintf(line, sizeof(line), "%s", test);
        }

        word = strtok(NULL, " ");
    }

    if (line[0] != '\0' && y + lineHeight <= area.y + area.height)
        DrawText(line, (int)area.x, (int)y, fontSize, color);
}

void DrawLevelUpMenu(const LevelUpMenu *menu)
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f));

    const char *title = "LEVEL UP!";
    int titleSize = 40;
    int titleWidth = MeasureText(title, titleSize);
    DrawText(title, (screenWidth - titleWidth) / 2, 100, titleSize, WHITE);

    // int cardWidth = 320;
    // int cardHeight = 420;
    // int gap = 30;

    // int totalWidth = 3 * cardWidth + 2 * gap;
    // int startX = (screenWidth - totalWidth) / 2;
    // int startY = (screenHeight - cardHeight) / 2;

    Color dark = (Color){17, 29, 43, 255};
    Color cyan = (Color){54, 197, 232, 255};

    for (int i = 0; i < 3; i++)
    {
        Rectangle rec = GetCardRect(i);

        DrawTexturePro(
            cardTexture,
            (Rectangle){0, 0,
                        (float)cardTexture.width,
                        (float)cardTexture.height},
            rec,
            (Vector2){0, 0},
            0.0f,
            WHITE);

        Rectangle header = {
            rec.x + rec.width * 0.11f,
            rec.y + rec.height * 0.11f,
            rec.width * 0.73f,
            rec.height * 0.19f};
        DrawTextCenteredIn(menu->choices[i].name, header, 32, dark);

        Rectangle body = {
            rec.x + 10 + rec.width * 0.14f,
            rec.y + rec.height * 0.37f,
            rec.width * 0.74f,
            rec.height * 0.48f};

        BeginScissorMode(
            (int)body.x, (int)body.y,
            (int)body.width, (int)body.height);
        DrawTextWrappedIn(
            menu->choices[i].description,
            body, 20, 24, cyan);
        EndScissorMode();

        // Rectangle footerInstruction = {
        //     rec.x + rec.width * 0.04f + 15,
        //     rec.y - 40 + rec.height * 0.86f,
        //     rec.width * 0.30f,
        //     rec.height * 0.08f};

        Rectangle footerNumber = {
            rec.x + rec.width * 0.04f,
            rec.y + rec.height * 0.86f,
            rec.width * 0.30f,
            rec.height * 0.08f};

        // DrawTextCenteredIn("Type: ", footerInstruction, 20, cyan);

        DrawTextCenteredIn(TextFormat("%d", i + 1), footerNumber, 24, cyan);
    }
}
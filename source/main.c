#include <coreinit/screen.h>
#include <malloc.h>
#include <proc_ui/procui.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vpad/input.h>
#include <whb/proc.h>
#include <sysapp/launch.h>
#include "gamepad_ui_bin.h"

#define RGB(r, g, b) (((uint32_t)(r) << 24) | ((uint32_t)(g) << 16) | ((uint32_t)(b) << 8) | 0xFFu)
#define COLOUR_BG       RGB(12, 22, 34)
#define COLOUR_PANEL    RGB(31, 48, 65)
#define COLOUR_LINE     RGB(130, 151, 170)
#define COLOUR_ACTIVE   RGB(50, 220, 125)
#define COLOUR_ACCENT   RGB(55, 190, 255)
#define COLOUR_TOUCH    RGB(255, 210, 55)
#define COLOUR_DANGER   RGB(255, 85, 85)
#define COLOUR_PRESSED  RGB(255, 32, 42)
#define COLOUR_WHITE    RGB(255, 255, 255)

typedef enum AppPage {
    PAGE_MENU,
    PAGE_SCREEN_TEST,
    PAGE_KEY_TEST,
} AppPage;

typedef struct DisplayInfo {
    OSScreenID id;
    int width;
    int height;
} DisplayInfo;

typedef struct AppState {
    AppPage page;
    int menuItem;
    int colourIndex;
    int screenIntroFrames;
    bool exitRequested;
} AppState;

static const uint32_t kTestColours[] = {
    0x00000000u, 0xFFFFFFFFu, 0xFF0000FFu, 0x00FF00FFu,
    0x0000FFFFu, 0x00FFFFFFu, 0xFF00FFFFu, 0xFFFF00FFu,
    0x808080FFu,
};

#define TEST_COLOUR_COUNT ((int)(sizeof(kTestColours) / sizeof(kTestColours[0])))

static void fillRect(const DisplayInfo *display, int x, int y, int width, int height,
                     uint32_t colour);

static void putText(OSScreenID screen, uint32_t column, uint32_t row, const char *text)
{
    OSScreenPutFontEx(screen, column, row, text);
}

static void putFormatted(OSScreenID screen, uint32_t column, uint32_t row,
                         const char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    putText(screen, column, row, buffer);
}

static void putPixelLabel(const DisplayInfo *display, int x, int y, const char *text)
{
    int width = (int)strlen(text) * 8;
    int column = (x - width) / 16;
    int row = (y - 12) / 24;
    if (column < 0) column = 0;
    if (row < 0) row = 0;
    putText(display->id, (uint32_t)column, (uint32_t)row, text);
}

static const char kSmallFontChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-/:,.&()?";
static const uint8_t kSmallFont[][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{15,16,16,16,16,16,15},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {15,16,16,19,17,17,15},{17,17,17,31,17,17,17},{31,4,4,4,4,4,31},
    {1,1,1,1,17,17,14},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14},
    {0,4,4,31,4,4,0},{0,0,0,31,0,0,0},{1,2,4,8,16,0,0},
    {0,4,4,0,4,4,0},{0,0,0,0,4,4,8},{0,0,0,0,0,6,6},
    {12,18,20,8,21,18,13},{2,4,8,8,8,4,2},{8,4,2,2,2,4,8},
    {14,17,1,2,4,0,4},
};

static const uint8_t *smallGlyph(char character)
{
    if (character >= 'a' && character <= 'z') character -= 'a' - 'A';
    const char *found = strchr(kSmallFontChars, character);
    if (!found) found = strchr(kSmallFontChars, '?');
    return kSmallFont[found - kSmallFontChars];
}

static int smallTextWidth(const char *text, int scale)
{
    size_t length = strlen(text);
    return length ? (int)length * 6 * scale - scale : 0;
}

static void drawSmallText(const DisplayInfo *display, int x, int y, const char *text,
                          int scale, uint32_t colour)
{
    for (; *text; ++text, x += 6 * scale) {
        if (*text == ' ') continue;
        const uint8_t *glyph = smallGlyph(*text);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if (glyph[row] & (1u << (4 - column)))
                    fillRect(display, x + column * scale, y + row * scale,
                             scale, scale, colour);
            }
        }
    }
}

static void drawSmallTextCentred(const DisplayInfo *display, int centreX, int y,
                                 const char *text, int scale, uint32_t colour)
{
    drawSmallText(display, centreX - smallTextWidth(text, scale) / 2,
                  y, text, scale, colour);
}

static void drawSmallFormatted(const DisplayInfo *display, int x, int y, int scale,
                               uint32_t colour, const char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    drawSmallText(display, x, y, buffer, scale, colour);
}

static bool inside(const DisplayInfo *display, int x, int y)
{
    return x >= 0 && y >= 0 && x < display->width && y < display->height;
}

static void pixel(const DisplayInfo *display, int x, int y, uint32_t colour)
{
    if (inside(display, x, y))
        OSScreenPutPixelEx(display->id, (uint32_t)x, (uint32_t)y, colour);
}

static void fillRect(const DisplayInfo *display, int x, int y, int width, int height,
                     uint32_t colour)
{
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + width > display->width ? display->width : x + width;
    int y1 = y + height > display->height ? display->height : y + height;
    for (int py = y0; py < y1; ++py)
        for (int px = x0; px < x1; ++px) pixel(display, px, py, colour);
}

static void outlineRect(const DisplayInfo *display, int x, int y, int width, int height,
                        uint32_t colour)
{
    for (int px = x; px < x + width; ++px) {
        pixel(display, px, y, colour);
        pixel(display, px, y + height - 1, colour);
    }
    for (int py = y; py < y + height; ++py) {
        pixel(display, x, py, colour);
        pixel(display, x + width - 1, py, colour);
    }
}

static void line(const DisplayInfo *display, int x0, int y0, int x1, int y1,
                 uint32_t colour)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        pixel(display, x0, y0, colour);
        if (x0 == x1 && y0 == y1) break;
        int twice = error * 2;
        if (twice >= dy) { error += dy; x0 += sx; }
        if (twice <= dx) { error += dx; y0 += sy; }
    }
}

static void fillCircle(const DisplayInfo *display, int cx, int cy, int radius,
                       uint32_t colour)
{
    for (int y = -radius; y <= radius; ++y) {
        int span = (int)sqrtf((float)(radius * radius - y * y));
        for (int x = -span; x <= span; ++x) pixel(display, cx + x, cy + y, colour);
    }
}

static void outlineCircle(const DisplayInfo *display, int cx, int cy, int radius,
                          uint32_t colour)
{
    int x = radius;
    int y = 0;
    int error = 1 - radius;
    while (x >= y) {
        pixel(display, cx + x, cy + y, colour); pixel(display, cx + y, cy + x, colour);
        pixel(display, cx - y, cy + x, colour); pixel(display, cx - x, cy + y, colour);
        pixel(display, cx - x, cy - y, colour); pixel(display, cx - y, cy - x, colour);
        pixel(display, cx + y, cy - x, colour); pixel(display, cx + x, cy - y, colour);
        ++y;
        if (error < 0) error += 2 * y + 1;
        else { --x; error += 2 * (y - x) + 1; }
    }
}

static void drawButton(const DisplayInfo *display, int x, int y, int radius, bool held)
{
    if (held) {
        fillCircle(display, x, y, radius, COLOUR_ACTIVE);
        outlineCircle(display, x, y, radius - 3, COLOUR_WHITE);
    } else {
        outlineCircle(display, x, y, radius, COLOUR_LINE);
        outlineCircle(display, x, y, radius - 3, COLOUR_PANEL);
    }
}

static void drawRectButton(const DisplayInfo *display, int x, int y, int width, int height,
                           bool held)
{
    if (held) {
        fillRect(display, x, y, width, height, COLOUR_ACTIVE);
        outlineRect(display, x + 2, y + 2, width - 4, height - 4, COLOUR_WHITE);
    } else {
        outlineRect(display, x, y, width, height, COLOUR_LINE);
        outlineRect(display, x + 2, y + 2, width - 4, height - 4, COLOUR_PANEL);
    }
}

static void renderMenu(const DisplayInfo *display, const AppState *app)
{
    OSScreenClearBufferEx(display->id, COLOUR_BG);
    int uiScale = display->id == SCREEN_DRC ? 2 : 3;
    int minorScale = display->id == SCREEN_DRC ? 1 : 2;
    drawSmallText(display, display->width / 12, 46, "WIIU GAMEPAD TESTER",
                  uiScale, COLOUR_WHITE);
    drawSmallText(display, display->width / 12, 88, "DISPLAY / CONTROLS / MOTION",
                  minorScale, COLOUR_ACCENT);
    line(display, display->width / 24, 122, display->width * 23 / 24, 122, COLOUR_ACCENT);

    static const char *names[] = {"SCREEN TEST", "KEY & SENSOR", "EXIT"};
    static const char *descriptions[] = {
        "SOLID COLOUR / DEAD PIXELS",
        "BUTTONS / STICKS / MOTION",
        "RETURN TO WII U MENU",
    };
    int cardX = display->width / 12;
    int cardW = display->width * 10 / 12;
    int cardH = display->height / 10;
    int firstY = display->height * 30 / 100;
    int gap = display->height * 14 / 100;
    for (int i = 0; i < 3; ++i) {
        int y = firstY + i * gap;
        uint32_t colour = app->menuItem == i ? COLOUR_ACTIVE : COLOUR_LINE;
        outlineRect(display, cardX, y, cardW, cardH, colour);
        if (app->menuItem == i) {
            outlineRect(display, cardX + 3, y + 3, cardW - 6, cardH - 6, COLOUR_PANEL);
            fillRect(display, cardX, y, 8, cardH, COLOUR_ACTIVE);
        }
        int textY = y + (cardH - 7 * uiScale) / 2;
        drawSmallText(display, cardX + cardW / 14, textY,
                      names[i], uiScale, COLOUR_WHITE);
        drawSmallText(display, cardX + cardW * 43 / 100, textY,
                      descriptions[i], minorScale, COLOUR_WHITE);
    }

    drawSmallTextCentred(display, display->width / 2,
                         display->height - 38 * minorScale,
                         "WALKING BUNNY X CODEX",
                         minorScale, COLOUR_ACCENT);
    drawSmallTextCentred(display, display->width / 2,
                         display->height - 18 * minorScale,
                         "D-PAD NAVIGATE   A SELECT   HOME EXIT",
                         minorScale, COLOUR_LINE);
}

static void renderScreenTest(const DisplayInfo *display, const AppState *app)
{
    OSScreenClearBufferEx(display->id, kTestColours[app->colourIndex]);
    if (app->colourIndex == 0 && app->screenIntroFrames > 0) {
        uint32_t middleRow = (uint32_t)(display->height / 48);
        putText(display->id, display->id == SCREEN_DRC ? 14 : 27,
                middleRow, "A: NEXT COLOUR    B: EXIT");
    }
}

static void drawStick(const DisplayInfo *display, int cx, int cy, int radius,
                      float x, float y, bool pressed)
{
    outlineCircle(display, cx, cy, radius, pressed ? COLOUR_ACTIVE : COLOUR_LINE);
    line(display, cx - radius, cy, cx + radius, cy, COLOUR_PANEL);
    line(display, cx, cy - radius, cx, cy + radius, COLOUR_PANEL);
    int px = cx + (int)(x * (radius - 5));
    int py = cy - (int)(y * (radius - 5));
    fillCircle(display, px, py, 5, COLOUR_ACCENT);
}

static void drawDpad(const DisplayInfo *display, int cx, int cy, int size, uint32_t hold)
{
    int third = size / 3;
    drawRectButton(display, cx - third / 2, cy - size / 2, third, third,
                   (hold & VPAD_BUTTON_UP) != 0);
    drawRectButton(display, cx - third / 2, cy + size / 2 - third, third, third,
                   (hold & VPAD_BUTTON_DOWN) != 0);
    drawRectButton(display, cx - size / 2, cy - third / 2, third, third,
                   (hold & VPAD_BUTTON_LEFT) != 0);
    drawRectButton(display, cx + size / 2 - third, cy - third / 2, third, third,
                   (hold & VPAD_BUTTON_RIGHT) != 0);
    fillRect(display, cx - third / 2, cy - third / 2, third, third, COLOUR_PANEL);
}

static void drawMotionMeter(const DisplayInfo *display, int x, int y, int width,
                            float value, uint32_t colour)
{
    line(display, x, y, x + width, y, COLOUR_LINE);
    line(display, x + width / 2, y - 4, x + width / 2, y + 4, COLOUR_WHITE);
    float clamped = value < -2.0f ? -2.0f : value > 2.0f ? 2.0f : value;
    int marker = x + width / 2 + (int)(clamped * (float)width / 4.0f);
    fillRect(display, marker - 3, y - 6, 7, 13, colour);
}

static void renderKeyTest(const DisplayInfo *display, const VPADStatus *status,
                          const VPADTouchData *touch, VPADReadError readError)
{
    OSScreenClearBufferEx(display->id, COLOUR_BG);
    putText(display->id, 1, 1, "KEY / STICK / MOTION TEST");
    putText(display->id, 1, 3, "Hold PLUS + MINUS to return");
    line(display, 16, 106, display->width - 16, 106, COLOUR_ACCENT);
    if (readError != VPAD_READ_SUCCESS) {
        putFormatted(display->id, 1, 6, "GamePad read error: %d", (int)readError);
        putText(display->id, 1, 8, "Check the GamePad connection.");
        return;
    }

    outlineRect(display, 12, 116, display->width * 59 / 100, 94, COLOUR_PANEL);
    putFormatted(display->id, 1, 5, "L  %+1.2f %+1.2f   R  %+1.2f %+1.2f",
                 status->leftStick.x, status->leftStick.y,
                 status->rightStick.x, status->rightStick.y);
    putFormatted(display->id, 1, 6, "Gyro X%+1.2f Y%+1.2f Z%+1.2f",
                 status->gyro.x, status->gyro.y, status->gyro.z);
    putFormatted(display->id, 1, 7, "Accel X%+1.2f Y%+1.2f Z%+1.2f",
                 status->accelorometer.acc.x, status->accelorometer.acc.y,
                 status->accelorometer.acc.z);
    putFormatted(display->id, 1, 8, "Touch %s X%u Y%u  Battery %u  Vol %u",
                 touch->touched ? "ON " : "OFF", touch->x, touch->y,
                 status->battery, status->slideVolume);

    int meterX = display->width * 65 / 100;
    int meterW = display->width * 28 / 100;
    drawMotionMeter(display, meterX, 142, meterW, status->gyro.x, COLOUR_DANGER);
    drawMotionMeter(display, meterX, 166, meterW, status->gyro.y, COLOUR_ACTIVE);
    drawMotionMeter(display, meterX, 190, meterW, status->gyro.z, COLOUR_ACCENT);

    int bodyX = display->width / 16;
    int bodyY = display->height * 50 / 100;
    int bodyW = display->width * 14 / 16;
    int bodyH = display->height * 44 / 100;
    outlineRect(display, bodyX, bodyY, bodyW, bodyH, COLOUR_LINE);
    outlineRect(display, bodyX + 3, bodyY + 3, bodyW - 6, bodyH - 6, COLOUR_PANEL);

    int shoulderW = bodyW / 7;
    int shoulderH = bodyH / 10;
    drawRectButton(display, bodyX + bodyW / 12, bodyY - shoulderH, shoulderW, shoulderH,
                   (status->hold & VPAD_BUTTON_ZL) != 0);
    drawRectButton(display, bodyX + bodyW * 3 / 12, bodyY - shoulderH, shoulderW, shoulderH,
                   (status->hold & VPAD_BUTTON_L) != 0);
    drawRectButton(display, bodyX + bodyW * 8 / 12, bodyY - shoulderH, shoulderW, shoulderH,
                   (status->hold & VPAD_BUTTON_R) != 0);
    drawRectButton(display, bodyX + bodyW * 10 / 12, bodyY - shoulderH, shoulderW, shoulderH,
                   (status->hold & VPAD_BUTTON_ZR) != 0);
    putPixelLabel(display, bodyX + bodyW / 12 + shoulderW / 2,
                  bodyY - shoulderH / 2, "ZL");
    putPixelLabel(display, bodyX + bodyW * 3 / 12 + shoulderW / 2,
                  bodyY - shoulderH / 2, "L");
    putPixelLabel(display, bodyX + bodyW * 8 / 12 + shoulderW / 2,
                  bodyY - shoulderH / 2, "R");
    putPixelLabel(display, bodyX + bodyW * 10 / 12 + shoulderW / 2,
                  bodyY - shoulderH / 2, "ZR");

    int stickRadius = bodyH / 10;
    int leftStickX = bodyX + bodyW * 15 / 100;
    int leftStickY = bodyY + bodyH * 24 / 100;
    int rightStickX = bodyX + bodyW * 85 / 100;
    int rightStickY = bodyY + bodyH * 24 / 100;
    drawStick(display, leftStickX, leftStickY, stickRadius,
              status->leftStick.x, status->leftStick.y,
              (status->hold & VPAD_BUTTON_STICK_L) != 0);
    drawStick(display, rightStickX, rightStickY, stickRadius,
              status->rightStick.x, status->rightStick.y,
              (status->hold & VPAD_BUTTON_STICK_R) != 0);
    putPixelLabel(display, leftStickX, leftStickY, "LS");
    putPixelLabel(display, rightStickX, rightStickY, "RS");
    drawDpad(display, bodyX + bodyW * 16 / 100, bodyY + bodyH * 62 / 100,
             bodyH / 3, status->hold);

    int faceX = bodyX + bodyW * 86 / 100;
    int faceY = bodyY + bodyH * 57 / 100;
    int faceGap = bodyH / 9;
    int faceRadius = bodyH / 22;
    drawButton(display, faceX, faceY - faceGap, faceRadius, (status->hold & VPAD_BUTTON_X) != 0);
    drawButton(display, faceX, faceY + faceGap, faceRadius, (status->hold & VPAD_BUTTON_B) != 0);
    drawButton(display, faceX - faceGap, faceY, faceRadius, (status->hold & VPAD_BUTTON_Y) != 0);
    drawButton(display, faceX + faceGap, faceY, faceRadius, (status->hold & VPAD_BUTTON_A) != 0);
    putPixelLabel(display, faceX, faceY - faceGap, "X");
    putPixelLabel(display, faceX, faceY + faceGap, "B");
    putPixelLabel(display, faceX - faceGap, faceY, "Y");
    putPixelLabel(display, faceX + faceGap, faceY, "A");

    int touchX = bodyX + bodyW * 30 / 100;
    int touchY = bodyY + bodyH * 16 / 100;
    int touchW = bodyW * 40 / 100;
    int touchH = bodyH * 60 / 100;
    outlineRect(display, touchX, touchY, touchW, touchH, COLOUR_ACCENT);
    outlineRect(display, touchX + 3, touchY + 3, touchW - 6, touchH - 6, COLOUR_PANEL);
    fillCircle(display, bodyX + bodyW / 2, bodyY + bodyH * 9 / 100,
               bodyH / 80 + 1, COLOUR_ACCENT);
    if (touch->touched && touch->validity == VPAD_VALID) {
        int px = touchX + (int)((uint32_t)touch->x * (uint32_t)(touchW - 1) / 853u);
        int py = touchY + (int)((uint32_t)touch->y * (uint32_t)(touchH - 1) / 479u);
        fillCircle(display, px, py, 5, COLOUR_TOUCH);
    }

    int smallX = bodyX + bodyW * 74 / 100;
    int smallW = bodyW / 22;
    int smallH = bodyH / 10;
    int plusY = bodyY + bodyH * 49 / 100;
    int minusY = bodyY + bodyH * 66 / 100;
    drawRectButton(display, smallX, plusY, smallW, smallH,
                   (status->hold & VPAD_BUTTON_PLUS) != 0);
    drawRectButton(display, smallX, minusY, smallW, smallH,
                   (status->hold & VPAD_BUTTON_MINUS) != 0);
    putPixelLabel(display, smallX + smallW / 2, plusY + smallH / 2, "+");
    putPixelLabel(display, smallX + smallW / 2, minusY + smallH / 2, "-");

    int systemY = bodyY + bodyH * 88 / 100;
    int systemW = bodyW / 18;
    int systemH = bodyH / 13;
    int homeX = bodyX + bodyW * 47 / 100;
    int tvX = bodyX + bodyW * 64 / 100;
    int syncX = bodyX + bodyW * 72 / 100;
    drawRectButton(display, homeX, systemY, systemW, systemH,
                   (status->hold & VPAD_BUTTON_HOME) != 0);
    drawRectButton(display, tvX, systemY, systemW, systemH,
                   (status->hold & VPAD_BUTTON_TV) != 0);
    drawRectButton(display, syncX, systemY, systemW, systemH,
                   (status->hold & VPAD_BUTTON_SYNC) != 0);
    putPixelLabel(display, homeX + systemW / 2, systemY + systemH / 2, "H");
    putPixelLabel(display, tvX + systemW / 2, systemY + systemH / 2, "TV");
    putPixelLabel(display, syncX + systemW / 2, systemY + systemH / 2, "S");
}

static void drawGamePadPhoto(const DisplayInfo *display)
{
    OSScreenClearBufferEx(display->id, COLOUR_BG);
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 854; ++x) {
            size_t offset = ((size_t)y * 854u + (size_t)x) * 3u;
            uint32_t colour = RGB(gamepad_ui_bin[offset],
                                  gamepad_ui_bin[offset + 1],
                                  gamepad_ui_bin[offset + 2]);
            OSScreenPutPixelEx(display->id, (uint32_t)x, (uint32_t)y, colour);
        }
    }
}

static void highlightCircle(const DisplayInfo *display, int x, int y, int radius)
{
    outlineCircle(display, x, y, radius, COLOUR_ACTIVE);
    outlineCircle(display, x, y, radius + 2, COLOUR_ACTIVE);
    outlineCircle(display, x, y, radius + 4, COLOUR_WHITE);
}

static uint32_t photoPixelColour(int x, int y)
{
    size_t offset = ((size_t)y * 854u + (size_t)x) * 3u;
    return RGB(gamepad_ui_bin[offset], gamepad_ui_bin[offset + 1],
               gamepad_ui_bin[offset + 2]);
}

static uint32_t redBlend(uint32_t source)
{
    uint32_t red = (source >> 24) & 0xFFu;
    uint32_t green = (source >> 16) & 0xFFu;
    uint32_t blue = (source >> 8) & 0xFFu;
    /* 58% red overlay, computed in software because OSScreen has no alpha. */
    red = (red * 42u + 255u * 58u) / 100u;
    green = (green * 42u + 32u * 58u) / 100u;
    blue = (blue * 42u + 42u * 58u) / 100u;
    return RGB(red, green, blue);
}

static void photoPressedCircle(const DisplayInfo *display, int cx, int cy, int radius)
{
    for (int y = -radius; y <= radius; ++y) {
        int span = (int)sqrtf((float)(radius * radius - y * y));
        for (int x = -span; x <= span; ++x) {
            int px = cx + x;
            int py = cy + y;
            if (inside(display, px, py))
                pixel(display, px, py, redBlend(photoPixelColour(px, py)));
        }
    }
    outlineCircle(display, cx, cy, radius, COLOUR_PRESSED);
    outlineCircle(display, cx, cy, radius - 2, COLOUR_PRESSED);
}

static void photoPressedRect(const DisplayInfo *display, int x, int y, int width, int height)
{
    for (int py = y; py < y + height; ++py) {
        for (int px = x; px < x + width; ++px) {
            if (inside(display, px, py))
                pixel(display, px, py, redBlend(photoPixelColour(px, py)));
        }
    }
    outlineRect(display, x, y, width, height, COLOUR_PRESSED);
    outlineRect(display, x + 2, y + 2, width - 4, height - 4, COLOUR_PRESSED);
}

static void photoButtonCircle(const DisplayInfo *display, uint32_t hold, uint32_t mask,
                              int x, int y, int radius)
{
    if (hold & mask) photoPressedCircle(display, x, y, radius);
}

static void renderPhotoKeyTest(const DisplayInfo *display, const VPADStatus *status,
                               const VPADTouchData *touch, VPADReadError readError)
{
    drawGamePadPhoto(display);
    if (readError != VPAD_READ_SUCCESS) {
        putFormatted(display->id, 17, 9, "GamePad read error: %d", (int)readError);
        return;
    }

    uint32_t hold = status->hold;
    photoButtonCircle(display, hold, VPAD_BUTTON_X, 676, 198, 14);
    photoButtonCircle(display, hold, VPAD_BUTTON_Y, 648, 225, 14);
    photoButtonCircle(display, hold, VPAD_BUTTON_A, 705, 225, 14);
    photoButtonCircle(display, hold, VPAD_BUTTON_B, 676, 252, 14);
    photoButtonCircle(display, hold, VPAD_BUTTON_PLUS, 647, 294, 9);
    photoButtonCircle(display, hold, VPAD_BUTTON_MINUS, 647, 327, 9);
    photoButtonCircle(display, hold, VPAD_BUTTON_HOME, 427, 383, 14);
    photoButtonCircle(display, hold, VPAD_BUTTON_TV, 568, 383, 10);

    if (hold & VPAD_BUTTON_UP) photoPressedRect(display, 170, 193, 20, 31);
    if (hold & VPAD_BUTTON_DOWN) photoPressedRect(display, 170, 225, 20, 30);
    if (hold & VPAD_BUTTON_LEFT) photoPressedRect(display, 149, 214, 31, 20);
    if (hold & VPAD_BUTTON_RIGHT) photoPressedRect(display, 180, 214, 31, 20);

    if (hold & VPAD_BUTTON_STICK_L) photoPressedCircle(display, 153, 147, 24);
    if (hold & VPAD_BUTTON_STICK_R) photoPressedCircle(display, 703, 147, 24);

    int leftX = 153 + (int)(status->leftStick.x * 22.0f);
    int leftY = 147 - (int)(status->leftStick.y * 22.0f);
    int rightX = 703 + (int)(status->rightStick.x * 22.0f);
    int rightY = 147 - (int)(status->rightStick.y * 22.0f);
    fillCircle(display, leftX, leftY, 5, COLOUR_ACCENT);
    fillCircle(display, rightX, rightY, 5, COLOUR_ACCENT);

    outlineRect(display, 20, 18, 52, 27, COLOUR_LINE);
    outlineRect(display, 82, 18, 52, 27, COLOUR_LINE);
    outlineRect(display, 720, 18, 52, 27, COLOUR_LINE);
    outlineRect(display, 782, 18, 52, 27, COLOUR_LINE);
    if (hold & VPAD_BUTTON_ZL) photoPressedRect(display, 20, 18, 52, 27);
    if (hold & VPAD_BUTTON_L) photoPressedRect(display, 82, 18, 52, 27);
    if (hold & VPAD_BUTTON_R) photoPressedRect(display, 720, 18, 52, 27);
    if (hold & VPAD_BUTTON_ZR) photoPressedRect(display, 782, 18, 52, 27);
    drawSmallTextCentred(display, 46, 28, "ZL", 1, COLOUR_WHITE);
    drawSmallTextCentred(display, 108, 28, "L", 1, COLOUR_WHITE);
    drawSmallTextCentred(display, 746, 28, "R", 1, COLOUR_WHITE);
    drawSmallTextCentred(display, 808, 28, "ZR", 1, COLOUR_WHITE);

    drawSmallTextCentred(display, 427, 158, "KEY / SENSOR TEST", 2, COLOUR_WHITE);
    drawSmallFormatted(display, 260, 199, 1, COLOUR_WHITE,
                       "LS %+1.2f,%+1.2f   RS %+1.2f,%+1.2f",
                       status->leftStick.x, status->leftStick.y,
                       status->rightStick.x, status->rightStick.y);
    drawSmallFormatted(display, 260, 218, 1, COLOUR_WHITE,
                       "GYRO  X%+1.2f  Y%+1.2f  Z%+1.2f",
                       status->gyro.x, status->gyro.y, status->gyro.z);
    drawSmallFormatted(display, 260, 237, 1, COLOUR_WHITE,
                       "ACCEL X%+1.2f  Y%+1.2f  Z%+1.2f",
                       status->accelorometer.acc.x, status->accelorometer.acc.y,
                       status->accelorometer.acc.z);
    drawSmallFormatted(display, 260, 256, 1, COLOUR_WHITE,
                       "TOUCH %-3s X%3u Y%3u   BAT %u VOL %u",
                       touch->touched ? "ON" : "OFF", touch->x, touch->y,
                       status->battery, status->slideVolume);
    drawSmallTextCentred(display, 427, 463, "HOLD + AND - TO RETURN",
                         1, COLOUR_LINE);

    if (touch->touched && touch->validity == VPAD_VALID) {
        int x = 246 + (int)((uint32_t)touch->x * 360u / 853u);
        int y = 141 + (int)((uint32_t)touch->y * 202u / 479u);
        highlightCircle(display, x, y, 6);
    }
}

static void render(const DisplayInfo *display, const AppState *app,
                   const VPADStatus *status, const VPADTouchData *touch,
                   VPADReadError readError)
{
    if (app->page == PAGE_SCREEN_TEST) renderScreenTest(display, app);
    else if (app->page == PAGE_KEY_TEST) {
        if (display->id == SCREEN_DRC) renderPhotoKeyTest(display, status, touch, readError);
        else renderKeyTest(display, status, touch, readError);
    }
    else renderMenu(display, app);
}

static void handleInput(AppState *app, const VPADStatus *status)
{
    uint32_t pressed = status->trigger;
    if (app->page == PAGE_MENU) {
        if (pressed & VPAD_BUTTON_UP) app->menuItem = (app->menuItem + 2) % 3;
        if (pressed & VPAD_BUTTON_DOWN) app->menuItem = (app->menuItem + 1) % 3;
        if (pressed & VPAD_BUTTON_A) {
            if (app->menuItem == 0) {
                app->colourIndex = 0;
                app->screenIntroFrames = 120;
                app->page = PAGE_SCREEN_TEST;
            }
            else if (app->menuItem == 1) app->page = PAGE_KEY_TEST;
            else {
                app->exitRequested = true;
                SYSLaunchMenu();
            }
        }
        return;
    }
    if (app->page == PAGE_SCREEN_TEST) {
        if (pressed & VPAD_BUTTON_B) app->page = PAGE_MENU;
        else if (pressed & VPAD_BUTTON_A)
            app->colourIndex = (app->colourIndex + 1) % TEST_COLOUR_COUNT;
        return;
    }
    if ((status->hold & (VPAD_BUTTON_PLUS | VPAD_BUTTON_MINUS)) ==
        (VPAD_BUTTON_PLUS | VPAD_BUTTON_MINUS)) app->page = PAGE_MENU;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    WHBProcInit();
    OSScreenInit();

    uint32_t tvSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    uint32_t drcSize = OSScreenGetBufferSizeEx(SCREEN_DRC);
    void *tvBuffer = memalign(0x100, tvSize);
    void *drcBuffer = memalign(0x100, drcSize);
    if (!tvBuffer || !drcBuffer) {
        if (tvBuffer) free(tvBuffer);
        if (drcBuffer) free(drcBuffer);
        OSScreenShutdown();
        WHBProcShutdown();
        return 1;
    }

    OSScreenSetBufferEx(SCREEN_TV, tvBuffer);
    OSScreenSetBufferEx(SCREEN_DRC, drcBuffer);
    OSScreenEnableEx(SCREEN_TV, TRUE);
    OSScreenEnableEx(SCREEN_DRC, TRUE);

    const DisplayInfo tv = {SCREEN_TV, 1280, 720};
    const DisplayInfo drc = {SCREEN_DRC, 854, 480};
    AppState app = {PAGE_MENU, 0, 0, 0, false};
    VPADStatus status;
    VPADTouchData touch;
    VPADReadError currentError = VPAD_READ_NO_SAMPLES;
    memset(&status, 0, sizeof(status));
    memset(&touch, 0, sizeof(touch));

    while (WHBProcIsRunning()) {
        /* ProcUI may release the foreground before reporting EXITING. Drawing
           during that interval can lock up the console on return to the menu. */
        if (!ProcUIInForeground()) continue;

        VPADStatus sample;
        VPADReadError readError;
        int32_t count = VPADRead(VPAD_CHAN_0, &sample, 1, &readError);
        if (count > 0 && readError == VPAD_READ_SUCCESS) {
            status = sample;
            currentError = VPAD_READ_SUCCESS;
            VPADGetTPCalibratedPointEx(VPAD_CHAN_0, VPAD_TP_854X480,
                                       &touch, &status.tpNormal);
            handleInput(&app, &status);
            if (app.exitRequested) continue;
        } else if (readError != VPAD_READ_NO_SAMPLES) {
            currentError = readError;
        }

        render(&tv, &app, &status, &touch, currentError);
        render(&drc, &app, &status, &touch, currentError);
        OSScreenFlipBuffersEx(SCREEN_TV);
        OSScreenFlipBuffersEx(SCREEN_DRC);
        if (app.page == PAGE_SCREEN_TEST && app.screenIntroFrames > 0)
            --app.screenIntroFrames;
    }

    free(tvBuffer);
    free(drcBuffer);
    WHBProcShutdown();
    return 0;
}

#include "global.h"
#include "gflib.h"
#include "scanline_effect.h"
#include "text_window_graphics.h"
#include "menu.h"
#include "task.h"
#include "text_window.h"
#include "title_screen.h"
#include "game_language.h"
#include "language_select.h"
#include "korean_ui_strings.h"
#include "langselect_strings.h"
#include "sound.h"
#include "constants/songs.h"

// Boot-time language selection screen. Shown after the intro and before the
// title screen, so the player chooses English / Korean / Japanese / Chinese
// before pressing START to enter gameplay. Modeled on the Options menu so it
// reuses that screen's proven BG/palette/frame/highlight setup. Korean, Japanese
// and Chinese are labeled in their native scripts using the baked font glyphs.

#define ROW_PITCH 16

enum
{
    WIN_TITLE,
    WIN_LIST,
    WIN_FOOTER,
};

struct LanguageSelect
{
    u16 cursorPos;
    u8 state;
    u8 loadPaletteState;
    u8 loadState;
};

static EWRAM_DATA struct LanguageSelect *sLangSelectPtr = NULL;

static void CB2_LanguageSelect(void);
static void VBlankCB_LanguageSelect(void);
static void InitLangSelectBg(void);
static bool8 LoadLangSelectPalette(void);
static void DrawLangSelectFrame(void);
static void PrintLangSelectTitle(void);
static void PrintLangSelectItems(void);
static void PrintLangSelectFooter(void);
static void UpdateLangSelectHighlight(u16 selection);
static void Task_LanguageSelect(u8 taskId);
static void CloseLanguageSelect(u8 taskId);

// The four choices, each in its own script (English Latin, Korean/Japanese/
// Chinese as baked native glyphs). Index == GAME_LANG_* value.
static const u8 sText_English[] = _("ENGLISH");
static const u8 *const sLanguageNames[GAME_LANG_COUNT] =
{
    [GAME_LANG_ENGLISH]  = sText_English,
    [GAME_LANG_KOREAN]   = sKorText_Korean,   // 한국어
    [GAME_LANG_JAPANESE] = sLangText_Japanese, // 日本語
    [GAME_LANG_CHINESE]  = sLangText_Chinese,  // 中文
};

static const u8 sText_SelectLanguage[] = _("SELECT LANGUAGE");
static const u8 sText_Footer[] = _("{DPAD_UPDOWN}CHOOSE  {A_BUTTON}{B_BUTTON}{START_BUTTON}OK");

static const u16 sLangSelectPalette[] = INCBIN_U16("graphics/misc/option_menu.gbapal");
static const u8 sFooterTextColor[] = {TEXT_DYNAMIC_COLOR_6, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY};

static const struct WindowTemplate sLangSelectWinTemplates[] =
{
    [WIN_TITLE] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    [WIN_LIST] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 7,
        .width = 26,
        .height = 12,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    [WIN_FOOTER] = {
        .bg = 2,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x16e
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sLangSelectBgTemplates[] =
{
    {
        .bg = 1, .charBaseIndex = 1, .mapBaseIndex = 30,
        .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0
    },
    {
        .bg = 0, .charBaseIndex = 1, .mapBaseIndex = 31,
        .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0
    },
    {
        .bg = 2, .charBaseIndex = 1, .mapBaseIndex = 29,
        .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0
    },
};

void CB2_InitLanguageSelect(void)
{
    sLangSelectPtr = AllocZeroed(sizeof(struct LanguageSelect));
    sLangSelectPtr->state = 0;
    sLangSelectPtr->loadPaletteState = 0;
    sLangSelectPtr->loadState = 0;
    sLangSelectPtr->cursorPos = gSaveBlock2Ptr->optionsLanguage;
    if (sLangSelectPtr->cursorPos >= GAME_LANG_COUNT)
        sLangSelectPtr->cursorPos = GAME_LANG_ENGLISH;
    SetMainCallback2(CB2_LanguageSelect);
}

static void MainCB_LanguageSelect(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_LanguageSelect(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_LanguageSelect(void)
{
    switch (sLangSelectPtr->state)
    {
    case 0:
        SetVBlankCallback(NULL);
        SetHBlankCallback(NULL);
        break;
    case 1:
        InitLangSelectBg();
        break;
    case 2:
        ResetSpriteData();
        ResetPaletteFade();
        FreeAllSpritePalettes();
        ResetTasks();
        ScanlineEffect_Stop();
        break;
    case 3:
        if (LoadLangSelectPalette() != TRUE)
            return;
        break;
    case 4:
        PrintLangSelectTitle();
        break;
    case 5:
        DrawLangSelectFrame();
        break;
    case 6:
        PrintLangSelectItems();
        break;
    case 7:
        PrintLangSelectFooter();
        break;
    case 8:
        UpdateLangSelectHighlight(sLangSelectPtr->cursorPos);
        break;
    default:
        CreateTask(Task_LanguageSelect, 0);
        SetMainCallback2(MainCB_LanguageSelect);
        break;
    }
    sLangSelectPtr->state++;
}

static void InitLangSelectBg(void)
{
    void *dest = (void *)VRAM;

    DmaClearLarge16(3, dest, VRAM_SIZE, 0x1000);
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    DmaClear16(3, (void *)PLTT, PLTT_SIZE);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sLangSelectBgTemplates, NELEMS(sLangSelectBgTemplates));
    ChangeBgX(0, 0, 0); ChangeBgY(0, 0, 0);
    ChangeBgX(1, 0, 0); ChangeBgY(1, 0, 0);
    ChangeBgX(2, 0, 0); ChangeBgY(2, 0, 0);
    ChangeBgX(3, 0, 0); ChangeBgY(3, 0, 0);
    InitWindows(sLangSelectWinTemplates);
    DeactivateAllTextPrinters();
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_EFFECT_LIGHTEN);
    SetGpuReg(REG_OFFSET_BLDY, BLDCNT_TGT1_BG1);
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_CLR);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
}

static bool8 LoadLangSelectPalette(void)
{
    u8 frame = gSaveBlock2Ptr->optionsWindowFrameType;

    switch (sLangSelectPtr->loadPaletteState)
    {
    case 0:
        LoadBgTiles(1, GetUserWindowGraphics(frame)->tiles, 0x120, 0x1AA);
        break;
    case 1:
        LoadPalette(GetUserWindowGraphics(frame)->palette, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
        break;
    case 2:
        LoadPalette(sLangSelectPalette, BG_PLTT_ID(1), sizeof(sLangSelectPalette));
        LoadPalette(GetTextWindowPalette(2), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        break;
    case 3:
        LoadStdWindowGfxOnBg(1, 0x1B3, BG_PLTT_ID(3));
        break;
    default:
        return TRUE;
    }
    sLangSelectPtr->loadPaletteState++;
    return FALSE;
}

static void PrintLangSelectTitle(void)
{
    FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_TITLE, FONT_NORMAL, sText_SelectLanguage, 8, 1, TEXT_SKIP_DRAW, NULL);
    PutWindowTilemap(WIN_TITLE);
    CopyWindowToVram(WIN_TITLE, COPYWIN_FULL);
}

static void DrawLangSelectFrame(void)
{
    u8 h = 2;

    FillBgTilemapBufferRect(1, 0x1B3, 1, 2, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1B4, 2, 2, 0x1B, 1, 3);
    FillBgTilemapBufferRect(1, 0x1B5, 0x1C, 2, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1B6, 1, 3, 1, h, 3);
    FillBgTilemapBufferRect(1, 0x1B8, 0x1C, 3, 1, h, 3);
    FillBgTilemapBufferRect(1, 0x1B9, 1, 5, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1BA, 2, 5, 0x1B, 1, 3);
    FillBgTilemapBufferRect(1, 0x1BB, 0x1C, 5, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1AA, 1, 6, 1, 1, h);
    FillBgTilemapBufferRect(1, 0x1AB, 2, 6, 0x1A, 1, h);
    FillBgTilemapBufferRect(1, 0x1AC, 0x1C, 6, 1, 1, h);
    FillBgTilemapBufferRect(1, 0x1AD, 1, 7, 1, 0x10, h);
    FillBgTilemapBufferRect(1, 0x1AF, 0x1C, 7, 1, 0x10, h);
    FillBgTilemapBufferRect(1, 0x1B0, 1, 0x13, 1, 1, h);
    FillBgTilemapBufferRect(1, 0x1B1, 2, 0x13, 0x1A, 1, h);
    FillBgTilemapBufferRect(1, 0x1B2, 0x1C, 0x13, 1, 1, h);
    CopyBgTilemapBufferToVram(1);
}

static void PrintLangSelectItems(void)
{
    u8 i;

    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(1));
    for (i = 0; i < GAME_LANG_COUNT; i++)
        AddTextPrinterParameterized(WIN_LIST, FONT_NORMAL, sLanguageNames[i], 8, (i * ROW_PITCH) + 2, TEXT_SKIP_DRAW, NULL);
    PutWindowTilemap(WIN_LIST);
    CopyWindowToVram(WIN_LIST, COPYWIN_FULL);
}

static void PrintLangSelectFooter(void)
{
    s32 x = 0xE4 - GetStringWidth(FONT_SMALL, sText_Footer, 0);

    FillWindowPixelBuffer(WIN_FOOTER, PIXEL_FILL(15));
    AddTextPrinterParameterized3(WIN_FOOTER, FONT_SMALL, x, 0, sFooterTextColor, 0, sText_Footer);
    PutWindowTilemap(WIN_FOOTER);
    CopyWindowToVram(WIN_FOOTER, COPYWIN_FULL);
}

static void UpdateLangSelectHighlight(u16 selection)
{
    u16 maxLetterHeight = GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT);
    u16 y = selection * ROW_PITCH + 0x3A;

    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(y, y + maxLetterHeight));
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0x10, 0xE0));
}

static void Task_LanguageSelect(u8 taskId)
{
    switch (sLangSelectPtr->loadState)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_LanguageSelect);
        sLangSelectPtr->loadState++;
        break;
    case 1:
        if (gPaletteFade.active)
            return;
        sLangSelectPtr->loadState++;
        break;
    case 2:
        if (JOY_NEW(DPAD_UP))
        {
            if (sLangSelectPtr->cursorPos == 0)
                sLangSelectPtr->cursorPos = GAME_LANG_COUNT - 1;
            else
                sLangSelectPtr->cursorPos--;
            PlaySE(SE_SELECT);
            UpdateLangSelectHighlight(sLangSelectPtr->cursorPos);
        }
        else if (JOY_NEW(DPAD_DOWN))
        {
            if (sLangSelectPtr->cursorPos == GAME_LANG_COUNT - 1)
                sLangSelectPtr->cursorPos = 0;
            else
                sLangSelectPtr->cursorPos++;
            PlaySE(SE_SELECT);
            UpdateLangSelectHighlight(sLangSelectPtr->cursorPos);
        }
        // Confirm on A, B or START -- the same set the title screen accepts.
        // This screen is the first gate at boot, so accepting only one button
        // would strand a player whose A key is unmapped in their emulator
        // (common with external keyboards) before they can reach any menu.
        else if (JOY_NEW(A_BUTTON | B_BUTTON | START_BUTTON))
        {
            PlaySE(SE_SELECT);
            gSaveBlock2Ptr->optionsLanguage = sLangSelectPtr->cursorPos;
            // The title screen reloads SaveBlock2 from flash after this screen,
            // which would wipe the line above; remember the pick outside the
            // save block so it can be re-applied once that load has happened.
            GameLanguage_SetBootChoice(sLangSelectPtr->cursorPos);
            sLangSelectPtr->loadState++;
        }
        break;
    case 3:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
        sLangSelectPtr->loadState++;
        break;
    case 4:
        if (gPaletteFade.active)
            return;
        sLangSelectPtr->loadState++;
        break;
    case 5:
        CloseLanguageSelect(taskId);
        break;
    }
}

static void CloseLanguageSelect(u8 taskId)
{
    FreeAllWindowBuffers();
    FREE_AND_SET_NULL(sLangSelectPtr);
    DestroyTask(taskId);
    SetMainCallback2(CB2_InitTitleScreen);
}

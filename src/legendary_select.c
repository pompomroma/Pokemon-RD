#include "global.h"
#include "gflib.h"
#include "scanline_effect.h"
#include "text_window_graphics.h"
#include "menu.h"
#include "task.h"
#include "text_window.h"
#include "overworld.h"
#include "sound.h"
#include "data.h"
#include "trainer_pokemon_sprites.h"
#include "script_pokemon_util.h"
#include "legendary_select.h"
#include "string_util.h"
#include "constants/songs.h"
#include "constants/items.h"
#include "constants/species.h"

// Randomizer-mode starter: PROF. OAK opens his legendary vault and the player
// takes TWO. Every legendary the game has is listed here -- these are all 21
// that exist in this ROM (species stop at 412, so Gen 4+ legendaries have no
// data to show).
//
// Modeled on src/language_select.c, which uses the same proven BG/palette/frame
// and WIN0-highlight scaffolding.

#define ROW_PITCH   14
#define VISIBLE_ROWS 7
#define STARTER_LEVEL 5

static const u16 sLegendaries[] =
{
    SPECIES_ARTICUNO, SPECIES_ZAPDOS,   SPECIES_MOLTRES,  SPECIES_MEWTWO,
    SPECIES_MEW,      SPECIES_RAIKOU,   SPECIES_ENTEI,    SPECIES_SUICUNE,
    SPECIES_LUGIA,    SPECIES_HO_OH,    SPECIES_CELEBI,   SPECIES_REGIROCK,
    SPECIES_REGICE,   SPECIES_REGISTEEL, SPECIES_LATIAS,  SPECIES_LATIOS,
    SPECIES_KYOGRE,   SPECIES_GROUDON,  SPECIES_RAYQUAZA, SPECIES_JIRACHI,
    SPECIES_DEOXYS,
};
#define LEGENDARY_COUNT ARRAY_COUNT(sLegendaries)

enum { WIN_TITLE, WIN_LIST, WIN_FOOTER };

struct LegendarySelect
{
    u16 cursor;      // index into sLegendaries
    u16 scroll;      // first visible row
    u16 picked[2];
    u8 numPicked;
    u8 state;
    u8 loadPaletteState;
    u8 loadState;
    u16 spriteId;
};

static EWRAM_DATA struct LegendarySelect *sPtr = NULL;

static void CB2_LegendarySelect(void);
static void VBlankCB_LegendarySelect(void);
static void InitBgs(void);
static bool8 LoadPal(void);
static void DrawFrame(void);
static void PrintTitle(void);
static void PrintList(void);
static void PrintFooter(void);
static void UpdateHighlight(void);
static void ShowCurrentSprite(void);
static void Task_LegendarySelect(u8 taskId);
static void Finish(u8 taskId);

static const u8 sText_Title[] = _("CHOOSE TWO LEGENDS");
static const u8 sText_Footer[] = _("{DPAD_UPDOWN}PICK  {A_BUTTON}TAKE");
static const u8 sText_First[] = _(" 1");
static const u8 sText_Second[] = _(" 2");

static const u16 sPalette[] = INCBIN_U16("graphics/misc/option_menu.gbapal");
static const u8 sFooterColor[] = {TEXT_DYNAMIC_COLOR_6, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY};

static const struct WindowTemplate sWinTemplates[] =
{
    [WIN_TITLE]  = { .bg = 1, .tilemapLeft = 2,  .tilemapTop = 1, .width = 26, .height = 2,  .paletteNum = 1, .baseBlock = 2 },
    [WIN_LIST]   = { .bg = 0, .tilemapLeft = 2,  .tilemapTop = 5, .width = 14, .height = 14, .paletteNum = 1, .baseBlock = 0x36 },
    [WIN_FOOTER] = { .bg = 2, .tilemapLeft = 0,  .tilemapTop = 0, .width = 30, .height = 2,  .paletteNum = 15, .baseBlock = 0x16e },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sBgTemplates[] =
{
    { .bg = 1, .charBaseIndex = 1, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0 },
    { .bg = 0, .charBaseIndex = 1, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0 },
    { .bg = 2, .charBaseIndex = 1, .mapBaseIndex = 29, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0 },
};

void CB2_InitLegendarySelect(void)
{
    sPtr = AllocZeroed(sizeof(struct LegendarySelect));
    sPtr->spriteId = SPRITE_NONE;
    SetMainCallback2(CB2_LegendarySelect);
}

static void MainCB(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_LegendarySelect(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void CB2_LegendarySelect(void)
{
    switch (sPtr->state)
    {
    case 0: SetVBlankCallback(NULL); SetHBlankCallback(NULL); break;
    case 1: InitBgs(); break;
    case 2:
        ResetSpriteData();
        ResetPaletteFade();
        FreeAllSpritePalettes();
        ResetTasks();
        ScanlineEffect_Stop();
        break;
    case 3: if (LoadPal() != TRUE) return; break;
    case 4: PrintTitle(); break;
    case 5: DrawFrame(); break;
    case 6: PrintList(); break;
    case 7: PrintFooter(); break;
    case 8: UpdateHighlight(); ShowCurrentSprite(); break;
    default:
        CreateTask(Task_LegendarySelect, 0);
        SetMainCallback2(MainCB);
        break;
    }
    sPtr->state++;
}

static void InitBgs(void)
{
    void *dest = (void *)VRAM;

    DmaClearLarge16(3, dest, VRAM_SIZE, 0x1000);
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    DmaClear16(3, (void *)PLTT, PLTT_SIZE);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sBgTemplates, NELEMS(sBgTemplates));
    ChangeBgX(0, 0, 0); ChangeBgY(0, 0, 0);
    ChangeBgX(1, 0, 0); ChangeBgY(1, 0, 0);
    ChangeBgX(2, 0, 0); ChangeBgY(2, 0, 0);
    InitWindows(sWinTemplates);
    DeactivateAllTextPrinters();
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_EFFECT_LIGHTEN);
    SetGpuReg(REG_OFFSET_BLDY, BLDCNT_TGT1_BG1);
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_BG2 | WINOUT_WIN01_CLR);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON);
    ShowBg(0); ShowBg(1); ShowBg(2);
}

static bool8 LoadPal(void)
{
    u8 frame = gSaveBlock2Ptr->optionsWindowFrameType;

    switch (sPtr->loadPaletteState)
    {
    case 0: LoadBgTiles(1, GetUserWindowGraphics(frame)->tiles, 0x120, 0x1AA); break;
    case 1: LoadPalette(GetUserWindowGraphics(frame)->palette, BG_PLTT_ID(2), PLTT_SIZE_4BPP); break;
    case 2:
        LoadPalette(sPalette, BG_PLTT_ID(1), sizeof(sPalette));
        LoadPalette(GetTextWindowPalette(2), BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        break;
    case 3: LoadStdWindowGfxOnBg(1, 0x1B3, BG_PLTT_ID(3)); break;
    default: return TRUE;
    }
    sPtr->loadPaletteState++;
    return FALSE;
}

static void PrintTitle(void)
{
    FillWindowPixelBuffer(WIN_TITLE, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_TITLE, FONT_NORMAL, sText_Title, 8, 1, TEXT_SKIP_DRAW, NULL);
    PutWindowTilemap(WIN_TITLE);
    CopyWindowToVram(WIN_TITLE, COPYWIN_FULL);
}

static void DrawFrame(void)
{
    u8 h = 2;

    FillBgTilemapBufferRect(1, 0x1B3, 1, 0, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1B4, 2, 0, 0x1B, 1, 3);
    FillBgTilemapBufferRect(1, 0x1B5, 0x1C, 0, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1B6, 1, 1, 1, h, 3);
    FillBgTilemapBufferRect(1, 0x1B8, 0x1C, 1, 1, h, 3);
    FillBgTilemapBufferRect(1, 0x1B9, 1, 3, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1BA, 2, 3, 0x1B, 1, 3);
    FillBgTilemapBufferRect(1, 0x1BB, 0x1C, 3, 1, 1, 3);
    FillBgTilemapBufferRect(1, 0x1AA, 1, 4, 1, 1, h);
    FillBgTilemapBufferRect(1, 0x1AB, 2, 4, 0x1A, 1, h);
    FillBgTilemapBufferRect(1, 0x1AC, 0x1C, 4, 1, 1, h);
    FillBgTilemapBufferRect(1, 0x1AD, 1, 5, 1, 0x0E, h);
    FillBgTilemapBufferRect(1, 0x1AF, 0x1C, 5, 1, 0x0E, h);
    FillBgTilemapBufferRect(1, 0x1B0, 1, 0x13, 1, 1, h);
    FillBgTilemapBufferRect(1, 0x1B1, 2, 0x13, 0x1A, 1, h);
    FillBgTilemapBufferRect(1, 0x1B2, 0x1C, 0x13, 1, 1, h);
    CopyBgTilemapBufferToVram(1);
}

// Marks already-taken picks with " 1" / " 2" so the player can see their choices.
static void PrintList(void)
{
    u8 i;

    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(1));
    for (i = 0; i < VISIBLE_ROWS; i++)
    {
        u16 idx = sPtr->scroll + i;
        u8 buf[24];
        u8 *end;

        if (idx >= LEGENDARY_COUNT)
            break;
        end = StringCopy(buf, gSpeciesNames[sLegendaries[idx]]);
        if (sPtr->numPicked > 0 && sPtr->picked[0] == idx)
            StringCopy(end, sText_First);
        else if (sPtr->numPicked > 1 && sPtr->picked[1] == idx)
            StringCopy(end, sText_Second);
        AddTextPrinterParameterized(WIN_LIST, FONT_NORMAL, buf, 8, i * ROW_PITCH + 2, TEXT_SKIP_DRAW, NULL);
    }
    PutWindowTilemap(WIN_LIST);
    CopyWindowToVram(WIN_LIST, COPYWIN_FULL);
}

static void PrintFooter(void)
{
    s32 x = 0xE4 - GetStringWidth(FONT_SMALL, sText_Footer, 0);

    FillWindowPixelBuffer(WIN_FOOTER, PIXEL_FILL(15));
    AddTextPrinterParameterized3(WIN_FOOTER, FONT_SMALL, x, 0, sFooterColor, 0, sText_Footer);
    PutWindowTilemap(WIN_FOOTER);
    CopyWindowToVram(WIN_FOOTER, COPYWIN_FULL);
}

static void UpdateHighlight(void)
{
    u16 row = sPtr->cursor - sPtr->scroll;
    u16 y = row * ROW_PITCH + 0x2A;

    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(y, y + GetFontAttribute(FONT_NORMAL, FONTATTR_MAX_LETTER_HEIGHT)));
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0x10, 0x78));
}

// Shows the highlighted legendary so the player picks by sight, not just name.
static void ShowCurrentSprite(void)
{
    u16 species = sLegendaries[sPtr->cursor];

    if (sPtr->spriteId != SPRITE_NONE)
    {
        FreeAndDestroyMonPicSprite(sPtr->spriteId);
        sPtr->spriteId = SPRITE_NONE;
    }
    sPtr->spriteId = CreateMonPicSprite_HandleDeoxys(species, 0, 0x8000, TRUE, 180, 72, 0,
                                                     gMonPaletteTable[species].tag);
}

static void Task_LegendarySelect(u8 taskId)
{
    switch (sPtr->loadState)
    {
    case 0:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_LegendarySelect);
        sPtr->loadState++;
        break;
    case 1:
        if (gPaletteFade.active)
            return;
        sPtr->loadState++;
        break;
    case 2:
        if (JOY_NEW(DPAD_UP) && sPtr->cursor > 0)
        {
            sPtr->cursor--;
            if (sPtr->cursor < sPtr->scroll)
                sPtr->scroll = sPtr->cursor;
            PlaySE(SE_SELECT);
            PrintList(); UpdateHighlight(); ShowCurrentSprite();
        }
        else if (JOY_NEW(DPAD_DOWN) && sPtr->cursor < LEGENDARY_COUNT - 1)
        {
            sPtr->cursor++;
            if (sPtr->cursor >= sPtr->scroll + VISIBLE_ROWS)
                sPtr->scroll = sPtr->cursor - VISIBLE_ROWS + 1;
            PlaySE(SE_SELECT);
            PrintList(); UpdateHighlight(); ShowCurrentSprite();
        }
        else if (JOY_NEW(A_BUTTON))
        {
            // Ignore a repeat of the same entry; two distinct legends are taken.
            if (sPtr->numPicked == 1 && sPtr->picked[0] == sPtr->cursor)
                return;
            sPtr->picked[sPtr->numPicked++] = sPtr->cursor;
            PlaySE(SE_SELECT);
            PrintList();
            if (sPtr->numPicked >= 2)
                sPtr->loadState++;
        }
        break;
    case 3:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
        sPtr->loadState++;
        break;
    case 4:
        if (gPaletteFade.active)
            return;
        sPtr->loadState++;
        break;
    case 5:
        Finish(taskId);
        break;
    }
}

static void Finish(u8 taskId)
{
    u16 a = sLegendaries[sPtr->picked[0]];
    u16 b = sLegendaries[sPtr->picked[1]];

    if (sPtr->spriteId != SPRITE_NONE)
        FreeAndDestroyMonPicSprite(sPtr->spriteId);
    ScriptGiveMon(a, STARTER_LEVEL, ITEM_NONE, 0, 0, 0);
    ScriptGiveMon(b, STARTER_LEVEL, ITEM_NONE, 0, 0, 0);
    FreeAllWindowBuffers();
    FREE_AND_SET_NULL(sPtr);
    DestroyTask(taskId);
    SetMainCallback2(CB2_ReturnToFieldContinueScript);
}

// Script entry point: opens the vault. Used only in randomizer mode, from
// PROF. OAK's lab, and the script waits on it with `waitstate`.
void ChooseTwoLegendaries(void)
{
    CB2_InitLegendarySelect();
}

#include "global.h"
#include "gflib.h"
#include "sprite.h"
#include "decompress.h"
#include "lens_flare.h"
#include "trig.h"

// See include/lens_flare.h.

#define FLARE_TAG 0xD7A0

#define FLARE_COUNT 5

// The light source sits off the top-left corner, matching the warm sunlight
// direction the cinematic grade already assumes. Flares land along the line
// running from there through the centre of the screen.
#define LIGHT_X (-24)
#define LIGHT_Y (-16)
#define SCREEN_CX 120
#define SCREEN_CY 80

// Frame in the sheet, and how far along the light axis each element sits.
// t is a Q8 fraction: 256 is the centre of the screen, higher overshoots past
// it, which is what gives a flare its strung-out look.
struct FlareElement
{
    u8 frame;
    s16 t;
    u8 subpriority;
};

static const struct FlareElement sElements[FLARE_COUNT] =
{
    {0, 110, 0},   // large soft disc, still up near the light
    {2, 205, 1},   // small disc
    {3, 300, 1},   // chromatic ring past centre
    {1, 395, 2},   // medium disc
    {2, 470, 2},   // small trailing disc
};

static const u32 sFlareGfx[] = INCBIN_U32("graphics/misc/lens_flare.4bpp.lz");
static const u16 sFlarePal[] = INCBIN_U16("graphics/misc/lens_flare.gbapal");

static const struct CompressedSpriteSheet sFlareSheet =
{
    .data = sFlareGfx,
    .size = 4 * 0x200, // four 32x32 4bpp frames
    .tag = FLARE_TAG,
};

static const struct SpritePalette sFlarePalette =
{
    .data = sFlarePal,
    .tag = FLARE_TAG,
};

static const struct OamData sFlareOam =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
};

static const union AnimCmd sAnim0[] = {ANIMCMD_FRAME(0, 1), ANIMCMD_END};
static const union AnimCmd sAnim1[] = {ANIMCMD_FRAME(16, 1), ANIMCMD_END};
static const union AnimCmd sAnim2[] = {ANIMCMD_FRAME(32, 1), ANIMCMD_END};
static const union AnimCmd sAnim3[] = {ANIMCMD_FRAME(48, 1), ANIMCMD_END};

static const union AnimCmd *const sFlareAnims[] = {sAnim0, sAnim1, sAnim2, sAnim3};

static void SpriteCB_Flare(struct Sprite *sprite);

static const struct SpriteTemplate sFlareTemplate =
{
    .tileTag = FLARE_TAG,
    .paletteTag = FLARE_TAG,
    .oam = &sFlareOam,
    .anims = sFlareAnims,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Flare,
};

// data[0] is the element's position along the light axis, data[1] a phase
// offset so the elements do not all breathe in step.
#define sT     data[0]
#define sPhase data[1]

static void SpriteCB_Flare(struct Sprite *sprite)
{
    s32 t = sprite->sT;

    sprite->sPhase += 1;

    // Slow drift along the axis, so the flare sways as if the light moves.
    t += Sin((sprite->sPhase >> 2) & 0xFF, 10);

    sprite->x = LIGHT_X + (((SCREEN_CX - LIGHT_X) * t) >> 8);
    sprite->y = LIGHT_Y + (((SCREEN_CY - LIGHT_Y) * t) >> 8);

    // A slight breathing bob perpendicular to the axis keeps it alive.
    sprite->y2 = Sin((sprite->sPhase >> 1) & 0xFF, 2);
}

void LensFlare_Create(void)
{
    s32 i;

    if (IndexOfSpritePaletteTag(FLARE_TAG) != 0xFF)
        return; // already up for this scene

    LoadCompressedSpriteSheet(&sFlareSheet);
    LoadSpritePalette(&sFlarePalette);

    for (i = 0; i < FLARE_COUNT; i++)
    {
        u8 spriteId = CreateSprite(&sFlareTemplate, 0, 0, sElements[i].subpriority);

        if (spriteId == MAX_SPRITES)
            break; // scene is out of sprite slots; drop the rest quietly
        StartSpriteAnim(&gSprites[spriteId], sElements[i].frame);
        gSprites[spriteId].sT = sElements[i].t;
        gSprites[spriteId].sPhase = i * 40;
        gSprites[spriteId].callback(&gSprites[spriteId]);
    }
}

void LensFlare_Destroy(void)
{
    s32 i;

    for (i = 0; i < MAX_SPRITES; i++)
    {
        if (gSprites[i].inUse && gSprites[i].template->tileTag == FLARE_TAG)
            DestroySprite(&gSprites[i]);
    }
    FreeSpriteTilesByTag(FLARE_TAG);
    FreeSpritePaletteByTag(FLARE_TAG);
}

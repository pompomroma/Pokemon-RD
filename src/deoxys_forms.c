#include "global.h"
#include "gflib.h"
#include "decompress.h"
#include "graphics.h"
#include "pokemon.h"
#include "deoxys_forms.h"
#include "constants/pokemon.h"
#include "constants/species.h"

// Per-mon Deoxys forme: storage, base stats and sprite selection.
// See include/deoxys_forms.h for why the forme has to be stored per mon.

// The forme table was carved out of SaveBlock2's trailing filler, so the block
// must come out exactly the same size and encryptionKey must not move --
// otherwise every existing save decrypts to garbage.
STATIC_ASSERT(sizeof(struct DeoxysFormRecord) == 12, DeoxysFormRecordSize);
STATIC_ASSERT(sizeof(struct SaveBlock2) == 0xF24, SaveBlock2SizeUnchanged);
STATIC_ASSERT(offsetof(struct SaveBlock2, deoxysForms) == 0xE30, DeoxysFormsOffset);
STATIC_ASSERT(offsetof(struct SaveBlock2, encryptionKey) == 0xF20, EncryptionKeyOffset);

// Canonical Gen 3 base stats, STAT_HP..STAT_SPDEF (the STAT_* order puts SPEED
// before SPATK). Every forme shares baseHP 50, which is why HP needs no hook.
static const u16 sFormBaseStats[DEOXYS_FORM_COUNT][6] =
{
    [DEOXYS_FORM_NORMAL] =
    {
        [STAT_HP] = 50, [STAT_ATK] = 150, [STAT_DEF] = 50,
        [STAT_SPEED] = 150, [STAT_SPATK] = 150, [STAT_SPDEF] = 50,
    },
    [DEOXYS_FORM_ATTACK] =
    {
        [STAT_HP] = 50, [STAT_ATK] = 180, [STAT_DEF] = 20,
        [STAT_SPEED] = 150, [STAT_SPATK] = 180, [STAT_SPDEF] = 20,
    },
    [DEOXYS_FORM_DEFENSE] =
    {
        [STAT_HP] = 50, [STAT_ATK] = 70, [STAT_DEF] = 160,
        [STAT_SPEED] = 90, [STAT_SPATK] = 70, [STAT_SPDEF] = 160,
    },
    [DEOXYS_FORM_SPEED] =
    {
        [STAT_HP] = 50, [STAT_ATK] = 95, [STAT_DEF] = 90,
        [STAT_SPEED] = 180, [STAT_SPATK] = 95, [STAT_SPDEF] = 90,
    },
};

static const u8 sFormName_Normal[]  = _("DEOXYS-N");
static const u8 sFormName_Attack[]  = _("DEOXYS-A");
static const u8 sFormName_Defense[] = _("DEOXYS-D");
static const u8 sFormName_Speed[]   = _("DEOXYS-S");

static const u8 *const sFormNames[DEOXYS_FORM_COUNT] =
{
    [DEOXYS_FORM_NORMAL]  = sFormName_Normal,
    [DEOXYS_FORM_ATTACK]  = sFormName_Attack,
    [DEOXYS_FORM_DEFENSE] = sFormName_Defense,
    [DEOXYS_FORM_SPEED]   = sFormName_Speed,
};

// The ROM has no Speed forme art, so Speed reuses the Normal silhouette with
// its body colours swapped into the greens the same palette already carries:
// the reds/oranges (2,3,4) trade places with the greens (7,6,5). Staying inside
// the mon's own 16 colours means no extra palette and no clash when it is
// blended (fusion) or graded (battle).
static const u8 sSpeedColorSwap[16] =
{
    0, 1, 7, 6, 5, 4, 3, 2, 8, 9, 10, 11, 12, 13, 14, 15,
};

static u8 sPreviewForm = DEOXYS_FORM_NONE;

static struct DeoxysFormRecord *FindByPersonality(u32 personality)
{
    s32 i;

    for (i = 0; i < DEOXYS_FORM_RECORDS_COUNT; i++)
    {
        struct DeoxysFormRecord *rec = &gSaveBlock2Ptr->deoxysForms[i];
        if ((rec->flags & DEOXYS_FORM_FLAG_ACTIVE) && rec->personality == personality)
            return rec;
    }
    return NULL;
}

static struct DeoxysFormRecord *FindFree(void)
{
    s32 i;

    for (i = 0; i < DEOXYS_FORM_RECORDS_COUNT; i++)
    {
        if (!(gSaveBlock2Ptr->deoxysForms[i].flags & DEOXYS_FORM_FLAG_ACTIVE))
            return &gSaveBlock2Ptr->deoxysForms[i];
    }
    return NULL;
}

// Pic decompression only has the personality to go on, so it matches on that
// alone -- the same trade-off Fusion_FindRecordByPersonality already makes.
u8 Deoxys_GetFormByPersonality(u32 personality)
{
    struct DeoxysFormRecord *rec = FindByPersonality(personality);

    if (rec == NULL || rec->form >= DEOXYS_FORM_COUNT)
        return DEOXYS_FORM_ATTACK; // vanilla FireRed forme
    return rec->form;
}

u8 Deoxys_GetBoxMonForm(struct BoxPokemon *boxMon)
{
    struct DeoxysFormRecord *rec;

    if (GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL) != SPECIES_DEOXYS)
        return DEOXYS_FORM_NONE;
    rec = FindByPersonality(GetBoxMonData(boxMon, MON_DATA_PERSONALITY, NULL));
    if (rec == NULL || rec->otId != GetBoxMonData(boxMon, MON_DATA_OT_ID, NULL)
     || rec->form >= DEOXYS_FORM_COUNT)
        return DEOXYS_FORM_ATTACK;
    return rec->form;
}

u8 Deoxys_GetMonForm(struct Pokemon *mon)
{
    return Deoxys_GetBoxMonForm(&mon->box);
}

bool8 Deoxys_SetMonForm(struct Pokemon *mon, u8 form)
{
    u32 personality, otId;
    struct DeoxysFormRecord *rec;

    if (form >= DEOXYS_FORM_COUNT
     || GetMonData(mon, MON_DATA_SPECIES, NULL) != SPECIES_DEOXYS)
        return FALSE;

    personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
    otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
    rec = FindByPersonality(personality);
    if (rec == NULL)
        rec = FindFree();
    if (rec == NULL)
        return FALSE; // no slots left

    rec->personality = personality;
    rec->otId = otId;
    rec->form = form;
    rec->flags = DEOXYS_FORM_FLAG_ACTIVE;
    CalculateMonStats(mon);
    return TRUE;
}

u16 Deoxys_GetFormBaseStat(u8 form, u8 statIndex)
{
    if (form >= DEOXYS_FORM_COUNT || statIndex >= 6)
        return 0;
    return sFormBaseStats[form][statIndex];
}

const u8 *Deoxys_GetFormName(u8 form)
{
    if (form >= DEOXYS_FORM_COUNT)
        return sFormNames[DEOXYS_FORM_ATTACK];
    return sFormNames[form];
}

void Deoxys_SetPreviewForm(u8 form)
{
    sPreviewForm = form;
}

// Byte offset of a forme's icon inside gMonIcon_Deoxys, which holds the Normal,
// Attack and Defense icons back to back. There is no Speed icon, so Speed shows
// the Normal one -- the icon palette is a shared 3-palette set, so the sprite's
// colour swap does not carry over to it.
u32 Deoxys_GetIconOffset(u8 form)
{
    switch (form)
    {
    case DEOXYS_FORM_ATTACK:
    default:
        return 0x400;
    case DEOXYS_FORM_DEFENSE:
        return 0x800;
    case DEOXYS_FORM_NORMAL:
    case DEOXYS_FORM_SPEED:
        return 0x000;
    }
}

// Swaps the body colours of the frame already sitting at dest.
static void RecolorForSpeedForm(u8 *dest)
{
    s32 i;

    for (i = 0; i < 0x800; i++)
        dest[i] = (sSpeedColorSwap[dest[i] >> 4] << 4) | sSpeedColorSwap[dest[i] & 0xF];
}

// Called on a freshly decompressed Deoxys pic, which is always two 0x800 frames:
// frame 0 is the Normal forme and frame 1 the version's own forme. Only frame 0
// is uploaded to VRAM (every sheet is declared 0x800), so selecting a forme
// means getting the right art into frame 0.
void Deoxys_ApplyFormToPic(void *dest, s32 species, u32 personality, bool8 isFrontPic)
{
    u8 form;

    if (species != SPECIES_DEOXYS)
        return;

    form = (sPreviewForm != DEOXYS_FORM_NONE) ? sPreviewForm
                                              : Deoxys_GetFormByPersonality(personality);
    switch (form)
    {
    case DEOXYS_FORM_NORMAL:
        break; // frame 0 is already the Normal forme
    case DEOXYS_FORM_ATTACK:
    default:
        CpuCopy32(dest + 0x800, dest, 0x800);
        break;
    case DEOXYS_FORM_DEFENSE:
        // The Defense art is a separate two-frame pic; decompress it over the
        // buffer (same 0x1000 footprint) and take its frame 1.
        LZ77UnCompWram(isFrontPic ? gMonFrontPic_DeoxysDefense : gMonBackPic_DeoxysDefense, dest);
        CpuCopy32(dest + 0x800, dest, 0x800);
        break;
    case DEOXYS_FORM_SPEED:
        RecolorForSpeedForm(dest);
        break;
    }
}

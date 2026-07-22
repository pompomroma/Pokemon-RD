#include "global.h"
#include "pokemon.h"
#include "stat_custom.h"
#include "pokemon_fusion.h"
#include "constants/pokemon.h"

// Per-mon custom stat records live in gSaveBlock2Ptr->statCustom, keyed by
// personality + otId. A record is "active" once flagged; allocStats all-zero
// means "no allocation" (talent may still be set).

static struct StatCustomRecord *FindByKey(u32 personality, u32 otId)
{
    s32 i;

    for (i = 0; i < STAT_CUSTOM_RECORDS_COUNT; i++)
    {
        struct StatCustomRecord *rec = &gSaveBlock2Ptr->statCustom[i];
        if ((rec->flags & STAT_CUSTOM_FLAG_ACTIVE)
         && rec->personality == personality && rec->otId == otId)
            return rec;
    }
    return NULL;
}

static struct StatCustomRecord *FindFree(void)
{
    s32 i;

    for (i = 0; i < STAT_CUSTOM_RECORDS_COUNT; i++)
    {
        if (!(gSaveBlock2Ptr->statCustom[i].flags & STAT_CUSTOM_FLAG_ACTIVE))
            return &gSaveBlock2Ptr->statCustom[i];
    }
    return NULL;
}

struct StatCustomRecord *StatCustom_FindRecord(struct BoxPokemon *boxMon)
{
    return FindByKey(GetBoxMonData(boxMon, MON_DATA_PERSONALITY, NULL),
                     GetBoxMonData(boxMon, MON_DATA_OT_ID, NULL));
}

static bool8 AllocIsSet(struct StatCustomRecord *rec)
{
    s32 i;

    if (rec == NULL)
        return FALSE;
    for (i = 0; i < 6; i++)
    {
        if (rec->allocStats[i] != 0)
            return TRUE;
    }
    return FALSE;
}

bool8 StatCustom_HasAllocation(struct BoxPokemon *boxMon)
{
    return AllocIsSet(StatCustom_FindRecord(boxMon));
}

bool8 StatCustom_MonHasAllocation(struct Pokemon *mon)
{
    return AllocIsSet(StatCustom_FindRecord(&mon->box));
}

u16 StatCustom_GetAllocBase(struct Pokemon *mon, u8 statIndex)
{
    struct StatCustomRecord *rec = StatCustom_FindRecord(&mon->box);

    if (rec == NULL || statIndex >= 6)
        return 0;
    return rec->allocStats[statIndex];
}

u8 StatCustom_GetTalent(struct Pokemon *mon)
{
    struct StatCustomRecord *rec = StatCustom_FindRecord(&mon->box);

    if (rec == NULL)
        return STAT_CUSTOM_TALENT_NONE;
    return rec->talent;
}

// +15% to the upside stat, -15% to the downside stat. No upper clamp (computed
// stats/HP can exceed 255); floored at 1.
s32 StatCustom_ApplyTalent(struct Pokemon *mon, s32 value, u8 statIndex)
{
    u8 talent = StatCustom_GetTalent(mon);

    if (talent == STAT_CUSTOM_TALENT_NONE)
        return value;
    if (STAT_CUSTOM_TALENT_UP(talent) == statIndex)
        value = value * 115 / 100;
    else if (STAT_CUSTOM_TALENT_DOWN(talent) == statIndex)
        value = value * 85 / 100;
    if (value < 1)
        value = 1;
    return value;
}

// The fusion-aware base stat (ignores any custom allocation) — the value the
// editor starts from and sums into the arrangeable pool.
u16 StatCustom_GetEffectiveBase(struct Pokemon *mon, u8 statIndex)
{
    u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    u16 fusionPartner = Fusion_GetPartnerSpecies(&mon->box);
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    u16 base;

    if (fusionPartner != SPECIES_NONE)
        return Fusion_ApplyStageToStat(Fusion_GetBaseStat(species, fusionPartner, statIndex),
                                       Fusion_GetMonEffectiveStage(&mon->box));
    switch (statIndex)
    {
    case STAT_HP:    base = info->baseHP;        break;
    case STAT_ATK:   base = info->baseAttack;    break;
    case STAT_DEF:   base = info->baseDefense;   break;
    case STAT_SPEED: base = info->baseSpeed;     break;
    case STAT_SPATK: base = info->baseSpAttack;  break;
    case STAT_SPDEF:
    default:         base = info->baseSpDefense; break;
    }
    return base;
}

u16 StatCustom_GetBaseTotal(struct Pokemon *mon)
{
    u16 total = 0;
    u8 i;

    for (i = 0; i < 6; i++)
        total += StatCustom_GetEffectiveBase(mon, i);
    return total;
}

bool8 StatCustom_Save(struct Pokemon *mon, const u8 allocStats[6], u8 talent)
{
    u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
    u32 otId = GetMonData(mon, MON_DATA_OT_ID, NULL);
    struct StatCustomRecord *rec = FindByKey(personality, otId);
    bool8 wantAlloc = FALSE;
    s32 i;

    for (i = 0; i < 6; i++)
    {
        if (allocStats[i] != 0)
            wantAlloc = TRUE;
    }

    // Nothing to store -> free the record if one exists.
    if (!wantAlloc && talent == STAT_CUSTOM_TALENT_NONE)
    {
        if (rec != NULL)
            *rec = (struct StatCustomRecord){0};
        CalculateMonStats(mon);
        return TRUE;
    }

    if (rec == NULL)
        rec = FindFree();
    if (rec == NULL)
        return FALSE; // no slots left

    rec->personality = personality;
    rec->otId = otId;
    rec->talent = talent;
    rec->flags = STAT_CUSTOM_FLAG_ACTIVE;
    for (i = 0; i < 6; i++)
        rec->allocStats[i] = wantAlloc ? allocStats[i] : 0;

    CalculateMonStats(mon);
    return TRUE;
}

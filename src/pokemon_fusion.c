#include "global.h"
#include "gflib.h"
#include "battle.h"
#include "battle_anim.h"
#include "data.h"
#include "decompress.h"
#include "pokemon.h"
#include "pokemon_fusion.h"
#include "pokemon_storage_system.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/songs.h"

// Every attribute of a fused Pokémon is generated from its two parent
// species, so all species pair combinations are supported uniformly:
//  - base stats:  max(a, b) + min(a, b) / 4 per stat (capped at 255)
//  - typing:      base mon's primary + partner's primary (partner's
//                 secondary if the primaries match)
//  - nickname:    front half of the base species' name + back half of the
//                 partner species' name
//  - sprite:      partner's top half spliced onto the base mon's bottom
//                 half, with both palettes blended 50/50
//  - signature:   MOVE_FUSION_BURST, whose displayed name, type, and power
//                 are generated from the pair at runtime

static const u8 sSignatureMoveSuffixes[NUMBER_OF_MON_TYPES][7] = {
    [TYPE_NORMAL]   = _(" NOVA"),
    [TYPE_FIGHTING] = _(" FIST"),
    [TYPE_FLYING]   = _(" GALE"),
    [TYPE_POISON]   = _(" OOZE"),
    [TYPE_GROUND]   = _(" QUAKE"),
    [TYPE_ROCK]     = _(" EDGE"),
    [TYPE_BUG]      = _(" SWARM"),
    [TYPE_GHOST]    = _(" CURSE"),
    [TYPE_STEEL]    = _(" FORGE"),
    [TYPE_MYSTERY]  = _(" NOVA"),
    [TYPE_FIRE]     = _(" BLAZE"),
    [TYPE_WATER]    = _(" SURGE"),
    [TYPE_GRASS]    = _(" BLOOM"),
    [TYPE_ELECTRIC] = _(" VOLT"),
    [TYPE_PSYCHIC]  = _(" MIND"),
    [TYPE_ICE]      = _(" FROST"),
    [TYPE_DRAGON]   = _(" ROAR"),
    [TYPE_DARK]     = _(" DREAD"),
};

struct FusionRecord *Fusion_FindRecordByPersonality(u32 personality)
{
    s32 i;

    for (i = 0; i < FUSION_RECORDS_COUNT; i++)
    {
        struct FusionRecord *rec = &gSaveBlock1Ptr->fusionRecords[i];
        if (rec->partnerSpecies != SPECIES_NONE && rec->personality == personality)
            return rec;
    }
    return NULL;
}

struct FusionRecord *Fusion_FindRecordByMon(struct BoxPokemon *boxMon)
{
    struct FusionRecord *rec = Fusion_FindRecordByPersonality(GetBoxMonData(boxMon, MON_DATA_PERSONALITY, NULL));

    if (rec != NULL && rec->otId == GetBoxMonData(boxMon, MON_DATA_OT_ID, NULL))
        return rec;
    return NULL;
}

bool8 Fusion_IsMonFused(struct BoxPokemon *boxMon)
{
    return Fusion_FindRecordByMon(boxMon) != NULL;
}

u16 Fusion_GetPartnerSpecies(struct BoxPokemon *boxMon)
{
    struct FusionRecord *rec = Fusion_FindRecordByMon(boxMon);

    if (rec == NULL)
        return SPECIES_NONE;
    return rec->partnerSpecies;
}

static struct FusionRecord *FindFreeRecord(void)
{
    s32 i;

    for (i = 0; i < FUSION_RECORDS_COUNT; i++)
    {
        if (gSaveBlock1Ptr->fusionRecords[i].partnerSpecies == SPECIES_NONE)
            return &gSaveBlock1Ptr->fusionRecords[i];
    }
    return NULL;
}

bool8 Fusion_HasFreeRecord(void)
{
    return FindFreeRecord() != NULL;
}

// --- Fusion evolution stages ---------------------------------------------
// The effective stage used for stats/power is derived purely from level, so
// it is always correct even before the stage-up is announced. The stored
// stage (flags bits 1-2) tracks what has been announced, and drives the
// per-stage palette intensification and the one-time stage-up notification.

u8 Fusion_GetLevelStage(u8 level)
{
    if (level >= FUSION_STAGE2_LEVEL)
        return 2;
    if (level >= FUSION_STAGE1_LEVEL)
        return 1;
    return 0;
}

u8 Fusion_GetRecordStage(struct FusionRecord *rec)
{
    if (rec == NULL)
        return 0;
    return (rec->flags & FUSION_STAGE_MASK) >> FUSION_STAGE_SHIFT;
}

static void Fusion_SetRecordStage(struct FusionRecord *rec, u8 stage)
{
    if (stage > FUSION_MAX_STAGE)
        stage = FUSION_MAX_STAGE;
    rec->flags = (rec->flags & ~FUSION_STAGE_MASK) | (stage << FUSION_STAGE_SHIFT);
}

u8 Fusion_GetMonEffectiveStage(struct BoxPokemon *boxMon)
{
    if (!Fusion_IsMonFused(boxMon))
        return 0;
    return Fusion_GetLevelStage(GetLevelFromBoxMonExp(boxMon));
}

// stage 0 = x1.00, stage 1 = x1.15, stage 2 = x1.30, clamped to 255.
u16 Fusion_ApplyStageToStat(u16 base, u8 stage)
{
    u32 result = (u32)base * (100 + 15 * stage) / 100;

    if (result > 255)
        result = 255;
    return result;
}

// Bring the stored (announced) stage up to the level-derived stage. Returns
// TRUE if it advanced (so callers can play a stage-up notification).
bool8 Fusion_TryStageUp(struct Pokemon *mon)
{
    struct FusionRecord *rec = Fusion_FindRecordByMon(&mon->box);
    u8 levelStage, storedStage;

    if (rec == NULL)
        return FALSE;
    levelStage = Fusion_GetLevelStage(GetMonData(mon, MON_DATA_LEVEL, NULL));
    storedStage = Fusion_GetRecordStage(rec);
    if (levelStage <= storedStage)
        return FALSE;
    Fusion_SetRecordStage(rec, levelStage);
    CalculateMonStats(mon);
    return TRUE;
}

u16 Fusion_FuseBaseStatPair(u16 statA, u16 statB)
{
    u16 result;

    if (statA < statB)
    {
        result = statB + statA / 4;
    }
    else
    {
        result = statA + statB / 4;
    }
    if (result > 255)
        result = 255;
    return result;
}

u8 Fusion_GetBaseStat(u16 species, u16 partnerSpecies, u8 statIndex)
{
    const struct SpeciesInfo *a = &gSpeciesInfo[species];
    const struct SpeciesInfo *b = &gSpeciesInfo[partnerSpecies];

    switch (statIndex)
    {
    case STAT_HP:
        return Fusion_FuseBaseStatPair(a->baseHP, b->baseHP);
    case STAT_ATK:
        return Fusion_FuseBaseStatPair(a->baseAttack, b->baseAttack);
    case STAT_DEF:
        return Fusion_FuseBaseStatPair(a->baseDefense, b->baseDefense);
    case STAT_SPEED:
        return Fusion_FuseBaseStatPair(a->baseSpeed, b->baseSpeed);
    case STAT_SPATK:
        return Fusion_FuseBaseStatPair(a->baseSpAttack, b->baseSpAttack);
    case STAT_SPDEF:
    default:
        return Fusion_FuseBaseStatPair(a->baseSpDefense, b->baseSpDefense);
    }
}

void Fusion_GetTypes(u16 species, u16 partnerSpecies, u8 *type1, u8 *type2)
{
    *type1 = gSpeciesInfo[species].types[0];
    *type2 = gSpeciesInfo[partnerSpecies].types[0];
    if (*type2 == *type1)
        *type2 = gSpeciesInfo[partnerSpecies].types[1];
}

void Fusion_BuildFusedName(u16 speciesA, u16 speciesB, u8 *dest)
{
    const u8 *nameA = gSpeciesNames[speciesA];
    const u8 *nameB = gSpeciesNames[speciesB];
    u8 lenA = StringLength(nameA);
    u8 lenB = StringLength(nameB);
    u8 front = (lenA + 1) / 2;
    u8 backStart = lenB / 2;
    u8 i, n = 0;

    for (i = 0; i < front && n < POKEMON_NAME_LENGTH; i++)
        dest[n++] = nameA[i];
    for (i = backStart; i < lenB && n < POKEMON_NAME_LENGTH; i++)
        dest[n++] = nameB[i];
    dest[n] = EOS;
}

static u8 GetOffensiveBaseStat(u16 species)
{
    u8 atk = gSpeciesInfo[species].baseAttack;
    u8 spAtk = gSpeciesInfo[species].baseSpAttack;

    return atk > spAtk ? atk : spAtk;
}

u8 Fusion_GetSignatureMoveType(u16 species, u16 partnerSpecies)
{
    // The signature move channels the absorbed partner's essence.
    return gSpeciesInfo[partnerSpecies].types[0];
}

u8 Fusion_GetSignatureMovePower(u16 species, u16 partnerSpecies)
{
    u16 power = 90 + (GetOffensiveBaseStat(species) + GetOffensiveBaseStat(partnerSpecies)) / 8;

    if (power > 140)
        power = 140;
    return power;
}

void Fusion_BuildSignatureMoveName(u16 species, u16 partnerSpecies, u8 *dest)
{
    const u8 *nameA = gSpeciesNames[species];
    const u8 *nameB = gSpeciesNames[partnerSpecies];
    u8 type = Fusion_GetSignatureMoveType(species, partnerSpecies);
    u8 i, n = 0;

    for (i = 0; i < 3 && nameA[i] != EOS; i++)
        dest[n++] = nameA[i];
    for (i = 0; i < 3 && nameB[i] != EOS; i++)
        dest[n++] = nameB[i];
    for (i = 0; sSignatureMoveSuffixes[type][i] != EOS && n < MOVE_NAME_LENGTH; i++)
        dest[n++] = sSignatureMoveSuffixes[type][i];
    dest[n] = EOS;
}

static void GiveSignatureMove(struct Pokemon *mon)
{
    u8 slot;

    for (slot = 0; slot < MAX_MON_MOVES; slot++)
    {
        if (GetMonData(mon, MON_DATA_MOVE1 + slot, NULL) == MOVE_NONE)
            break;
    }
    if (slot >= MAX_MON_MOVES)
        slot = MAX_MON_MOVES - 1;
    SetMonMoveSlot(mon, MOVE_FUSION_BURST, slot);
}

static void RemoveSignatureMove(struct Pokemon *mon)
{
    u16 moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
    u8 i, n = 0;
    u16 none = MOVE_NONE;
    u8 zero = 0;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        u16 move = GetMonData(mon, MON_DATA_MOVE1 + i, NULL);
        if (move != MOVE_NONE && move != MOVE_FUSION_BURST)
        {
            moves[n] = move;
            pp[n] = GetMonData(mon, MON_DATA_PP1 + i, NULL);
            n++;
        }
    }
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        if (i < n)
        {
            SetMonData(mon, MON_DATA_MOVE1 + i, &moves[i]);
            SetMonData(mon, MON_DATA_PP1 + i, &pp[i]);
        }
        else
        {
            SetMonData(mon, MON_DATA_MOVE1 + i, &none);
            SetMonData(mon, MON_DATA_PP1 + i, &zero);
        }
    }
}

bool8 Fusion_FuseParty(u8 slotA, u8 slotB)
{
    struct Pokemon *monA = &gPlayerParty[slotA];
    struct Pokemon *monB = &gPlayerParty[slotB];
    struct FusionRecord *rec = FindFreeRecord();
    u16 speciesA = GetMonData(monA, MON_DATA_SPECIES, NULL);
    u16 speciesB = GetMonData(monB, MON_DATA_SPECIES, NULL);
    u8 name[POKEMON_NAME_LENGTH + 1];

    if (rec == NULL || slotA == slotB)
        return FALSE;
    if (speciesA == SPECIES_NONE || speciesB == SPECIES_NONE)
        return FALSE;
    if (Fusion_IsMonFused(&monA->box) || Fusion_IsMonFused(&monB->box))
        return FALSE;

    rec->personality = GetMonData(monA, MON_DATA_PERSONALITY, NULL);
    rec->otId = GetMonData(monA, MON_DATA_OT_ID, NULL);
    rec->partnerPersonality = GetMonData(monB, MON_DATA_PERSONALITY, NULL);
    rec->partnerSpecies = speciesB;
    rec->partnerLevel = GetMonData(monB, MON_DATA_LEVEL, NULL);
    rec->flags = FUSION_FLAG_ACTIVE;

    Fusion_BuildFusedName(speciesA, speciesB, name);
    SetMonData(monA, MON_DATA_NICKNAME, name);
    GiveSignatureMove(monA);
    CalculateMonStats(monA);

    ZeroMonData(monB);
    CompactPartySlots();
    CalculatePlayerPartyCount();
    return TRUE;
}

bool8 Fusion_UnfuseParty(u8 slot)
{
    struct Pokemon *mon = &gPlayerParty[slot];
    struct FusionRecord *rec = Fusion_FindRecordByMon(&mon->box);
    u16 species;

    if (rec == NULL)
        return FALSE;
    if (CalculatePlayerPartyCount() >= PARTY_SIZE)
        return FALSE;

    species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    SetMonData(mon, MON_DATA_NICKNAME, gSpeciesNames[species]);
    RemoveSignatureMove(mon);

    CreateMon(&gPlayerParty[gPlayerPartyCount], rec->partnerSpecies, rec->partnerLevel,
              USE_RANDOM_IVS, TRUE, rec->partnerPersonality, OT_ID_PLAYER_ID, 0);

    rec->personality = 0;
    rec->otId = 0;
    rec->partnerPersonality = 0;
    rec->partnerSpecies = SPECIES_NONE;
    rec->partnerLevel = 0;
    rec->flags = 0;

    CalculateMonStats(mon);
    CalculatePlayerPartyCount();
    return TRUE;
}

bool8 Fusion_GetMoveNameForMon(struct BoxPokemon *boxMon, u16 move, u8 *dest)
{
    struct FusionRecord *rec;
    u16 species;

    if (move != MOVE_FUSION_BURST || boxMon == NULL)
        return FALSE;
    rec = Fusion_FindRecordByMon(boxMon);
    if (rec == NULL)
        return FALSE;
    species = GetBoxMonData(boxMon, MON_DATA_SPECIES, NULL);
    Fusion_BuildSignatureMoveName(species, rec->partnerSpecies, dest);
    return TRUE;
}

struct Pokemon *Fusion_GetBattlerPartyMon(u8 battler)
{
    if (GetBattlerSide(battler) != B_SIDE_PLAYER)
        return NULL;
    return &gPlayerParty[gBattlerPartyIndexes[battler]];
}

bool8 Fusion_GetMoveNameForBattler(u8 battler, u16 move, u8 *dest)
{
    struct Pokemon *mon = Fusion_GetBattlerPartyMon(battler);

    if (mon == NULL)
        return FALSE;
    return Fusion_GetMoveNameForMon(&mon->box, move, dest);
}

void Fusion_ApplyBattleTypes(u8 battler)
{
    struct Pokemon *mon = Fusion_GetBattlerPartyMon(battler);
    struct FusionRecord *rec;
    u8 type1, type2;

    if (mon == NULL)
        return;
    rec = Fusion_FindRecordByMon(&mon->box);
    if (rec == NULL)
        return;
    if (gBattleMons[battler].species != GetMonData(mon, MON_DATA_SPECIES, NULL))
        return; // transformed into something else; keep its own types
    Fusion_GetTypes(gBattleMons[battler].species, rec->partnerSpecies, &type1, &type2);
    gBattleMons[battler].type1 = type1;
    gBattleMons[battler].type2 = type2;
}

void Fusion_SetSignatureMoveDynamics(u8 battler)
{
    struct Pokemon *mon = Fusion_GetBattlerPartyMon(battler);
    struct FusionRecord *rec;
    u16 species;

    if (mon == NULL)
        return;
    rec = Fusion_FindRecordByMon(&mon->box);
    if (rec == NULL)
        return;
    species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    gBattleStruct->dynamicMoveType = Fusion_GetSignatureMoveType(species, rec->partnerSpecies) | 0x80;
    // The signature move grows with the fusion's evolution stage.
    gDynamicBasePower = Fusion_ApplyStageToStat(Fusion_GetSignatureMovePower(species, rec->partnerSpecies),
                                                Fusion_GetMonEffectiveStage(&mon->box));
}

void Fusion_SpliceMonPic(void *dest, u32 personality, bool8 isFrontPic)
{
    struct FusionRecord *rec = Fusion_FindRecordByPersonality(personality);
    u8 *buffer;
    u16 partner;

    if (rec == NULL)
        return;
    partner = rec->partnerSpecies;
    if (partner == SPECIES_NONE || partner >= NUM_SPECIES)
        return;
    buffer = Alloc(MON_PIC_SIZE);
    if (buffer == NULL)
        return;
    // Decompress the partner pic directly (not via the public loaders, which
    // are hooked to call this function).
    if (isFrontPic)
        LZ77UnCompWram(gMonFrontPicTable[partner].data, buffer);
    else
        LZ77UnCompWram(gMonBackPicTable[partner].data, buffer);
    if (partner == SPECIES_DEOXYS)
        CpuCopy32(buffer + 0x800, buffer, 0x800);
    // Splice the partner's top half (first 4 rows of 8x8 tiles) onto the
    // base Pokémon's bottom half.
    memcpy(dest, buffer, MON_PIC_SIZE / 2);
    Free(buffer);
}

static bool8 GetPartnerBlendPalette(u32 personality, u32 otId, u16 *partnerPal, u8 *stage)
{
    struct FusionRecord *rec = Fusion_FindRecordByPersonality(personality);

    if (rec == NULL)
        return FALSE;
    if (rec->partnerSpecies == SPECIES_NONE || rec->partnerSpecies >= NUM_SPECIES)
        return FALSE;
    LZ77UnCompWram(GetMonSpritePalFromSpeciesAndPersonality(rec->partnerSpecies, otId, rec->partnerPersonality), partnerPal);
    *stage = Fusion_GetRecordStage(rec);
    return TRUE;
}

// 50/50 blend of the two parent colors, then lifted toward a vivid glow by
// the fusion's evolution stage so each stage looks more intense.
static u16 BlendColorPair(u16 a, u16 b, u8 stage)
{
    s32 r = (GET_R(a) + GET_R(b)) / 2;
    s32 g = (GET_G(a) + GET_G(b)) / 2;
    s32 bl = (GET_B(a) + GET_B(b)) / 2;

    // +~9% toward white per stage.
    r += (31 - r) * 3 * stage / 32;
    g += (31 - g) * 3 * stage / 32;
    bl += (31 - bl) * 3 * stage / 32;
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (bl > 31) bl = 31;
    return RGB(r, g, bl);
}

// Blend a just-decompressed 16-color mon palette in place (before it is
// loaded to palette RAM).
void Fusion_BlendMonPalBuffer(u32 personality, u32 otId, u16 *palBuffer)
{
    u16 partnerPal[16];
    u8 stage;
    s32 i;

    if (!GetPartnerBlendPalette(personality, otId, partnerPal, &stage))
        return;
    for (i = 0; i < 16; i++)
        palBuffer[i] = BlendColorPair(palBuffer[i], partnerPal[i], stage);
}

// Blend a mon palette that is already loaded into palette RAM at palOffset.
void Fusion_BlendMonSpritePalette(u32 personality, u32 otId, u32 palOffset)
{
    u16 buffer[16];
    u16 partnerPal[16];
    u8 stage;
    s32 i;

    if (!GetPartnerBlendPalette(personality, otId, partnerPal, &stage))
        return;
    for (i = 0; i < 16; i++)
        buffer[i] = BlendColorPair(gPlttBufferUnfaded[palOffset + i], partnerPal[i], stage);
    LoadPalette(buffer, palOffset, 32);
}

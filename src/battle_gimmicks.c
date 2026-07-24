#include "global.h"
#include "gflib.h"
#include "util.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_scripts.h"
#include "battle_gimmicks.h"
#include "data.h"
#include "pokemon.h"
#include "pokemon_fusion.h"
#include "characters.h"
#include "sound.h"
#include "constants/battle.h"
#include "constants/battle_string_ids.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/songs.h"

#define DYNAMAX_TURN_COUNT 3

struct Pokemon *Gimmick_GetBattlerPartyMon(u8 battler)
{
    struct Pokemon *party = (GetBattlerSide(battler) == B_SIDE_PLAYER) ? gPlayerParty : gEnemyParty;

    return &party[gBattlerPartyIndexes[battler]];
}

static u16 HeldArtifact(u8 battler)
{
    return gBattleMons[battler].item;
}

// A battler "has" an artifact if it holds it in its real item slot OR it stored
// it in its fusion record (a fused mon keeps both source items and can hold a
// separate third item in its real slot).
static bool8 HasArtifact(u8 battler, u16 item)
{
    return HeldArtifact(battler) == item || Fusion_BattlerHasStoredItem(battler, item);
}

static bool8 HasBothMegaAndDyna(u8 battler)
{
    return HasArtifact(battler, ITEM_MEGA_STONE) && HasArtifact(battler, ITEM_DYNA_BAND);
}

static bool8 IsBattlerFused(u8 battler)
{
    struct Pokemon *mon = Gimmick_GetBattlerPartyMon(battler);

    return Fusion_IsMonFused(&mon->box);
}

static u16 MultiplyStat(u16 stat, u16 num, u16 den)
{
    u32 result = (u32)stat * num / den;

    if (result > 0xFFFF)
        result = 0xFFFF;
    return result;
}

void Gimmick_ApplyBattleStats(u8 battler)
{
    // Independent (not exclusive): a fused mon carrying BOTH a Mega Stone and a
    // Dyna Band gets both boosts stacked at once.
    if (HasArtifact(battler, ITEM_MEGA_STONE))
    {
        // Mega Evolution: +30% to every non-HP stat.
        gBattleMons[battler].attack    = MultiplyStat(gBattleMons[battler].attack, 13, 10);
        gBattleMons[battler].defense   = MultiplyStat(gBattleMons[battler].defense, 13, 10);
        gBattleMons[battler].speed     = MultiplyStat(gBattleMons[battler].speed, 13, 10);
        gBattleMons[battler].spAttack  = MultiplyStat(gBattleMons[battler].spAttack, 13, 10);
        gBattleMons[battler].spDefense = MultiplyStat(gBattleMons[battler].spDefense, 13, 10);
    }
    if (gBattleStruct->dynamaxTurns[battler] != 0)
    {
        // Dynamax swells the defenses; Gigantamax (a fused holder) more so.
        u16 num = (gBattleStruct->gigantamaxed & gBitTable[battler]) ? 7 : 3;
        u16 den = (gBattleStruct->gigantamaxed & gBitTable[battler]) ? 4 : 2;

        gBattleMons[battler].defense   = MultiplyStat(gBattleMons[battler].defense, num, den);
        gBattleMons[battler].spDefense = MultiplyStat(gBattleMons[battler].spDefense, num, den);
    }
}

bool8 Gimmick_TrySwitchInActivate(u8 battler)
{
    if (gBattleMons[battler].hp == 0)
        return FALSE;

    // Mega Evolution entrance (once per battle per battler).
    if (HasArtifact(battler, ITEM_MEGA_STONE)
     && !(gBattleStruct->megaEvolved & gBitTable[battler]))
    {
        gBattleStruct->megaEvolved |= gBitTable[battler];
        gBattleScripting.battler = battler;
        BattleScriptPushCursorAndCallback(BattleScript_MegaEvolutionActivates);
        return TRUE;
    }

    // Dynamax / Gigantamax entrance (once per battler while not active).
    if (HasArtifact(battler, ITEM_DYNA_BAND)
     && gBattleStruct->dynamaxTurns[battler] == 0
     && !(gBattleStruct->dynamaxed & gBitTable[battler]))
    {
        u32 heal;

        gBattleStruct->dynamaxed |= gBitTable[battler];
        gBattleStruct->dynamaxTurns[battler] = DYNAMAX_TURN_COUNT;
        if (IsBattlerFused(battler))
            gBattleStruct->gigantamaxed |= gBitTable[battler];
        else
            gBattleStruct->gigantamaxed &= ~gBitTable[battler];

        // A one-time HP surge (safe: never exceeds maxHP, no party writeback issue).
        heal = gBattleMons[battler].maxHP / 2;
        gBattleMons[battler].hp += heal;
        if (gBattleMons[battler].hp > gBattleMons[battler].maxHP)
            gBattleMons[battler].hp = gBattleMons[battler].maxHP;

        gBattleScripting.battler = battler;
        if (gBattleStruct->gigantamaxed & gBitTable[battler])
            BattleScriptPushCursorAndCallback(BattleScript_GigantamaxActivates);
        else
            BattleScriptPushCursorAndCallback(BattleScript_DynamaxActivates);
        return TRUE;
    }

    return FALSE;
}

void Gimmick_ApplyMovePower(u8 battler, u16 move)
{
    u16 power = gBattleMoves[move].power;

    gBattleStruct->zMoveThisMove &= ~gBitTable[battler];

    if (power == 0 || move == MOVE_FUSION_BURST)
        return; // status moves and the fusion signature keep their own rules

    // Z-Move: the first damaging move of the battle is supercharged.
    if (HasArtifact(battler, ITEM_Z_CRYSTAL)
     && !(gBattleStruct->zMoveUsed & gBitTable[battler]))
    {
        u32 boosted = power * 7 / 4;

        gBattleStruct->zMoveUsed |= gBitTable[battler];
        gBattleStruct->zMoveThisMove |= gBitTable[battler];
        gDynamicBasePower = min(boosted, 250);
        return;
    }

    // Dynamax / Gigantamax: damaging moves hit harder while active.
    if (gBattleStruct->dynamaxTurns[battler] != 0)
    {
        u16 num = (gBattleStruct->gigantamaxed & gBitTable[battler]) ? 7 : 3;
        u16 den = (gBattleStruct->gigantamaxed & gBitTable[battler]) ? 4 : 2;
        u32 boosted = power * num / den;

        gDynamicBasePower = min(boosted, 250);
    }
}

bool8 Gimmick_IsZMoveName(u8 battler, u16 move, u8 *dest)
{
    const u8 *name;
    u8 i, n = 0;

    if (!(gBattleStruct->zMoveThisMove & gBitTable[battler]) || move >= MOVES_COUNT)
        return FALSE;

    dest[n++] = CHAR_Z;
    dest[n++] = CHAR_HYPHEN;
    name = gMoveNames[move];
    for (i = 0; name[i] != EOS && n < MOVE_NAME_LENGTH; i++)
        dest[n++] = name[i];
    dest[n] = EOS;
    return TRUE;
}

// Per-type form-change tints, so every Pokemon's powered-up form looks distinct.
static const u16 sFormTypeColors[] =
{
    [TYPE_NORMAL]   = RGB2(30, 28, 26),
    [TYPE_FIGHTING] = RGB2(31, 14, 8),
    [TYPE_FLYING]   = RGB2(18, 24, 31),
    [TYPE_POISON]   = RGB2(24, 8, 28),
    [TYPE_GROUND]   = RGB2(28, 22, 10),
    [TYPE_ROCK]     = RGB2(24, 18, 10),
    [TYPE_BUG]      = RGB2(22, 28, 8),
    [TYPE_GHOST]    = RGB2(16, 8, 26),
    [TYPE_STEEL]    = RGB2(22, 24, 28),
    [TYPE_MYSTERY]  = RGB2(20, 20, 20),
    [TYPE_FIRE]     = RGB2(31, 12, 6),
    [TYPE_WATER]    = RGB2(6, 18, 31),
    [TYPE_GRASS]    = RGB2(12, 31, 10),
    [TYPE_ELECTRIC] = RGB2(31, 30, 8),
    [TYPE_PSYCHIC]  = RGB2(31, 8, 24),
    [TYPE_ICE]      = RGB2(14, 28, 31),
    [TYPE_DRAGON]   = RGB2(12, 12, 31),
    [TYPE_DARK]     = RGB2(14, 10, 16),
};

bool8 Gimmick_TryStartFormChange(u8 battler)
{
    u16 color, num;
    u8 type;
    bool8 fused, mega, dyna, zc, both;

    if (gBattleMons[battler].hp == 0)
        return FALSE;
    if (gBattleStruct->formChanged & gBitTable[battler])
        return FALSE; // once per battle per battler

    // Consider both the real item slot AND the two items a fused mon stored.
    mega = HasArtifact(battler, ITEM_MEGA_STONE);
    dyna = HasArtifact(battler, ITEM_DYNA_BAND);
    zc = HasArtifact(battler, ITEM_Z_CRYSTAL);
    fused = IsBattlerFused(battler);
    both = mega && dyna; // a fused mon carrying both forms
    if (!mega && !dyna && !zc && !fused)
        return FALSE; // nothing to transform into

    gBattleStruct->formChanged |= gBitTable[battler];

    // A fused mon that stored BOTH a Mega Stone and a Dyna Band gets the biggest
    // boost of all (x3 every stat); anything else gets a huge x2.
    num = both ? 3 : 2;
    gBattleMons[battler].attack    = MultiplyStat(gBattleMons[battler].attack, num, 1);
    gBattleMons[battler].defense   = MultiplyStat(gBattleMons[battler].defense, num, 1);
    gBattleMons[battler].speed     = MultiplyStat(gBattleMons[battler].speed, num, 1);
    gBattleMons[battler].spAttack  = MultiplyStat(gBattleMons[battler].spAttack, num, 1);
    gBattleMons[battler].spDefense = MultiplyStat(gBattleMons[battler].spDefense, num, 1);
    // HP surge (heal only; maxHP unchanged so the health bar stays consistent).
    gBattleMons[battler].hp += gBattleMons[battler].maxHP / 2;
    if (gBattleMons[battler].hp > gBattleMons[battler].maxHP)
        gBattleMons[battler].hp = gBattleMons[battler].maxHP;

    // "Cooler look" recolor. The both-forms fusion gets a unique radiant
    // white-gold double blend; every other form is tinted by the mon's type so
    // each transformation looks different.
    if (both)
    {
        BlendPalette(OBJ_PLTT_ID(battler), 16, 6, RGB2(31, 31, 31)); // brighten
        BlendPalette(OBJ_PLTT_ID(battler), 16, 8, RGB2(31, 24, 12)); // radiant gold
    }
    else
    {
        type = gBattleMons[battler].type1;
        color = (type < ARRAY_COUNT(sFormTypeColors)) ? sFormTypeColors[type] : RGB2(28, 20, 31);
        BlendPalette(OBJ_PLTT_ID(battler), 16, 9, color);
    }

    PlaySE(SE_M_MEGA_KICK);
    return TRUE;
}

void Gimmick_EndTurnDynamaxCountdown(void)
{
    u8 battler;

    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (gBattleStruct->dynamaxTurns[battler] == 0)
            continue;
        if (--gBattleStruct->dynamaxTurns[battler] == 0)
        {
            struct Pokemon *mon = Gimmick_GetBattlerPartyMon(battler);

            // Revert the Dynamax defense boost by reloading the base stats.
            gBattleMons[battler].defense = GetMonData(mon, MON_DATA_DEF, NULL);
            gBattleMons[battler].spDefense = GetMonData(mon, MON_DATA_SPDEF, NULL);
            gBattleStruct->dynamaxed &= ~gBitTable[battler];
            gBattleStruct->gigantamaxed &= ~gBitTable[battler];
        }
    }
}

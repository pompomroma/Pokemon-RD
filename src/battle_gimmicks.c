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

// Per-type Z-Move base power. Each type's Z-Move hits for a different, fixed
// power (Sun/Moon style), so a fused/typed mon's Z-Move feels distinct.
static const u8 sZMovePowerByType[] =
{
    [TYPE_NORMAL]   = 200,
    [TYPE_FIGHTING] = 190,
    [TYPE_FLYING]   = 170,
    [TYPE_POISON]   = 175,
    [TYPE_GROUND]   = 180,
    [TYPE_ROCK]     = 180,
    [TYPE_BUG]      = 170,
    [TYPE_GHOST]    = 175,
    [TYPE_STEEL]    = 180,
    [TYPE_MYSTERY]  = 180,
    [TYPE_FIRE]     = 185,
    [TYPE_WATER]    = 185,
    [TYPE_GRASS]    = 185,
    [TYPE_ELECTRIC] = 175,
    [TYPE_PSYCHIC]  = 185,
    [TYPE_ICE]      = 180,
    [TYPE_DRAGON]   = 190,
    [TYPE_DARK]     = 180,
};

// Is the battler allowed to charge `move` into its Z-Move right now?
bool8 Gimmick_CanArmZMove(u8 battler, u16 move)
{
    if (move == MOVE_NONE || move == MOVE_FUSION_BURST || move >= MOVES_COUNT)
        return FALSE;
    if (gBattleMoves[move].power == 0) // status moves have no Z-Move
        return FALSE;
    if (gBattleStruct->zMoveUsed & gBitTable[battler]) // one per battle
        return FALSE;
    return HasArtifact(battler, ITEM_Z_CRYSTAL);
}

bool8 Gimmick_IsZMoveArmed(u8 battler)
{
    return (gBattleStruct->zMoveArmed & gBitTable[battler]) != 0;
}

// SELECT toggles the Z-Move on/off for the chosen move; returns the new state.
bool8 Gimmick_ToggleArmZMove(u8 battler)
{
    if (gBattleStruct->zMoveArmed & gBitTable[battler])
    {
        gBattleStruct->zMoveArmed &= ~gBitTable[battler];
        return FALSE;
    }
    gBattleStruct->zMoveArmed |= gBitTable[battler];
    return TRUE;
}

void Gimmick_ApplyMovePower(u8 battler, u16 move)
{
    u16 power = gBattleMoves[move].power;

    gBattleStruct->zMoveThisMove &= ~gBitTable[battler];

    if (power == 0 || move == MOVE_FUSION_BURST)
    {
        gBattleStruct->zMoveArmed &= ~gBitTable[battler]; // can't Z a status move
        return; // status moves and the fusion signature keep their own rules
    }

    // Z-Move: fired only when the player armed it (SELECT on the move screen),
    // once per battle. Power is the type's fixed Z base power (min the move's own
    // supercharge), so every type's Z-Move is distinct.
    if ((gBattleStruct->zMoveArmed & gBitTable[battler])
     && !(gBattleStruct->zMoveUsed & gBitTable[battler]))
    {
        u8 type = gBattleMoves[move].type;
        u32 zPower = (type < ARRAY_COUNT(sZMovePowerByType)) ? sZMovePowerByType[type] : 180;
        u32 boosted = max(power * 2, zPower);

        gBattleStruct->zMoveArmed &= ~gBitTable[battler];
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

// Names shown in the move-info window when a form change fires, so the player
// can see which form they triggered.
static const u8 sText_FormKrypton[] = _("KRYPTON!");
static const u8 sText_FormGiga[]    = _("G-MAX!");
static const u8 sText_FormMega[]    = _("MEGA!");
static const u8 sText_FormDyna[]    = _("DYNAMAX!");
static const u8 sText_FormAwaken[]  = _("AWAKEN!");

static const u8 *sLastFormName = NULL;

const u8 *Gimmick_GetFormChangeName(void)
{
    return sLastFormName;
}

bool8 Gimmick_CanKryptonEvolve(u8 battler)
{
    // KRYPTON EVOLUTION is reserved for a fused Pokemon built from one holder of
    // a Mega Stone and one holder of a Dyna Band -- both items are kept in the
    // fusion record, which leaves the real item slot free for a Z Crystal.
    return IsBattlerFused(battler) && HasBothMegaAndDyna(battler);
}

bool8 Gimmick_TryStartFormChange(u8 battler)
{
    u16 color, num;
    u8 type;
    bool8 fused, mega, dyna, zc, krypton;

    if (gBattleMons[battler].hp == 0)
        return FALSE;
    if (gBattleStruct->formChanged & gBitTable[battler])
        return FALSE; // once per battle per battler

    // Consider both the real item slot AND the two items a fused mon stored.
    mega = HasArtifact(battler, ITEM_MEGA_STONE);
    dyna = HasArtifact(battler, ITEM_DYNA_BAND);
    zc = HasArtifact(battler, ITEM_Z_CRYSTAL);
    fused = IsBattlerFused(battler);
    krypton = fused && mega && dyna;
    if (!mega && !dyna && !zc && !fused)
        return FALSE; // nothing to transform into

    gBattleStruct->formChanged |= gBitTable[battler];

    // KRYPTON EVOLUTION quadruples every stat. Every other form keeps the huge
    // x2 it already had.
    num = krypton ? 4 : 2;
    gBattleMons[battler].attack    = MultiplyStat(gBattleMons[battler].attack, num, 1);
    gBattleMons[battler].defense   = MultiplyStat(gBattleMons[battler].defense, num, 1);
    gBattleMons[battler].speed     = MultiplyStat(gBattleMons[battler].speed, num, 1);
    gBattleMons[battler].spAttack  = MultiplyStat(gBattleMons[battler].spAttack, num, 1);
    gBattleMons[battler].spDefense = MultiplyStat(gBattleMons[battler].spDefense, num, 1);
    // HP surge (heal only; maxHP unchanged so the health bar stays consistent).
    gBattleMons[battler].hp += gBattleMons[battler].maxHP / 2;
    if (gBattleMons[battler].hp > gBattleMons[battler].maxHP)
        gBattleMons[battler].hp = gBattleMons[battler].maxHP;

    // Each form gets its own look rather than one shared recolor:
    //   KRYPTON    radiant white-gold, brightest of all
    //   GIGANTAMAX deep violet swell (a fused Dyna Band holder)
    //   DYNAMAX    heavy crimson
    //   MEGA       hard bright rim, keeping the mon's own colours readable
    //   otherwise  tinted by the mon's primary type, so species still differ
    if (krypton)
    {
        sLastFormName = sText_FormKrypton;
        BlendPalette(OBJ_PLTT_ID(battler), 16, 8, RGB2(31, 31, 31)); // blazing core
        BlendPalette(OBJ_PLTT_ID(battler), 16, 9, RGB2(31, 26, 10)); // radiant gold
    }
    else if (dyna && fused)
    {
        sLastFormName = sText_FormGiga;
        BlendPalette(OBJ_PLTT_ID(battler), 16, 10, RGB2(20, 6, 28));
    }
    else if (dyna)
    {
        sLastFormName = sText_FormDyna;
        BlendPalette(OBJ_PLTT_ID(battler), 16, 10, RGB2(30, 6, 8));
    }
    else if (mega)
    {
        sLastFormName = sText_FormMega;
        BlendPalette(OBJ_PLTT_ID(battler), 16, 5, RGB2(31, 31, 31)); // rim lift
        BlendPalette(OBJ_PLTT_ID(battler), 16, 6, RGB2(16, 24, 31)); // cool mega sheen
    }
    else
    {
        sLastFormName = sText_FormAwaken;
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

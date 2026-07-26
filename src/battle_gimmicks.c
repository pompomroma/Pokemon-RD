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

// Rebuilds the battler's live stats from its party mon and re-applies whichever
// gimmick boosts are currently active. Written as a recompute rather than an
// in-place multiply so it is IDEMPOTENT: the engine calls it from four places
// (switch-in, SwitchInClearSetData, Baton Pass, intro), and one of those does not
// reload the base stats first, so multiplying in place would compound the boost.
void Gimmick_ApplyBattleStats(u8 battler)
{
    struct Pokemon *mon = Gimmick_GetBattlerPartyMon(battler);
    u16 atk, def, spe, spa, spd;

    // Transformed battlers wear another mon's stats; leave those alone.
    if (gBattleMons[battler].species != GetMonData(mon, MON_DATA_SPECIES, NULL))
        return;

    atk = GetMonData(mon, MON_DATA_ATK, NULL);
    def = GetMonData(mon, MON_DATA_DEF, NULL);
    spe = GetMonData(mon, MON_DATA_SPEED, NULL);
    spa = GetMonData(mon, MON_DATA_SPATK, NULL);
    spd = GetMonData(mon, MON_DATA_SPDEF, NULL);

    if (gBattleStruct->kryptonEvolved & gBitTable[battler])
    {
        // KRYPTON EVOLUTION quadruples every stat outright. It supersedes Mega
        // and Dynamax rather than stacking with them -- an eligible mon qualifies
        // for all three, and this is the top tier.
        atk = MultiplyStat(atk, 4, 1);
        def = MultiplyStat(def, 4, 1);
        spe = MultiplyStat(spe, 4, 1);
        spa = MultiplyStat(spa, 4, 1);
        spd = MultiplyStat(spd, 4, 1);
    }
    else
    {
        if (HasArtifact(battler, ITEM_MEGA_STONE))
        {
            // Mega Evolution: +30% to every non-HP stat.
            atk = MultiplyStat(atk, 13, 10);
            def = MultiplyStat(def, 13, 10);
            spe = MultiplyStat(spe, 13, 10);
            spa = MultiplyStat(spa, 13, 10);
            spd = MultiplyStat(spd, 13, 10);
        }
        if (gBattleStruct->dynamaxTurns[battler] != 0)
        {
            // Dynamax swells the defenses; Gigantamax (a fused holder) more so.
            u16 num = (gBattleStruct->gigantamaxed & gBitTable[battler]) ? 7 : 3;
            u16 den = (gBattleStruct->gigantamaxed & gBitTable[battler]) ? 4 : 2;

            def = MultiplyStat(def, num, den);
            spd = MultiplyStat(spd, num, den);
        }
    }

    gBattleMons[battler].attack    = atk;
    gBattleMons[battler].defense   = def;
    gBattleMons[battler].speed     = spe;
    gBattleMons[battler].spAttack  = spa;
    gBattleMons[battler].spDefense = spd;
}

// Marks the battler as Krypton-evolved and puts the x4 stats live. Safe to call
// more than once (the stat pass recomputes from the party mon's base stats).
static void ApplyKryptonEvolution(u8 battler)
{
    gBattleStruct->kryptonEvolved |= gBitTable[battler];
    // Krypton stands in for the lesser entrances, so their cutscenes never fire
    // on top of it and their (weaker) boosts never overwrite the x4.
    gBattleStruct->megaEvolved |= gBitTable[battler];
    gBattleStruct->dynamaxed |= gBitTable[battler];
    gBattleStruct->gigantamaxed &= ~gBitTable[battler];
    // Krypton is permanent. Leaving a Dynamax countdown running would reload the
    // base defenses when it expired and silently undo the x4.
    gBattleStruct->dynamaxTurns[battler] = 0;

    Gimmick_ApplyBattleStats(battler);

    // The same HP surge the other forms grant (heal only; maxHP is untouched so
    // the health bar stays consistent).
    gBattleMons[battler].hp += gBattleMons[battler].maxHP / 2;
    if (gBattleMons[battler].hp > gBattleMons[battler].maxHP)
        gBattleMons[battler].hp = gBattleMons[battler].maxHP;
}

bool8 Gimmick_TrySwitchInActivate(u8 battler)
{
    if (gBattleMons[battler].hp == 0)
        return FALSE;

    // KRYPTON EVOLUTION entrance -- checked FIRST, because a mon that qualifies
    // also holds a Mega Stone and a Dyna Band and would otherwise be caught by
    // the Mega branch below and never reach this tier.
    if (Gimmick_CanKryptonEvolve(battler)
     && !(gBattleStruct->kryptonEvolved & gBitTable[battler]))
    {
        ApplyKryptonEvolution(battler);
        gBattleScripting.battler = battler;
        BattleScriptPushCursorAndCallback(BattleScript_KryptonEvolutionActivates);
        return TRUE;
    }

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

    // KRYPTON EVOLUTION keeps the Gigantamax power bonus permanently -- it
    // replaces Dynamax, so without this the top tier would hit softer than the
    // tier below it once the countdown it suppressed would have been running.
    if (gBattleStruct->kryptonEvolved & gBitTable[battler])
    {
        gDynamicBasePower = min(power * 7 / 4, 250);
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
    bool8 fused, mega, dyna, zc;

    if (gBattleMons[battler].hp == 0)
        return FALSE;
    if (gBattleStruct->formChanged & gBitTable[battler])
        return FALSE; // once per battle per battler

    // KRYPTON EVOLUTION on demand. It normally fires by itself the moment an
    // eligible mon enters the battle, so this is the manual path for a mon that
    // became eligible mid-battle (e.g. it was given its Z Crystal after sending
    // out). Routed through the same applier so the x4 can never double up.
    if (Gimmick_CanKryptonEvolve(battler))
    {
        if (gBattleStruct->kryptonEvolved & gBitTable[battler])
            return FALSE; // already Krypton-evolved this battle
        gBattleStruct->formChanged |= gBitTable[battler];
        ApplyKryptonEvolution(battler);
        sLastFormName = sText_FormKrypton;
        BlendPalette(OBJ_PLTT_ID(battler), 16, 8, RGB2(31, 31, 31)); // blazing core
        BlendPalette(OBJ_PLTT_ID(battler), 16, 9, RGB2(31, 26, 10)); // radiant gold
        PlaySE(SE_M_MEGA_KICK);
        return TRUE;
    }

    // Consider both the real item slot AND the two items a fused mon stored.
    mega = HasArtifact(battler, ITEM_MEGA_STONE);
    dyna = HasArtifact(battler, ITEM_DYNA_BAND);
    zc = HasArtifact(battler, ITEM_Z_CRYSTAL);
    fused = IsBattlerFused(battler);
    if (!mega && !dyna && !zc && !fused)
        return FALSE; // nothing to transform into

    gBattleStruct->formChanged |= gBitTable[battler];

    num = 2; // Krypton (x4) is handled above; every other form keeps its x2.
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
    //   GIGANTAMAX deep violet swell (a fused Dyna Band holder)
    //   DYNAMAX    heavy crimson
    //   MEGA       hard bright rim, keeping the mon's own colours readable
    //   otherwise  tinted by the mon's primary type, so species still differ
    // (KRYPTON's radiant white-gold is applied on its own path above.)
    if (dyna && fused)
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

#ifndef GUARD_BATTLE_GIMMICKS_H
#define GUARD_BATTLE_GIMMICKS_H

#include "global.h"

// Held-item battle transformations, imitating the presentation of the
// Radical Red hack with FireRed's own effect assets:
//  - MEGA STONE  -> Mega Evolution: +30% to all non-HP stats on send-in.
//  - Z CRYSTAL   -> Z-Move: the holder's first damaging move of the battle
//                   is renamed "Z-<MOVE>" and its power is boosted ~1.75x.
//  - DYNA BAND   -> Dynamax: heals on send-in, +50% defenses and +50% move
//                   power for 3 turns. A fused holder Gigantamaxes instead
//                   (stronger multipliers, purple aura).
// The three are mutually exclusive per Pokemon (a mon holds one item).

struct Pokemon *Gimmick_GetBattlerPartyMon(u8 battler);

// Applied at every point gBattleMons[battler] is (re)derived, so the stat
// changes persist across switches. No-op when the battler holds no artifact.
void Gimmick_ApplyBattleStats(u8 battler);

// Called from the switch-in effect loop. Returns TRUE (and queues a
// cutscene battle script) when a Mega/Dynamax entrance fires this call.
bool8 Gimmick_TrySwitchInActivate(u8 battler);

// Called when a move begins. Applies Z-Move / Dynamax power boosts through
// gDynamicBasePower and flags a Z-Move for the name-override path.
void Gimmick_ApplyMovePower(u8 battler, u16 move);

// TRUE if the battler's current move should display with the "Z-" prefix.
bool8 Gimmick_IsZMoveName(u8 battler, u16 move, u8 *dest);

// Z-Move arming, from the move-selection screen (SELECT button). A holder of a
// Z Crystal (or a fused mon that stored one) that has not spent its Z-Move yet
// can "charge" the highlighted damaging move into its Z-Move: SELECT toggles it.
bool8 Gimmick_CanArmZMove(u8 battler, u16 move); // eligible to arm this move?
bool8 Gimmick_IsZMoveArmed(u8 battler);          // currently armed?
bool8 Gimmick_ToggleArmZMove(u8 battler);        // flip armed state; TRUE if now armed

// End-of-turn Dynamax countdown; reverts stats when it expires.
void Gimmick_EndTurnDynamaxCountdown(void);

// Manual "form change", triggered by pressing START on the move-selection
// screen. A holder of a Mega Stone / Dyna Band / Z Crystal (or any fused mon)
// powers up ONCE per battle: a huge boost to every stat plus a vivid palette
// recolor. A fused mon channels both forms for the biggest boost of all.
// Returns TRUE if a form change fired this call.
bool8 Gimmick_TryStartFormChange(u8 battler);

// KRYPTON EVOLUTION: the top form change, x4 to every stat. Only a fused
// Pokemon built from a Mega Stone holder + a Dyna Band holder qualifies (both
// items live in the fusion record, so its real item slot is still free to hold
// a Z Crystal).
bool8 Gimmick_CanKryptonEvolve(u8 battler);

// Short label of the form that just triggered ("KRYPTON!", "MEGA!", ...), shown
// in the move-info window so the player sees which form fired.
const u8 *Gimmick_GetFormChangeName(void);

#endif // GUARD_BATTLE_GIMMICKS_H

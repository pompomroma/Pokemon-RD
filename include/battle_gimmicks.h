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

// End-of-turn Dynamax countdown; reverts stats when it expires.
void Gimmick_EndTurnDynamaxCountdown(void);

// Manual "form change", triggered by pressing START on the move-selection
// screen. A holder of a Mega Stone / Dyna Band / Z Crystal (or any fused mon)
// powers up ONCE per battle: a huge boost to every stat plus a vivid palette
// recolor. A fused mon channels both forms for the biggest boost of all.
// Returns TRUE if a form change fired this call.
bool8 Gimmick_TryStartFormChange(u8 battler);

#endif // GUARD_BATTLE_GIMMICKS_H

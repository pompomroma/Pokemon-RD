#ifndef GUARD_MOVE_STYLE_H
#define GUARD_MOVE_STYLE_H

#include "global.h"
#include "pokemon.h"

// Attack and Sp. Atk no longer grow at the same rate for every mon: whichever
// kind of attacking move a mon actually uses is the stat that grows faster.
//
// Every landed damaging move is tallied as physical or special (Gen 3 decides
// that from the move's TYPE -- see IS_TYPE_PHYSICAL in include/battle.h), and
// the running tally biases the Attack and Sp. Atk results at level-up time.
// The two stats move in opposite directions by the same amount, so a mon's
// total offensive output stays fair -- it just specialises.

// Tallies one landed damaging move for a player-party mon.
void MoveStyle_RecordUse(u32 personality, bool8 isPhysical);

// -100 (purely special) .. +100 (purely physical); 0 until there is enough data.
s32 MoveStyle_GetLean(struct Pokemon *mon);

// Applies the lean to a freshly calculated STAT_ATK / STAT_SPATK value.
// Any other stat is returned untouched.
s32 MoveStyle_ApplyToStat(struct Pokemon *mon, s32 value, u8 statIndex);

// Drops a mon's tally (called when a party slot is reused by a different mon).
void MoveStyle_Clear(u32 personality);

#endif // GUARD_MOVE_STYLE_H

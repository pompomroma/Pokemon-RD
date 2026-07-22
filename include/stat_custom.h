#ifndef GUARD_STAT_CUSTOM_H
#define GUARD_STAT_CUSTOM_H

#include "global.h"
#include "pokemon.h"

// Round 4 "custom stat system":
//  - Stat Arrange: redistribute the six base stats (they still sum to the
//    mon's base-stat total) into any shape the player likes.
//  - Talent: one stat gets +15% (upside) and one gets -15% (downside).
// Both are stored per-mon in gSaveBlock2Ptr->statCustom, keyed by
// personality + otId (mirrors the fusion record system).

#define STAT_CUSTOM_TALENT_UP(t)   ((t) >> 4)
#define STAT_CUSTOM_TALENT_DOWN(t) ((t) & 0xF)
#define STAT_CUSTOM_MAKE_TALENT(up, down) (((up) << 4) | (down))

struct StatCustomRecord *StatCustom_FindRecord(struct BoxPokemon *boxMon);
bool8 StatCustom_HasAllocation(struct BoxPokemon *boxMon);

// The fusion-aware "effective" base stat and the pool (sum) the editor arranges.
u16 StatCustom_GetEffectiveBase(struct Pokemon *mon, u8 statIndex);
u16 StatCustom_GetBaseTotal(struct Pokemon *mon);

// Stat-calc hooks (used inside CalculateMonStats).
bool8 StatCustom_MonHasAllocation(struct Pokemon *mon);
u16 StatCustom_GetAllocBase(struct Pokemon *mon, u8 statIndex);
s32 StatCustom_ApplyTalent(struct Pokemon *mon, s32 value, u8 statIndex);

// Write the edited allocation + talent back and free the slot when default.
bool8 StatCustom_Save(struct Pokemon *mon, const u8 allocStats[6], u8 talent);
u8 StatCustom_GetTalent(struct Pokemon *mon); // packed, or STAT_CUSTOM_TALENT_NONE

#endif // GUARD_STAT_CUSTOM_H

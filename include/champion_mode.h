#ifndef GUARD_CHAMPION_MODE_H
#define GUARD_CHAMPION_MODE_H

#include "global.h"

// The hardest difficulty tier ("AW SHIBAL GAY BBAKSAY", DIFFICULTY_CHAMPION) is
// normally locked in at the start of a save. Once the player has cleared the
// game they can switch into it from the OPTION menu, and doing so is what
// reveals BBAKSAYON -- the original legendary made for this hack.
//
// The unlock needs no new save data: FLAG_SYS_GAME_CLEAR and
// gSaveBlock2Ptr->difficulty are both already persistent.

bool8 ChampionMode_IsActive(void);      // currently on the hardest tier
bool8 ChampionMode_CanSwitch(void);     // game cleared, so the tier is choosable
bool8 Bbaksayon_IsUnlocked(void);       // both of the above
void ChampionMode_SetDifficulty(u8 difficulty);
const u8 *ChampionMode_GetDifficultyName(u8 difficulty);

#endif // GUARD_CHAMPION_MODE_H

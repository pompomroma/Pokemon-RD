#ifndef GUARD_LEGENDARY_SELECT_H
#define GUARD_LEGENDARY_SELECT_H

#include "global.h"

// Randomizer-mode starter: OAK's legendary vault. Lists every legendary the ROM
// has (all 21 Gen 1-3 ones) with its sprite, and the player takes two.
void CB2_InitLegendarySelect(void);
void ChooseTwoLegendaries(void); // script special; pair with `waitstate`

#endif // GUARD_LEGENDARY_SELECT_H

#ifndef GUARD_RANDOMIZER_H
#define GUARD_RANDOMIZER_H

#include "global.h"

// Randomizer mode (chosen at new game). When active, species are remapped
// through a stable, seed-driven hash so wild encounters, trainer parties,
// and the starter are shuffled deterministically for the save.

bool8 Randomizer_IsActive(void);

// Returns the remapped species for `species`, or `species` unchanged when
// the randomizer is off or the input is out of the remappable range. Stable:
// the same (seed, species) always yields the same result.
u16 GetRandomizedSpecies(u16 species);

#endif // GUARD_RANDOMIZER_H

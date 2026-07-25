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

// Like GetRandomizedSpecies, but for the starter: returns a legendary the
// overwhelming majority of the time (see STARTER_LEGENDARY_CHANCE). Deterministic
// on (seed, species) so the shown starter and the gifted one always match.
u16 GetRandomizedStarterSpecies(u16 species);

// Wild encounters: usually returns `species`, but a set percentage of the time
// swaps in a random legendary. NOT gated on randomizer mode -- legendaries roam
// the wild in every save, at the encounter slot's own level.
u16 Wild_ApplyLegendaryChance(u16 species);

#endif // GUARD_RANDOMIZER_H

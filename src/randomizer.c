#include "global.h"
#include "random.h"
#include "randomizer.h"
#include "constants/species.h"

// Deterministic species remap. The map is a stable hash of (seed, species)
// so it never needs to be stored and is identical every time the game reads a
// species; it is not strictly bijective (some species may repeat or be
// absent), which is expected behaviour for a casual randomizer.

bool8 Randomizer_IsActive(void)
{
    return gSaveBlock2Ptr->randomizerMode != 0;
}

u16 GetRandomizedSpecies(u16 species)
{
    u32 h;

    if (!Randomizer_IsActive())
        return species;
    // Leave "no species", out-of-range, and the egg sentinel untouched.
    if (species == SPECIES_NONE || species >= SPECIES_EGG)
        return species;

    // Two ISO_RANDOMIZE rounds over (seed, species) — a pure hash, so it does
    // not disturb the global RNG (gRngValue).
    h = ISO_RANDOMIZE1(gSaveBlock2Ptr->randomizerSeed + species * 2246822519u);
    h = ISO_RANDOMIZE2(h ^ (species << 3));
    return 1 + (h >> 8) % (NUM_SPECIES - 1); // 1 .. NUM_SPECIES-1
}

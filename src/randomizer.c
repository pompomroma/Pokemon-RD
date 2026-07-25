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

// The 25 SPECIES_OLD_UNOWN_B..Z slots (252..276) are Gen-2 placeholders with
// generic 50/150 stats and a "??" sprite — never real Pokemon. We map around
// them so the randomizer only ever produces genuine Gen 1-3 species (which
// already includes every legendary, Articuno 144 .. Deoxys 410).
#define RANDOMIZER_JUNK_COUNT (SPECIES_OLD_UNOWN_Z - SPECIES_OLD_UNOWN_B + 1) // 25
#define RANDOMIZER_POOL_SIZE  ((NUM_SPECIES - 1) - RANDOMIZER_JUNK_COUNT)     // 386 real species

u16 GetRandomizedSpecies(u16 species)
{
    u32 h, idx;

    if (!Randomizer_IsActive())
        return species;
    // Leave "no species", out-of-range, and the egg sentinel untouched.
    if (species == SPECIES_NONE || species >= SPECIES_EGG)
        return species;

    // Two ISO_RANDOMIZE rounds over (seed, species) — a pure hash, so it does
    // not disturb the global RNG (gRngValue).
    h = ISO_RANDOMIZE1(gSaveBlock2Ptr->randomizerSeed + species * 2246822519u);
    h = ISO_RANDOMIZE2(h ^ (species << 3));

    // Pick an index into the real-species pool, then fold it back onto an actual
    // species id, jumping over the OLD_UNOWN placeholder gap.
    idx = (h >> 8) % RANDOMIZER_POOL_SIZE; // 0 .. 385
    if (idx < SPECIES_OLD_UNOWN_B - 1)     // 0 .. 250  -> Bulbasaur 1 .. Celebi 251
        return idx + 1;
    return SPECIES_TREECKO + (idx - (SPECIES_OLD_UNOWN_B - 1)); // Treecko 277 .. Chimecho 411
}

// Every Gen 1-3 legendary/mythical. The randomized STARTER is drawn from this
// pool the overwhelming majority of the time, so a randomizer run almost always
// begins with a legendary.
static const u16 sLegendaryStarters[] =
{
    SPECIES_ARTICUNO, SPECIES_ZAPDOS,  SPECIES_MOLTRES, SPECIES_MEWTWO,  SPECIES_MEW,
    SPECIES_RAIKOU,   SPECIES_ENTEI,   SPECIES_SUICUNE, SPECIES_LUGIA,   SPECIES_HO_OH,
    SPECIES_CELEBI,   SPECIES_REGIROCK,SPECIES_REGICE,  SPECIES_REGISTEEL,
    SPECIES_LATIAS,   SPECIES_LATIOS,  SPECIES_KYOGRE,  SPECIES_GROUDON, SPECIES_RAYQUAZA,
    SPECIES_JIRACHI,  SPECIES_DEOXYS,
};

// The percentage chance a randomized starter is a legendary (massively boosted
// from the ~5% a uniform pick would give).
#define STARTER_LEGENDARY_CHANCE 90

// Starter-specific remap: with STARTER_LEGENDARY_CHANCE it returns a legendary,
// otherwise it falls back to the ordinary any-species remap. Deterministic on
// (seed, species) like GetRandomizedSpecies, so the lab's shown Pokemon and the
// gift always agree. Both the display path (BufferRandomizedStarterSpecies) and
// the gift path (ScrCmd_givemon on an empty party) must route through this.
u16 GetRandomizedStarterSpecies(u16 species)
{
    u32 h;

    if (!Randomizer_IsActive())
        return species;
    if (species == SPECIES_NONE || species >= SPECIES_EGG)
        return species;

    // A separate hash from GetRandomizedSpecies (different constants) so the
    // starter roll is independent of the wild/trainer remap.
    h = ISO_RANDOMIZE1(gSaveBlock2Ptr->randomizerSeed + species * 40503u + 0x9E3779B9u);
    h = ISO_RANDOMIZE2(h ^ (species << 5));

    if ((h >> 8) % 100 < STARTER_LEGENDARY_CHANCE)
        return sLegendaryStarters[(h >> 16) % ARRAY_COUNT(sLegendaryStarters)];
    return GetRandomizedSpecies(species);
}

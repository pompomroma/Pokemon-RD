#include "global.h"
#include "pokemon.h"
#include "move_style.h"
#include "constants/pokemon.h"

// See include/move_style.h.

// Strongest swing the lean can produce, as a percentage of the stat. The two
// stats move by this much in opposite directions, so +-12% is a noticeable
// specialisation without making the "wrong" stat useless.
#define MOVE_STYLE_MAX_BIAS 12

// Uses needed before the lean reaches full strength. Below this the bias ramps
// in proportionally, so a couple of early moves cannot swing a mon's build.
#define MOVE_STYLE_CONFIDENCE 30

// Counters halve when either side reaches this, which keeps them in range and
// lets a mon that changes its moveset drift to a new lean instead of being
// locked in by its first hundred attacks.
#define MOVE_STYLE_DECAY_AT 4000

// This table was carved out of SaveBlock2's trailing filler; the block must
// keep its exact size and encryptionKey must not move, or saves stop decrypting.
STATIC_ASSERT(sizeof(struct MoveStyleRecord) == 8, MoveStyleRecordSize);
STATIC_ASSERT(sizeof(struct SaveBlock2) == 0xF24, SaveBlock2SizeUnchangedByMoveStyle);
STATIC_ASSERT(offsetof(struct SaveBlock2, moveStyle) == 0xE90, MoveStyleOffset);
STATIC_ASSERT(offsetof(struct SaveBlock2, encryptionKey) == 0xF20, EncryptionKeyStillAtF20);

static struct MoveStyleRecord *FindByPersonality(u32 personality)
{
    s32 i;

    for (i = 0; i < MOVE_STYLE_RECORDS_COUNT; i++)
    {
        struct MoveStyleRecord *rec = &gSaveBlock2Ptr->moveStyle[i];
        if ((rec->physUses != 0 || rec->specUses != 0) && rec->personality == personality)
            return rec;
    }
    return NULL;
}

// Claims a slot for a mon that has none. Party slots are reused constantly, so
// when the table is full the least-used record is recycled rather than failing.
static struct MoveStyleRecord *FindOrClaim(u32 personality)
{
    struct MoveStyleRecord *rec = FindByPersonality(personality);
    struct MoveStyleRecord *weakest;
    u32 weakestTotal;
    s32 i;

    if (rec != NULL)
        return rec;

    weakest = &gSaveBlock2Ptr->moveStyle[0];
    weakestTotal = 0xFFFFFFFF;
    for (i = 0; i < MOVE_STYLE_RECORDS_COUNT; i++)
    {
        struct MoveStyleRecord *cur = &gSaveBlock2Ptr->moveStyle[i];
        u32 total = cur->physUses + cur->specUses;

        if (total == 0)
        {
            weakest = cur;
            break;
        }
        if (total < weakestTotal)
        {
            weakestTotal = total;
            weakest = cur;
        }
    }

    weakest->personality = personality;
    weakest->physUses = 0;
    weakest->specUses = 0;
    return weakest;
}

void MoveStyle_RecordUse(u32 personality, bool8 isPhysical)
{
    struct MoveStyleRecord *rec = FindOrClaim(personality);

    if (isPhysical)
        rec->physUses++;
    else
        rec->specUses++;

    if (rec->physUses >= MOVE_STYLE_DECAY_AT || rec->specUses >= MOVE_STYLE_DECAY_AT)
    {
        rec->physUses /= 2;
        rec->specUses /= 2;
    }
}

void MoveStyle_Clear(u32 personality)
{
    struct MoveStyleRecord *rec = FindByPersonality(personality);

    if (rec != NULL)
        *rec = (struct MoveStyleRecord){0};
}

s32 MoveStyle_GetLean(struct Pokemon *mon)
{
    struct MoveStyleRecord *rec = FindByPersonality(GetMonData(mon, MON_DATA_PERSONALITY, NULL));
    s32 phys, spec, total, lean;

    if (rec == NULL)
        return 0;

    phys = rec->physUses;
    spec = rec->specUses;
    total = phys + spec;
    if (total == 0)
        return 0;

    // How lopsided the usage is, -100 (all special) .. +100 (all physical).
    lean = ((phys - spec) * 100) / total;

    // Ramp in over the first MOVE_STYLE_CONFIDENCE uses.
    if (total < MOVE_STYLE_CONFIDENCE)
        lean = (lean * total) / MOVE_STYLE_CONFIDENCE;

    return lean;
}

s32 MoveStyle_ApplyToStat(struct Pokemon *mon, s32 value, u8 statIndex)
{
    s32 lean;

    if (statIndex != STAT_ATK && statIndex != STAT_SPATK)
        return value;

    lean = MoveStyle_GetLean(mon);
    if (lean == 0)
        return value;

    // Physical lean raises Attack and lowers Sp. Atk; special lean does the
    // reverse. Same magnitude both ways, so the pair stays balanced.
    if (statIndex == STAT_SPATK)
        lean = -lean;

    value = value + (value * lean * MOVE_STYLE_MAX_BIAS) / 10000;
    if (value < 1)
        value = 1;
    return value;
}

#include "global.h"
#include "difficulty.h"
#include "constants/difficulty.h"
#include "constants/battle_ai.h"
#include "constants/pokemon.h"

// Difficulty chosen at new game. It only ever affects the trainers the player
// fights -- their AI sharpness, their levels and how well built their Pokemon
// are. Wild encounters and the player's own party are untouched.

u8 Difficulty_Get(void)
{
    u8 d = gSaveBlock2Ptr->difficulty;

    return (d < DIFFICULTY_COUNT) ? d : DIFFICULTY_NORMAL;
}

// Levels added to every trainer Pokemon.
s8 Difficulty_LevelOffset(void)
{
    switch (Difficulty_Get())
    {
    case DIFFICULTY_EASY:     return DIFFICULTY_LEVEL_OFFSET_EASY;
    case DIFFICULTY_HARD:     return DIFFICULTY_LEVEL_OFFSET_HARD;
    case DIFFICULTY_BRUTAL:   return DIFFICULTY_LEVEL_OFFSET_BRUTAL;
    case DIFFICULTY_CHAMPION: return DIFFICULTY_LEVEL_OFFSET_CHAMPION;
    default:                  return DIFFICULTY_LEVEL_OFFSET_NORMAL;
    }
}

u8 Difficulty_ScaleTrainerLevel(u8 level)
{
    s16 scaled = (s16)level + Difficulty_LevelOffset();

    if (scaled < 2)
        scaled = 2;
    if (scaled > MAX_LEVEL)
        scaled = MAX_LEVEL;
    return (u8)scaled;
}

// Fixed IV given to trainer Pokemon (the engine's own 0..255 "fixedIV" scale).
// Higher difficulties field better-built Pokemon; the top tier is fully maxed.
u8 Difficulty_TrainerFixedIV(u8 defaultIV)
{
    switch (Difficulty_Get())
    {
    case DIFFICULTY_EASY:     return 0;
    case DIFFICULTY_HARD:     return (defaultIV < 150) ? 150 : defaultIV;
    case DIFFICULTY_BRUTAL:   return (defaultIV < 200) ? 200 : defaultIV;
    case DIFFICULTY_CHAMPION: return MAX_PER_STAT_IVS;
    default:                  return defaultIV;
    }
}

// AI flags every trainer gets. The top tier turns on the engine's whole AI
// toolkit at once: it avoids bad moves, weighs viability, goes for the KO, sets
// up on turn one, plays risky lines, prefers its strongest option and tracks HP.
//
// Honest note: this is the GBA's own flag-driven AI turned to maximum, not a
// replay of real championship matches -- no such match data exists in the ROM.
u32 Difficulty_ApplyAiFlags(u32 flags)
{
    switch (Difficulty_Get())
    {
    case DIFFICULTY_EASY:
        return AI_SCRIPT_CHECK_BAD_MOVE;
    case DIFFICULTY_HARD:
        return flags | AI_SCRIPT_CHECK_BAD_MOVE | AI_SCRIPT_CHECK_VIABILITY | AI_SCRIPT_TRY_TO_FAINT;
    case DIFFICULTY_BRUTAL:
        return flags | AI_SCRIPT_CHECK_BAD_MOVE | AI_SCRIPT_CHECK_VIABILITY | AI_SCRIPT_TRY_TO_FAINT
                     | AI_SCRIPT_PREFER_STRONGEST_MOVE | AI_SCRIPT_HP_AWARE;
    case DIFFICULTY_CHAMPION:
        return flags | AI_SCRIPT_CHECK_BAD_MOVE | AI_SCRIPT_CHECK_VIABILITY | AI_SCRIPT_TRY_TO_FAINT
                     | AI_SCRIPT_PREFER_STRONGEST_MOVE | AI_SCRIPT_HP_AWARE
                     | AI_SCRIPT_SETUP_FIRST_TURN | AI_SCRIPT_RISKY;
    default:
        return flags;
    }
}

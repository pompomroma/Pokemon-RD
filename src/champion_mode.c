#include "global.h"
#include "event_data.h"
#include "champion_mode.h"
#include "constants/difficulty.h"
#include "constants/flags.h"

// See include/champion_mode.h. Names are kept here rather than in oak_speech.c
// so both the new-game intro and the OPTION menu can print the same strings.

static const u8 sText_Difficulty_Easy[]     = _("EASY");
static const u8 sText_Difficulty_Normal[]   = _("NORMAL");
static const u8 sText_Difficulty_Hard[]     = _("HARD");
static const u8 sText_Difficulty_Brutal[]   = _("BRUTAL");
static const u8 sText_Difficulty_Champion[] = _("AW SHIBAL GAY BBAKSAY");

static const u8 *const sDifficultyNames[DIFFICULTY_COUNT] =
{
    [DIFFICULTY_EASY]     = sText_Difficulty_Easy,
    [DIFFICULTY_NORMAL]   = sText_Difficulty_Normal,
    [DIFFICULTY_HARD]     = sText_Difficulty_Hard,
    [DIFFICULTY_BRUTAL]   = sText_Difficulty_Brutal,
    [DIFFICULTY_CHAMPION] = sText_Difficulty_Champion,
};

const u8 *ChampionMode_GetDifficultyName(u8 difficulty)
{
    if (difficulty >= DIFFICULTY_COUNT)
        difficulty = DIFFICULTY_NORMAL;
    return sDifficultyNames[difficulty];
}

bool8 ChampionMode_IsActive(void)
{
    return gSaveBlock2Ptr->difficulty == DIFFICULTY_CHAMPION;
}

bool8 ChampionMode_CanSwitch(void)
{
    return FlagGet(FLAG_SYS_GAME_CLEAR);
}

bool8 Bbaksayon_IsUnlocked(void)
{
    return ChampionMode_CanSwitch() && ChampionMode_IsActive();
}

void ChampionMode_SetDifficulty(u8 difficulty)
{
    if (difficulty < DIFFICULTY_COUNT)
        gSaveBlock2Ptr->difficulty = difficulty;
}

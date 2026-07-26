#ifndef GUARD_DIFFICULTY_H
#define GUARD_DIFFICULTY_H

#include "global.h"

// Difficulty picked at new game (after NORMAL/RANDOM). Scales only the trainers
// the player fights: AI sharpness, levels, and how well built their Pokemon are.
u8 Difficulty_Get(void);
s8 Difficulty_LevelOffset(void);
u8 Difficulty_ScaleTrainerLevel(u8 level);
u8 Difficulty_TrainerFixedIV(u8 defaultIV);
u32 Difficulty_ApplyAiFlags(u32 flags);

#endif // GUARD_DIFFICULTY_H

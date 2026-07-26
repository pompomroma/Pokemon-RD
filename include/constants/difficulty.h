#ifndef GUARD_CONSTANTS_DIFFICULTY_H
#define GUARD_CONSTANTS_DIFFICULTY_H

// Difficulty is chosen at new game, right after NORMAL/RANDOM, and stored in
// gSaveBlock2Ptr->difficulty. It scales the trainers the player fights: their
// AI sharpness, their levels, and how well built their Pokemon are.
#define DIFFICULTY_EASY     0
#define DIFFICULTY_NORMAL   1
#define DIFFICULTY_HARD     2
#define DIFFICULTY_BRUTAL   3
#define DIFFICULTY_CHAMPION 4  // "AW SHIBAL GAY BBAKSAY"
#define DIFFICULTY_COUNT    5

// Level added to (or removed from) every trainer Pokemon, per difficulty.
#define DIFFICULTY_LEVEL_OFFSET_EASY      -3
#define DIFFICULTY_LEVEL_OFFSET_NORMAL     0
#define DIFFICULTY_LEVEL_OFFSET_HARD       3
#define DIFFICULTY_LEVEL_OFFSET_BRUTAL     6
#define DIFFICULTY_LEVEL_OFFSET_CHAMPION  10

#endif // GUARD_CONSTANTS_DIFFICULTY_H

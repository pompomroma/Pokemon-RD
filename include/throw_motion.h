#ifndef GUARD_THROW_MOTION_H
#define GUARD_THROW_MOTION_H

#include "global.h"
#include "sprite.h"

// A proper throwing motion for trainer sprites on send-out.
//
// Vanilla just flips to the trainer's throw frame and slides the sprite off,
// so the throw reads as a single pose. This adds the three beats a real throw
// has -- a wind-up away from the target, a fast snap toward it, and a
// follow-through that settles back -- by offsetting the sprite each frame.
// It is driven by maths rather than by artwork, so every trainer gets it
// regardless of how many frames their pic has.
//
// Runs before the slide-off: the sprite's own callback is stashed and restored
// once the motion finishes, so the existing send-out sequence is untouched.

#define THROW_MOTION_FRAMES 26

// dir is +1 for a sprite that throws to the right (the player's back pic) and
// -1 for one that throws to the left (an opponent's front pic).
void ThrowMotion_Start(struct Sprite *sprite, s8 dir, void (*after)(struct Sprite *));

#endif // GUARD_THROW_MOTION_H

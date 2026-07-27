#include "global.h"
#include "gflib.h"
#include "sprite.h"
#include "throw_motion.h"
#include "trig.h"

// See include/throw_motion.h.
//
// The follow-up callback is stashed in a small side table rather than in the
// sprite's data[] slots: the send-out code already owns data[0..7] (the linear
// translation reads data[0..4], the battler id lives in data[5] and the
// stored end-callback in data[6..7]), and clobbering any of them breaks the
// slide that runs straight after this motion.

#define MOTION_SLOTS 4

struct ThrowMotionSlot
{
    void (*after)(struct Sprite *);
    u8 spriteId;
    s8 dir;
    u8 timer;
    bool8 active;
};

static EWRAM_DATA struct ThrowMotionSlot sSlots[MOTION_SLOTS] = {0};

// Beat boundaries inside THROW_MOTION_FRAMES.
#define WINDUP_END 13   // pulling back away from the target
#define SNAP_END   19   // driving forward through the release
// ...through THROW_MOTION_FRAMES: follow-through settling back to rest

// How far the sprite travels on each beat, in pixels.
#define WINDUP_BACK   5
#define WINDUP_CROUCH 2
#define SNAP_FORWARD  7
#define SNAP_RISE     3

static struct ThrowMotionSlot *FindSlot(u8 spriteId)
{
    s32 i;

    for (i = 0; i < MOTION_SLOTS; i++)
    {
        if (sSlots[i].active && sSlots[i].spriteId == spriteId)
            return &sSlots[i];
    }
    return NULL;
}

static void SpriteCB_ThrowMotion(struct Sprite *sprite)
{
    struct ThrowMotionSlot *slot = FindSlot(sprite - gSprites);
    void (*after)(struct Sprite *);
    s32 t, num, den;

    if (slot == NULL) // slot was recycled; bail out rather than stall the sprite
    {
        sprite->x2 = 0;
        sprite->y2 = 0;
        sprite->callback = SpriteCallbackDummy;
        return;
    }

    t = slot->timer++;

    if (t <= WINDUP_END)
    {
        // Ease out as the wind-up reaches its furthest point, so the pull-back
        // decelerates into the turnaround instead of stopping dead.
        num = Sin((t * 64) / WINDUP_END, 256);   // 0 -> 256 over the beat
        den = 256;
        sprite->x2 = -slot->dir * (WINDUP_BACK * num) / den;
        sprite->y2 = (WINDUP_CROUCH * num) / den;
    }
    else if (t <= SNAP_END)
    {
        // The snap is deliberately linear and short -- that is what sells it as
        // fast next to the eased beats on either side.
        num = t - WINDUP_END;
        den = SNAP_END - WINDUP_END;
        sprite->x2 = slot->dir * (-WINDUP_BACK * (den - num) + SNAP_FORWARD * num) / den;
        sprite->y2 = (WINDUP_CROUCH * (den - num) - SNAP_RISE * num) / den;
    }
    else if (t < THROW_MOTION_FRAMES)
    {
        // Follow-through: drift back to rest, easing in.
        num = THROW_MOTION_FRAMES - t;
        den = THROW_MOTION_FRAMES - SNAP_END;
        sprite->x2 = slot->dir * (SNAP_FORWARD * num) / den;
        sprite->y2 = (-SNAP_RISE * num) / den;
    }
    else
    {
        after = slot->after;
        sprite->x2 = 0;
        sprite->y2 = 0;
        slot->active = FALSE;
        if (after != NULL)
            after(sprite);
        else
            sprite->callback = SpriteCallbackDummy;
    }
}

void ThrowMotion_Start(struct Sprite *sprite, s8 dir, void (*after)(struct Sprite *))
{
    u8 spriteId = sprite - gSprites;
    struct ThrowMotionSlot *slot = FindSlot(spriteId);
    s32 i;

    if (slot == NULL)
    {
        for (i = 0; i < MOTION_SLOTS; i++)
        {
            if (!sSlots[i].active)
            {
                slot = &sSlots[i];
                break;
            }
        }
    }
    // Every slot busy: skip the flourish and run the original sequence, so a
    // crowded scene degrades to vanilla rather than losing the send-out.
    if (slot == NULL)
    {
        if (after != NULL)
            after(sprite);
        return;
    }

    slot->after = after;
    slot->spriteId = spriteId;
    slot->dir = (dir < 0) ? -1 : 1;
    slot->timer = 0;
    slot->active = TRUE;
    sprite->callback = SpriteCB_ThrowMotion;
}

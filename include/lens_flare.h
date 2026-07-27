#ifndef GUARD_LENS_FLARE_H
#define GUARD_LENS_FLARE_H

#include "global.h"

// The soft light circles a real lens throws when strong light hits it: a row of
// discs and a ring strung along the axis from the light source through the
// centre of frame, drifting and breathing slightly.
//
// They are ordinary opaque sprites whose translucency is faked with an ordered
// dither (see graphics/misc/lens_flare.png). That matters: both the battle
// scene and the overworld already own BLDCNT/BLDALPHA for the existing grade,
// depth-of-field and reflection work, so a blend-based flare would fight them.
// Dithering costs nothing but a sprite slot.

void LensFlare_Create(void);   // spawn the flare for the current scene
void LensFlare_Destroy(void);  // remove it (scene teardown)

#endif // GUARD_LENS_FLARE_H

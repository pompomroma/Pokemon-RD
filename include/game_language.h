#ifndef GUARD_GAME_LANGUAGE_H
#define GUARD_GAME_LANGUAGE_H

#include "global.h"

// In-game UI language toggle (Options menu). A curated set of strings is
// localized to Korean, rendered with real Hangul glyphs baked into the free
// slots of the normal font. Untranslated strings stay English.
#define GAME_LANG_ENGLISH 0
#define GAME_LANG_KOREAN   1

// Returns ko when the player selected Korean and ko is non-NULL, else en.
const u8 *GetLangString(const u8 *en, const u8 *ko);

#endif // GUARD_GAME_LANGUAGE_H

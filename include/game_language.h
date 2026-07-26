#ifndef GUARD_GAME_LANGUAGE_H
#define GUARD_GAME_LANGUAGE_H

#include "global.h"

// In-game UI language toggle (Options menu). A curated set of strings is
// localized to Korean, rendered with real Hangul glyphs baked into the free
// slots of the normal font. Untranslated strings stay English.
#define GAME_LANG_ENGLISH  0
#define GAME_LANG_KOREAN   1
#define GAME_LANG_JAPANESE 2
#define GAME_LANG_CHINESE  3
#define GAME_LANG_COUNT    4
#define GAME_LANG_NONE     0xFF // "no boot choice made" sentinel, never stored

// Records the boot screen's language pick, and re-applies it over a save that
// was loaded (or defaulted) afterwards. The title screen reloads SaveBlock2
// from flash after the boot screen runs, so without the re-apply the pick is
// discarded and the language option appears to do nothing.
void GameLanguage_SetBootChoice(u8 lang);
void GameLanguage_ReapplyBootChoice(void);

// Returns ko when the player selected Korean and ko is non-NULL, else en.
// (Japanese/Chinese have the boot selection screen and native-script labels,
// but no full string translation, so they read as English in game text.)
const u8 *GetLangString(const u8 *en, const u8 *ko);

// Translates a piece of baked dialogue (NPC msgbox text, Oak's opening speech)
// for the selected language. Returns the string unchanged when the language is
// English or the line has no translation, so it is safe to wrap any text
// pointer on its way to the screen.
const u8 *GameText_Localize(const u8 *str);

#endif // GUARD_GAME_LANGUAGE_H

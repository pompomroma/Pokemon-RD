#ifndef GUARD_DEOXYS_FORMS_H
#define GUARD_DEOXYS_FORMS_H

#include "global.h"
#include "pokemon.h"

// All four Gen 3 Deoxys formes, selectable individually.
//
// Gen 3 ties the forme to the game version (FireRed = Attack, LeafGreen =
// Defense, Ruby/Sapphire = Normal, Emerald = Speed), so vanilla only ever needs
// one. Here the player picks a forme per mon, which means the forme has to be
// stored per mon (gSaveBlock2Ptr->deoxysForms) and applied in two places: the
// base stats and the sprite.
//
// Art: Normal and Attack are frames 0 and 1 of deoxys/front.4bpp, Defense is
// frame 1 of deoxys/front_def.4bpp (compiled in from the LeafGreen art). The
// ROM has no Speed forme art at all, so Speed is a derived variant -- the
// Normal silhouette with its body colours swapped to the palette's greens.

#define DEOXYS_FORM_NORMAL  0
#define DEOXYS_FORM_ATTACK  1
#define DEOXYS_FORM_DEFENSE 2
#define DEOXYS_FORM_SPEED   3
#define DEOXYS_FORM_COUNT   4

#define DEOXYS_FORM_NONE    0xFF

u8 Deoxys_GetFormByPersonality(u32 personality);
u8 Deoxys_GetBoxMonForm(struct BoxPokemon *boxMon);
u8 Deoxys_GetMonForm(struct Pokemon *mon);

// Stores the forme and recalculates the mon's stats. FALSE if the table is full.
bool8 Deoxys_SetMonForm(struct Pokemon *mon, u8 form);

// Canonical Gen 3 base stats for a forme, in STAT_HP..STAT_SPDEF order.
u16 Deoxys_GetFormBaseStat(u8 form, u8 statIndex);

// Display name for menus ("DEOXYS-A" etc.), since gSpeciesNames has one entry.
const u8 *Deoxys_GetFormName(u8 form);

// Rewrites a freshly decompressed Deoxys pic in place to the mon's forme.
// Replaces vanilla's DuplicateDeoxysTiles.
void Deoxys_ApplyFormToPic(void *dest, s32 species, u32 personality, bool8 isFrontPic);

// Forces the next pic decompress to a given forme regardless of any record, so
// menus can preview a forme the player does not own yet. DEOXYS_FORM_NONE off.
void Deoxys_SetPreviewForm(u8 form);

// Byte offset of the forme's 0x400 icon inside gMonIcon_Deoxys.
u32 Deoxys_GetIconOffset(u8 form);

#endif // GUARD_DEOXYS_FORMS_H

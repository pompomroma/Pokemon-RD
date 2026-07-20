#ifndef GUARD_POKEMON_FUSION_H
#define GUARD_POKEMON_FUSION_H

#include "global.h"
#include "pokemon.h"

// The FUSION STONE merges two party Pokémon into a single, stronger one.
// Fusions are fully dynamic: any species pair produces generated stats,
// typing, nickname, composite sprite, and a per-pair signature move, so
// every possible combination is covered without per-pair data.

#define FUSION_FLAG_ACTIVE 0x1

// Record lookup
struct FusionRecord *Fusion_FindRecordByMon(struct BoxPokemon *boxMon);
struct FusionRecord *Fusion_FindRecordByPersonality(u32 personality);
bool8 Fusion_IsMonFused(struct BoxPokemon *boxMon);
u16 Fusion_GetPartnerSpecies(struct BoxPokemon *boxMon);

// Fusing / unfusing party members
bool8 Fusion_HasFreeRecord(void);
bool8 Fusion_FuseParty(u8 slotA, u8 slotB);
bool8 Fusion_UnfuseParty(u8 slot);

// Generated attributes
u16 Fusion_FuseBaseStatPair(u16 statA, u16 statB);
u8 Fusion_GetBaseStat(u16 species, u16 partnerSpecies, u8 statIndex);
void Fusion_GetTypes(u16 species, u16 partnerSpecies, u8 *type1, u8 *type2);
void Fusion_BuildFusedName(u16 speciesA, u16 speciesB, u8 *dest);
u8 Fusion_GetSignatureMoveType(u16 species, u16 partnerSpecies);
u8 Fusion_GetSignatureMovePower(u16 species, u16 partnerSpecies);
void Fusion_BuildSignatureMoveName(u16 species, u16 partnerSpecies, u8 *dest);

// Display/engine hooks
bool8 Fusion_GetMoveNameForMon(struct BoxPokemon *boxMon, u16 move, u8 *dest);
bool8 Fusion_GetMoveNameForBattler(u8 battler, u16 move, u8 *dest);
struct Pokemon *Fusion_GetBattlerPartyMon(u8 battler);
void Fusion_ApplyBattleTypes(u8 battler);
void Fusion_SetSignatureMoveDynamics(u8 battler);
void Fusion_SpliceMonPic(void *dest, u32 personality, bool8 isFrontPic);
void Fusion_BlendMonPalBuffer(u32 personality, u32 otId, u16 *palBuffer);
void Fusion_BlendMonSpritePalette(u32 personality, u32 otId, u32 palOffset);

#endif // GUARD_POKEMON_FUSION_H

#ifndef GUARD_POKEMON_FUSION_H
#define GUARD_POKEMON_FUSION_H

#include "global.h"
#include "pokemon.h"

// The FUSION STONE merges two party Pokémon into a single, stronger one.
// Fusions are fully dynamic: any species pair produces generated stats,
// typing, nickname, composite sprite, and a per-pair signature move, so
// every possible combination is covered without per-pair data.

#define FUSION_FLAG_ACTIVE 0x1
// Stored "announced" stage occupies flags bits 1-2 (0..2).
#define FUSION_STAGE_SHIFT 1
#define FUSION_STAGE_MASK  (3 << FUSION_STAGE_SHIFT)
#define FUSION_MAX_STAGE   2

// A fused Pokémon evolves through 3 power stages as it levels:
//   stage 0 (Lv 1-15) -> stage 1 (Lv 16-35) -> stage 2 (Lv 36+).
// Each stage multiplies the fused base stats and its signature move.
#define FUSION_STAGE1_LEVEL 16
#define FUSION_STAGE2_LEVEL 36

// Record lookup
struct FusionRecord *Fusion_FindRecordByMon(struct BoxPokemon *boxMon);
struct FusionRecord *Fusion_FindRecordByPersonality(u32 personality);
bool8 Fusion_IsMonFused(struct BoxPokemon *boxMon);
u16 Fusion_GetPartnerSpecies(struct BoxPokemon *boxMon);

// Fusing / unfusing party members
bool8 Fusion_HasFreeRecord(void);
bool8 Fusion_FuseParty(u8 slotA, u8 slotB);
bool8 Fusion_UnfuseParty(u8 slot);

// Fusion evolution stages
u8 Fusion_GetLevelStage(u8 level);
u8 Fusion_GetRecordStage(struct FusionRecord *rec);
u8 Fusion_GetMonEffectiveStage(struct BoxPokemon *boxMon);
u16 Fusion_ApplyStageToStat(u16 base, u8 stage);
bool8 Fusion_TryStageUp(struct Pokemon *mon); // returns TRUE if a stage was announced

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
bool8 Fusion_BattlerHasStoredItem(u8 battler, u16 item);
void Fusion_ApplyBattleTypes(u8 battler);
void Fusion_SetSignatureMoveDynamics(u8 battler);
void Fusion_SpliceMonPic(void *dest, u32 personality, bool8 isFrontPic);
void Fusion_BlendMonPalBuffer(u32 personality, u32 otId, u16 *palBuffer);
void Fusion_BlendMonSpritePalette(u32 personality, u32 otId, u32 palOffset);

#endif // GUARD_POKEMON_FUSION_H

#include "global.h"
#include "coop_group.h"
#include "event_object_movement.h"
#include "script_movement.h"
#include "script.h"
#include "pokemon.h"
#include "battle.h"
#include "random.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/battle.h"
#include "constants/pokemon.h"
#include "constants/species.h"

// Offline "party group": up to COOP_MAX_MEMBERS companion object events trail the
// player single-file along the exact path the player walked. Driven each field
// frame from CoopGroup_Update(). Movement is issued through the same script-
// movement backend the `applymovement` command uses, so it works on the spawned
// objects regardless of their (NONE) movement type.

#define COOP_HIST_LEN     32   // trail ring buffer (>= max lag + margin)
#define COOP_LOCALID_BASE 124  // distinct local ids (below LOCALID_CAMERA 127)
#define COOP_DIR_NONE     0

struct CoopTile
{
    s16 x;
    s16 y;
    u8 elev;
};

static EWRAM_DATA struct CoopTile sHist[COOP_HIST_LEN] = {0};
static EWRAM_DATA u32 sTileSeq = 0;                       // player tiles entered
static EWRAM_DATA u32 sMemberSeq[COOP_MAX_MEMBERS] = {0}; // each companion's trail position
static EWRAM_DATA u8 sMemberObjId[COOP_MAX_MEMBERS] = {0};
static EWRAM_DATA u8 sMoveScript[COOP_MAX_MEMBERS][2] = {0};
static EWRAM_DATA bool8 sSpawned = FALSE;
static EWRAM_DATA s16 sLastX = 0;
static EWRAM_DATA s16 sLastY = 0;
static EWRAM_DATA u8 sLastMapNum = 0xFF;
static EWRAM_DATA u8 sLastMapGroup = 0xFF;

static const u8 sMemberGfxDefault[COOP_MAX_MEMBERS] =
{
    OBJ_EVENT_GFX_BOY,
    OBJ_EVENT_GFX_LASS,
    OBJ_EVENT_GFX_YOUNGSTER,
};

u16 CoopGroup_GetOwnCode(void)
{
    if (gSaveBlock2Ptr->coopCode == 0)
    {
        u16 tid = (gSaveBlock2Ptr->playerTrainerId[1] << 8) | gSaveBlock2Ptr->playerTrainerId[0];
        u16 code = tid % 10000;
        if (code == 0)
            code = 1234;
        gSaveBlock2Ptr->coopCode = code;
    }
    return gSaveBlock2Ptr->coopCode;
}

bool8 CoopGroup_IsActive(void)
{
    return gSaveBlock2Ptr->coopMemberCount > 0;
}

u16 CoopGroup_GetJoinedCode(void)
{
    return gSaveBlock2Ptr->coopJoinedCode;
}

void CoopGroup_JoinByCode(u16 code)
{
    u8 i;

    gSaveBlock2Ptr->coopJoinedCode = code;
    if (code == 0)
    {
        gSaveBlock2Ptr->coopMemberCount = 0;
    }
    else
    {
        gSaveBlock2Ptr->coopMemberCount = COOP_MAX_MEMBERS;
        for (i = 0; i < COOP_MAX_MEMBERS; i++)
            gSaveBlock2Ptr->coopMemberGfx[i] = sMemberGfxDefault[i];
    }
    // Force a respawn/despawn on the next field update.
    sSpawned = FALSE;
}

static u8 CoopDirFromDelta(s16 dx, s16 dy)
{
    if (dx > 0)
        return DIR_EAST;
    if (dx < 0)
        return DIR_WEST;
    if (dy > 0)
        return DIR_SOUTH;
    if (dy < 0)
        return DIR_NORTH;
    return COOP_DIR_NONE;
}

static void CoopDespawn(void)
{
    u8 i;

    for (i = 0; i < COOP_MAX_MEMBERS; i++)
        RemoveObjectEventByLocalIdAndMap(COOP_LOCALID_BASE + i,
            gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup);
    sSpawned = FALSE;
}

static void CoopSpawn(struct ObjectEvent *player)
{
    u8 i;
    u8 count = gSaveBlock2Ptr->coopMemberCount;

    if (count > COOP_MAX_MEMBERS)
        count = COOP_MAX_MEMBERS;

    // Seed the trail with the player's current tile.
    sTileSeq = 1;
    sHist[0].x = player->currentCoords.x;
    sHist[0].y = player->currentCoords.y;
    sHist[0].elev = player->currentElevation;
    sLastX = player->currentCoords.x;
    sLastY = player->currentCoords.y;

    for (i = 0; i < count; i++)
    {
        sMemberObjId[i] = SpawnSpecialObjectEventParameterized(
            gSaveBlock2Ptr->coopMemberGfx[i], MOVEMENT_TYPE_NONE, COOP_LOCALID_BASE + i,
            player->currentCoords.x, player->currentCoords.y, player->currentElevation);
        sMemberSeq[i] = 0; // spawned on the player's tile (seq 0)
    }
    sSpawned = TRUE;
}

void CoopGroup_TryMakeWildBattleDouble(void)
{
    u8 i, able = 0;
    u16 species;
    u8 level;

    if (!CoopGroup_IsActive())
        return;

    // Need at least two able (non-egg, HP > 0) party Pokemon to field a double.
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) != SPECIES_NONE
         && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG, NULL)
         && GetMonData(&gPlayerParty[i], MON_DATA_HP, NULL) > 0)
            able++;
    }
    if (able < 2)
        return;

    species = GetMonData(&gEnemyParty[0], MON_DATA_SPECIES, NULL);
    level = GetMonData(&gEnemyParty[0], MON_DATA_LEVEL, NULL);
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return;

    // Add a second wild ally so the co-op group fights side-by-side.
    CreateMon(&gEnemyParty[1], species, level, USE_RANDOM_IVS, FALSE, 0, FALSE, 0);
    gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
}

void CoopGroup_Update(void)
{
    struct ObjectEvent *player;
    u8 i, count;

    if (!CoopGroup_IsActive())
    {
        if (sSpawned)
            CoopDespawn();
        return;
    }

    // Only run in the normal walking field; pause during scripts/cutscenes,
    // surfing, biking, etc. so we never fight the engine.
    if (ArePlayerFieldControlsLocked())
        return;
    if (!(gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_ON_FOOT))
        return;

    player = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!player->active)
        return;

    // Reset the group when the map changes (companions were wiped on load).
    if (sLastMapNum != gSaveBlock1Ptr->location.mapNum
     || sLastMapGroup != gSaveBlock1Ptr->location.mapGroup)
    {
        sLastMapNum = gSaveBlock1Ptr->location.mapNum;
        sLastMapGroup = gSaveBlock1Ptr->location.mapGroup;
        sSpawned = FALSE;
    }

    if (!sSpawned)
    {
        CoopSpawn(player);
        return;
    }

    count = gSaveBlock2Ptr->coopMemberCount;
    if (count > COOP_MAX_MEMBERS)
        count = COOP_MAX_MEMBERS;

    // Record a new trail tile whenever the player finishes a step.
    if (player->currentCoords.x != sLastX || player->currentCoords.y != sLastY)
    {
        sHist[sTileSeq % COOP_HIST_LEN].x = player->currentCoords.x;
        sHist[sTileSeq % COOP_HIST_LEN].y = player->currentCoords.y;
        sHist[sTileSeq % COOP_HIST_LEN].elev = player->currentElevation;
        sTileSeq++;
        sLastX = player->currentCoords.x;
        sLastY = player->currentCoords.y;
    }

    // Step each companion one tile along the recorded trail toward its slot.
    for (i = 0; i < count; i++)
    {
        struct ObjectEvent *m;
        u32 desiredSeq, nextSeq;
        struct CoopTile *target;
        u8 dir;

        if (sMemberObjId[i] >= OBJECT_EVENTS_COUNT)
            continue;
        m = &gObjectEvents[sMemberObjId[i]];
        if (!m->active)
        {
            // Slot lost (e.g. mid-transition) -> respawn everyone next frame.
            sSpawned = FALSE;
            return;
        }

        // Companion i should trail (i + 1) tiles behind the player's current tile.
        if (sTileSeq < (u32)(i + 2))
            continue; // player hasn't walked far enough yet
        desiredSeq = sTileSeq - 1 - (i + 1);
        if (sMemberSeq[i] >= desiredSeq)
            continue; // already in position

        if (!ScriptMovement_IsObjectMovementFinished(COOP_LOCALID_BASE + i,
                gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup))
            continue; // still mid-step

        nextSeq = sMemberSeq[i] + 1;
        target = &sHist[nextSeq % COOP_HIST_LEN];
        dir = CoopDirFromDelta(target->x - m->currentCoords.x, target->y - m->currentCoords.y);
        if (dir == COOP_DIR_NONE)
        {
            sMemberSeq[i] = nextSeq;
            continue;
        }
        sMoveScript[i][0] = GetWalkNormalMovementAction(dir);
        sMoveScript[i][1] = MOVEMENT_ACTION_STEP_END;
        ScriptMovement_StartObjectMovementScript(COOP_LOCALID_BASE + i,
            gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup, sMoveScript[i]);
        sMemberSeq[i] = nextSeq;
    }
}

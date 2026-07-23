#ifndef GUARD_COOP_GROUP_H
#define GUARD_COOP_GROUP_H

// Local co-op "party group" (single-player simulation).
// A 4-digit code entered in the Options menu forms a group of up to 3 companion
// avatars that trail the player single-file around the overworld. This is the
// offline, always-available half of the co-op feature; real human co-op uses the
// engine's existing link / Union-Room path over an emulator's netplay.

#define COOP_MAX_MEMBERS 3

// This save's own 4-digit code (derived once from the trainer id if unset).
u16 CoopGroup_GetOwnCode(void);

// TRUE when a group is active (at least one companion).
bool8 CoopGroup_IsActive(void);

// Enter a code: non-zero forms the group (companions spawn on the next field
// update); 0 disbands it.
void CoopGroup_JoinByCode(u16 code);

// The code the player last entered (for the Options display / editor).
u16 CoopGroup_GetJoinedCode(void);

// Per-frame overworld driver: records the player's trail and steps the
// companions along it. Safe to call every field frame (self-gates).
void CoopGroup_Update(void);

#endif // GUARD_COOP_GROUP_H

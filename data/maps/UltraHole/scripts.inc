UltraHole_MapScripts::
	.byte 0

@ Ultra Beast stand-ins. Defeat them (any way) to draw them into your team at
@ 100% with no Poke Ball, holding a Z Crystal. They respawn on re-entry, so the
@ hole is a farmable dimensional dungeon.

UltraHole_EventScript_Articuno::
	setvar VAR_0x8008, SPECIES_ARTICUNO
	call UltraHole_EventScript_Challenge
	setwildbattle SPECIES_ARTICUNO, 50
	special ScaleWildMonToPlayerLevel
	call UltraHole_EventScript_RunBattle
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, UltraHole_EventScript_CaughtArticuno
	release
	end

UltraHole_EventScript_CaughtArticuno::
	setvar VAR_0x8008, SPECIES_ARTICUNO
	setvar VAR_0x8009, ITEM_Z_CRYSTAL
	special GiveDungeonMonAtPlayerLevel
	goto UltraHole_EventScript_Obtained
	end

UltraHole_EventScript_Zapdos::
	setvar VAR_0x8008, SPECIES_ZAPDOS
	call UltraHole_EventScript_Challenge
	setwildbattle SPECIES_ZAPDOS, 50
	special ScaleWildMonToPlayerLevel
	call UltraHole_EventScript_RunBattle
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, UltraHole_EventScript_CaughtZapdos
	release
	end

UltraHole_EventScript_CaughtZapdos::
	setvar VAR_0x8008, SPECIES_ZAPDOS
	setvar VAR_0x8009, ITEM_Z_CRYSTAL
	special GiveDungeonMonAtPlayerLevel
	goto UltraHole_EventScript_Obtained
	end

UltraHole_EventScript_Moltres::
	setvar VAR_0x8008, SPECIES_MOLTRES
	call UltraHole_EventScript_Challenge
	setwildbattle SPECIES_MOLTRES, 50
	special ScaleWildMonToPlayerLevel
	call UltraHole_EventScript_RunBattle
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, UltraHole_EventScript_CaughtMoltres
	release
	end

UltraHole_EventScript_CaughtMoltres::
	setvar VAR_0x8008, SPECIES_MOLTRES
	setvar VAR_0x8009, ITEM_Z_CRYSTAL
	special GiveDungeonMonAtPlayerLevel
	goto UltraHole_EventScript_Obtained
	end

UltraHole_EventScript_Challenge::
	lock
	faceplayer
	waitse
	playmoncry VAR_0x8008, CRY_MODE_ENCOUNTER
	message UltraHole_Text_UltraBeast
	waitmessage
	waitmoncry
	delay 20
	waitbuttonpress
	return

UltraHole_EventScript_RunBattle::
	setflag FLAG_SYS_SPECIAL_WILD_BATTLE
	special StartLegendaryBattle
	waitstate
	clearflag FLAG_SYS_SPECIAL_WILD_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	return

UltraHole_EventScript_Obtained::
	bufferspeciesname STR_VAR_1, VAR_0x8008
	fadescreen FADE_TO_BLACK
	removeobject VAR_LAST_TALKED
	fadescreen FADE_FROM_BLACK
	msgbox UltraHole_Text_Obtained, MSGBOX_DEFAULT
	release
	end

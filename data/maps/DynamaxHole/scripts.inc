DynamaxHole_MapScripts::
	.byte 0

@ Dynamaxed bosses. Defeat them (any way) to obtain them at 100% with no Poke
@ Ball, holding a Dyna Band so they Dynamax in your own battles. They respawn on
@ re-entry, so the hole is a farmable raid-style dungeon.

DynamaxHole_EventScript_Lugia::
	setvar VAR_0x8008, SPECIES_LUGIA
	call DynamaxHole_EventScript_Challenge
	setwildbattle SPECIES_LUGIA, 60
	special ScaleWildMonToPlayerLevel
	call DynamaxHole_EventScript_RunBattle
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, DynamaxHole_EventScript_CaughtLugia
	release
	end

DynamaxHole_EventScript_CaughtLugia::
	setvar VAR_0x8008, SPECIES_LUGIA
	setvar VAR_0x8009, ITEM_DYNA_BAND
	special GiveDungeonMonAtPlayerLevel
	goto DynamaxHole_EventScript_Obtained
	end

DynamaxHole_EventScript_HoOh::
	setvar VAR_0x8008, SPECIES_HO_OH
	call DynamaxHole_EventScript_Challenge
	setwildbattle SPECIES_HO_OH, 60
	special ScaleWildMonToPlayerLevel
	call DynamaxHole_EventScript_RunBattle
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, DynamaxHole_EventScript_CaughtHoOh
	release
	end

DynamaxHole_EventScript_CaughtHoOh::
	setvar VAR_0x8008, SPECIES_HO_OH
	setvar VAR_0x8009, ITEM_DYNA_BAND
	special GiveDungeonMonAtPlayerLevel
	goto DynamaxHole_EventScript_Obtained
	end

DynamaxHole_EventScript_Suicune::
	setvar VAR_0x8008, SPECIES_SUICUNE
	call DynamaxHole_EventScript_Challenge
	setwildbattle SPECIES_SUICUNE, 60
	special ScaleWildMonToPlayerLevel
	call DynamaxHole_EventScript_RunBattle
	goto_if_eq VAR_RESULT, B_OUTCOME_WON, DynamaxHole_EventScript_CaughtSuicune
	release
	end

DynamaxHole_EventScript_CaughtSuicune::
	setvar VAR_0x8008, SPECIES_SUICUNE
	setvar VAR_0x8009, ITEM_DYNA_BAND
	special GiveDungeonMonAtPlayerLevel
	goto DynamaxHole_EventScript_Obtained
	end

DynamaxHole_EventScript_Challenge::
	lock
	faceplayer
	waitse
	playmoncry VAR_0x8008, CRY_MODE_ENCOUNTER
	message DynamaxHole_Text_Boss
	waitmessage
	waitmoncry
	delay 20
	waitbuttonpress
	return

DynamaxHole_EventScript_RunBattle::
	setflag FLAG_SYS_SPECIAL_WILD_BATTLE
	special StartLegendaryBattle
	waitstate
	clearflag FLAG_SYS_SPECIAL_WILD_BATTLE
	specialvar VAR_RESULT, GetBattleOutcome
	return

DynamaxHole_EventScript_Obtained::
	bufferspeciesname STR_VAR_1, VAR_0x8008
	fadescreen FADE_TO_BLACK
	removeobject VAR_LAST_TALKED
	fadescreen FADE_FROM_BLACK
	msgbox DynamaxHole_Text_Obtained, MSGBOX_DEFAULT
	release
	end

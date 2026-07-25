@ DIMENSION HOLE — Start-menu entry that drops the player into a randomly chosen
@ dungeon (an ULTRA HOLE or a DYNAMAX HOLE). The exact return spot is saved first
@ so the dungeon's "climb out" warp brings the player back to where they were.

DimensionHole_EventScript_Enter::
	lockall
	special SaveHoleReturnWarp
	msgbox DimensionHole_Text_Sense, MSGBOX_DEFAULT
	random 2
	goto_if_eq VAR_RESULT, 0, DimensionHole_EventScript_EnterUltra
	goto DimensionHole_EventScript_EnterDynamax

DimensionHole_EventScript_EnterUltra::
	fadescreen FADE_TO_BLACK
	warp MAP_ULTRA_HOLE, 0
	waitstate
	end

DimensionHole_EventScript_EnterDynamax::
	fadescreen FADE_TO_BLACK
	warp MAP_DYNAMAX_HOLE, 0
	waitstate
	end

@ Rolled from each hosting map's ON_TRANSITION: a dimensional hole (a physical
@ overworld entrance) appears there roughly 1 in 3 visits, so holes "spawn in
@ random places" as you travel. FLAG_WONDER_CARD_UNUSED_1 is the shared hide
@ flag for every overworld hole entrance (they're never all loaded at once).
DimensionHole_EventScript_RollSpawn::
	random 3
	goto_if_eq VAR_RESULT, 0, DimensionHole_EventScript_ShowHole
	setflag FLAG_WONDER_CARD_UNUSED_1
	return

DimensionHole_EventScript_ShowHole::
	clearflag FLAG_WONDER_CARD_UNUSED_1
	return

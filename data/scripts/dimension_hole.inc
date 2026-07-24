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

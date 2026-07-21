BattleHub_EventScript_Enter::
	lockall
	message BattleHub_Text_Welcome
	waitmessage
	msgbox BattleHub_Text_AskSingle, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, BattleHub_EventScript_Single
	msgbox BattleHub_Text_AskTeam, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, BattleHub_EventScript_Double
	msgbox BattleHub_Text_LinkInfo
	releaseall
	end

BattleHub_EventScript_Single::
	cleartrainerflag TRAINER_HUB_SINGLE
	trainerbattle_no_intro TRAINER_HUB_SINGLE, BattleHub_Text_SingleLose
	cleartrainerflag TRAINER_HUB_SINGLE
	releaseall
	end

BattleHub_EventScript_Double::
	special HealPlayerParty
	cleartrainerflag TRAINER_HUB_DOUBLE
	trainerbattle_double TRAINER_HUB_DOUBLE, BattleHub_Text_DoubleIntro, BattleHub_Text_DoubleLose, BattleHub_Text_NeedTwo
	cleartrainerflag TRAINER_HUB_DOUBLE
	releaseall
	end

TradeHub_EventScript_Enter::
	lockall
	msgbox TradeHub_Text_Info
	releaseall
	end

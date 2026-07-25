#include "global.h"
#include "game_language.h"
#include "event_scripts.h"
#include "korean_dialogue.h"

const u8 *GetLangString(const u8 *en, const u8 *ko)
{
    if (ko != NULL && gSaveBlock2Ptr->optionsLanguage == GAME_LANG_KOREAN)
        return ko;
    return en;
}

// Script/scene dialogue is baked into ROM as fixed strings, so it cannot be
// swapped by a language option on its own. Instead every line that has a
// translation is listed here, and the two places dialogue reaches the screen
// (ShowFieldMessage for NPC msgbox text, and Oak's opening scene) run the
// outgoing pointer through GameText_Localize below.
//
// Only lines present in this table are translated; anything else falls through
// unchanged and stays English. To translate more dialogue, add the Korean text
// to DIALOGUE in tools/korean_glyphs/gen_korean.py, re-run it, and add the pair
// here.
extern const u8 Text_WelcomeWantToHealPkmn[];
extern const u8 Text_TakeYourPkmnForFewSeconds[];
extern const u8 Text_WeHopeToSeeYouAgain[];
extern const u8 Text_RestoredPkmnToFullHealth[];
extern const u8 PalletTown_ProfessorOaksLab_Text_OakThreeMonsChooseOne[];
extern const u8 PalletTown_ProfessorOaksLab_Text_OakBePatientRival[];
extern const u8 PalletTown_ProfessorOaksLab_Text_OakWhichOneWillYouChoose[];
extern const u8 PalletTown_ProfessorOaksLab_Text_OakHeyDontGoAwayYet[];
extern const u8 PalletTown_ProfessorOaksLab_Text_OakThisMonIsEnergetic[];
extern const u8 PalletTown_ProfessorOaksLab_Text_ReceivedMonFromOak[];
extern const u8 PalletTown_PlayersHouse_1F_Text_YouShouldTakeQuickRest[];
extern const u8 PalletTown_PlayersHouse_1F_Text_LookingGreatTakeCare[];
extern const u8 PalletTown_PlayersHouse_1F_Text_AllBoysLeaveOakLookingForYou[];
extern const u8 PalletTown_PlayersHouse_1F_Text_AllGirlsLeaveOakLookingForYou[];

struct TranslatedText
{
    const u8 *en;
    const u8 *ko;
};

static const struct TranslatedText sTranslatedText[] =
{
    // PROF. OAK's opening speech
    { gOakSpeech_Text_WelcomeToTheWorld,          sKorOak_WelcomeToTheWorld },
    { gOakSpeech_Text_ThisWorld,                  sKorOak_ThisWorld },
    { gOakSpeech_Text_IsInhabitedFarAndWide,      sKorOak_IsInhabitedFarAndWide },
    { gOakSpeech_Text_IStudyPokemon,              sKorOak_IStudyPokemon },
    { gOakSpeech_Text_TellMeALittleAboutYourself, sKorOak_TellMeALittleAboutYourself },
    { gOakSpeech_Text_YourNameWhatIsIt,           sKorOak_YourNameWhatIsIt },
    { gOakSpeech_Text_SoYourNameIsPlayer,         sKorOak_SoYourNameIsPlayer },
    { gOakSpeech_Text_WhatWasHisName,             sKorOak_WhatWasHisName },
    { gOakSpeech_Text_YourRivalsNameWhatWasIt,    sKorOak_YourRivalsNameWhatWasIt },
    { gOakSpeech_Text_ConfirmRivalName,           sKorOak_ConfirmRivalName },
    { gOakSpeech_Text_RememberRivalsName,         sKorOak_RememberRivalsName },
    { gOakSpeech_Text_LetsGo,                     sKorOak_LetsGo },
    // POKeMON CENTER nurse -- reaches the screen through the NPC msgbox path
    { Text_WelcomeWantToHealPkmn,                 sKorNurse_Welcome },
    { Text_TakeYourPkmnForFewSeconds,             sKorNurse_TakeYourPkmn },
    { Text_WeHopeToSeeYouAgain,                   sKorNurse_SeeYouAgain },
    { Text_RestoredPkmnToFullHealth,              sKorNurse_Restored },
    // PROF. OAK's lab: being led in and given the first POKeMON
    { PalletTown_ProfessorOaksLab_Text_OakThreeMonsChooseOne,     sKorLab_OakThreeMonsChooseOne },
    { PalletTown_ProfessorOaksLab_Text_OakBePatientRival,         sKorLab_OakBePatientRival },
    { PalletTown_ProfessorOaksLab_Text_OakWhichOneWillYouChoose,  sKorLab_OakWhichOneWillYouChoose },
    { PalletTown_ProfessorOaksLab_Text_OakHeyDontGoAwayYet,       sKorLab_OakHeyDontGoAwayYet },
    { PalletTown_ProfessorOaksLab_Text_OakThisMonIsEnergetic,     sKorLab_OakThisMonIsEnergetic },
    { PalletTown_ProfessorOaksLab_Text_ReceivedMonFromOak,        sKorLab_ReceivedMonFromOak },
    // MOM at home
    { PalletTown_PlayersHouse_1F_Text_YouShouldTakeQuickRest,          sKorMom_TakeQuickRest },
    { PalletTown_PlayersHouse_1F_Text_LookingGreatTakeCare,            sKorMom_LookingGreat },
    { PalletTown_PlayersHouse_1F_Text_AllBoysLeaveOakLookingForYou,    sKorMom_AllBoysLeave },
    { PalletTown_PlayersHouse_1F_Text_AllGirlsLeaveOakLookingForYou,   sKorMom_AllGirlsLeave },
};

// Returns the translation of `str` for the selected language, or `str` itself
// when the language is English or the line has no translation. Matching is by
// pointer, so it is a short scan that never inspects string contents -- buffers
// built at runtime (gStringVar4 and friends) simply never match.
const u8 *GameText_Localize(const u8 *str)
{
    u32 i;

    if (str == NULL || gSaveBlock2Ptr->optionsLanguage != GAME_LANG_KOREAN)
        return str;

    for (i = 0; i < ARRAY_COUNT(sTranslatedText); i++)
    {
        if (sTranslatedText[i].en == str)
            return sTranslatedText[i].ko;
    }
    return str;
}

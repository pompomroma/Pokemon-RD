#include "global.h"
#include "game_language.h"

const u8 *GetLangString(const u8 *en, const u8 *ko)
{
    if (ko != NULL && gSaveBlock2Ptr->optionsLanguage == GAME_LANG_KOREAN)
        return ko;
    return en;
}

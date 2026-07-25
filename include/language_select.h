#ifndef GUARD_LANGUAGE_SELECT_H
#define GUARD_LANGUAGE_SELECT_H

// Boot-time language selection screen (English / Korean / Japanese / Chinese),
// shown after the intro and before the title screen. On confirm it stores the
// choice in gSaveBlock2Ptr->optionsLanguage and hands off to the title screen.
void CB2_InitLanguageSelect(void);

#endif // GUARD_LANGUAGE_SELECT_H

#!/usr/bin/env python3
# Bakes real Hangul glyphs for a curated set of Korean UI strings into the
# normal font's free glyph slots (0x118+), sets their widths, and emits a C
# header of the strings as byte arrays that use the engine's extra-symbol
# escape (0xF9 <n> -> glyph 0x100|n, or the wide 0xF9 0xFF <hi> <lo> form for
# the banks above 0x1FF).
import re, os
from PIL import Image, ImageFont, ImageDraw

ROOT = "/home/user/Pokemon-RD"

# Every font that Korean text can be printed with needs its OWN copy of the
# glyphs, because each font id reads a different .fwlatfont and a different
# width table. The dialogue box picks the font by who is speaking
# (new_menu_helpers.c DrawDialogueFrame path: FONT_MALE / FONT_FEMALE /
# FONT_NORMAL) and Oak's speech is always FONT_MALE (oak_speech.c), so baking
# into latin_normal alone left every male/female line as blank spaces of the
# right width. FONT_NORMAL and FONT_NORMAL_COPY_1 share latin_normal.fwlatfont
# but have separate width tables, so that sheet patches two tables.
FONT_TARGETS = [
    ("graphics/fonts/latin_normal.png",
     ["sFontNormalLatinGlyphWidths", "sFontNormalCopy1LatinGlyphWidths"]),
    ("graphics/fonts/latin_male.png",
     ["sFontMaleLatinGlyphWidths"]),
    ("graphics/fonts/latin_female.png",
     ["sFontFemaleLatinGlyphWidths"]),
]
# FONT_SMALL (latin_small.hwlatfont) is deliberately not included: it is a
# half-width 8px font used only for English hint lines ("PICK SWITCH CANCEL",
# the language-select footer), and Hangul is illegible at 8px.

PNG = os.path.join(ROOT, FONT_TARGETS[0][0])
TEXTC = os.path.join(ROOT, "src/text.c")
HDR = os.path.join(ROOT, "include/korean_ui_strings.h")
FONT = "/usr/share/fonts/truetype/nanum/NanumGothicBold.ttf"
if not os.path.exists(FONT):
    FONT = "/usr/share/fonts/truetype/nanum/NanumGothic.ttf"

# CJK font for the Japanese/Chinese language-select labels.
FONT_CJK = "/usr/share/fonts/opentype/noto/NotoSerifCJK-Bold.ttc"
LANGHDR = os.path.join(ROOT, "include/langselect_strings.h")

# Language-select labels rendered with the CJK font (Japanese / Chinese). The
# Korean and English labels reuse sKorText_Korean / plain Latin, so only these
# need new glyphs.
CJK_STRINGS = {
    "sLangText_Japanese": "日本語",
    "sLangText_Chinese":  "中文",
}

DLGHDR = os.path.join(ROOT, "include/korean_dialogue.h")

# --- Batch: high-frequency game UI strings -----------------------------------
# Keyed by the English symbol declared in include/strings.h. game_language.c
# includes that header, so the pair {symbol, sKorUI_symbol} needs no externs.
#
# Hangul advances a fixed 16px per syllable against roughly 6px for a Latin
# letter, so these are kept SHORT on purpose: several live in fixed-width
# windows (bag pockets, the party action list) where a literal translation
# would overrun the frame.
GAME_STRINGS = {
    # start menu
    "gText_MenuPokedex":        "도감",
    "gText_MenuPokemon":        "포켓몬",
    "gText_MenuBag":            "가방",
    "gText_MenuSave":           "저장",
    "gText_MenuOption":         "설정",
    "gText_MenuExit":           "나가기",
    "gText_MenuBattleHub":      "허브",
    "gText_MenuTradeHub":       "교환",
    "gText_MenuDimensionHole":  "차원홀",
    # confirmations
    "gText_Yes":                "예",
    "gText_No":                 "아니오",
    "gText_PartyMenu_OK":       "확인",
    "gText_OptionMenuCancel":   "취소",
    # bag pockets
    "gText_Items":              "도구",
    "gText_Items2":             "도구",
    "gText_KeyItems":           "중요도구",
    "gText_KeyItems2":          "중요도구",
    "gText_PokeBalls":          "몬스터볼",
    "gText_PokeBalls2":         "몬스터볼",
    "gText_TMCase":             "기술기계",
    "gText_TmCase":             "기술기계",
    "gText_BerryPouch":         "열매",
    "gText_BerryPouch_2":       "열매",
    # party / PC actions
    "gText_Switch2":            "교체",
    "gText_Summary5":           "능력치",
    "gText_Read2":              "읽기",
    "gText_Item":               "도구",
    "gText_Mail":               "편지",
    "gText_Take":               "받기",
    "gText_Store":              "맡기기",
    "gText_Shift":              "교대",
    "gText_Withdraw":           "꺼내기",
    "gText_Info":               "정보",
    "gText_Quit":               "그만",
    "gText_Toss":               "버리기",
    "gText_Register":           "등록",
    "gText_Box":                "박스",
    # stats and trainer card
    "gText_ItemEffect_Attack":  "공격",
    "gText_ItemEffect_Defense": "방어",
    "gText_ItemEffect_Speed":   "스피드",
    "gText_LevelUp_Attack":     "공격",
    "gText_LevelUp_Defense":    "방어",
    "gText_TrainerCardMoney":   "소지금",
    "gText_Time":               "시간",
    "gText_Badges":             "배지",
    "gText_Option":             "설정",
    "gText_FrameType":          "타입",
    # summary screen and stat labels
    "gText_ItemEffect_HP":      "체력",
    "gText_ItemEffect_SpAtk":   "특공",
    "gText_ItemEffect_SpDef":   "특방",
    "gText_LevelUp_Speed":      "스피드",
    "gText_PokeSum_ExpPoints":  "경험치",
    "gText_PokeSum_NextLv":     "다음레벨",
    "gText_PokeSum_Item_None":  "없음",
    "gText_ItemStorage":        "도구보관",
    "gText_ItemsPocket":        "도구주머니",
    "gText_Info_2":             "정보",
    "gText_YesNo":              "예\\n아니오",
}
GAMEHDR = os.path.join(ROOT, "include/korean_game_strings.h")


# Korean translations of PROF. OAK's opening speech (the game's most-read
# dialogue). Markup mirrors the .string syntax of the English source:
#   \n newline, \l scroll, \p new page, {PLAYER}/{RIVAL} name placeholders.
# Hangul glyphs advance 16px, so lines are kept to ~12 syllables to fit the box.
DIALOGUE = {
    "sKorOak_WelcomeToTheWorld":
        "안녕하세요!\\n만나서 반갑구나!\\p"
        "포켓몬 세계에\\n온 것을 환영한다!\\p"
        "내 이름은 오박사란다.\\p"
        "사람들은 나를\\n포켓몬 박사라 부르지.\\p",
    "sKorOak_ThisWorld":
        "이 세계는…",
    "sKorOak_IsInhabitedFarAndWide":
        "…포켓몬이라 불리는\\n생명체가 살고 있단다.\\p",
    "sKorOak_IStudyPokemon":
        "어떤 이에게는 친구이고\\n어떤 이는 함께 싸우지.\\p"
        "나로 말하자면…\\p"
        "포켓몬을 연구하고 있단다.\\p"
        "최근 내 연구는\\n놀라운 사실을 밝혀냈지…\\p"
        "포켓몬의 유대에는\\n상상을 뛰어넘는 힘이\\l깃들어 있었어!\\p"
        "하나가 되는 힘…\\n바로 퓨전이다!\\p",
    "sKorOak_TellMeALittleAboutYourself":
        "먼저 네 이야기를\\n조금 들려주겠니?\\p",
    "sKorOak_YourNameWhatIsIt":
        "이름부터 시작하자.\\n이름이 뭐니?\\p",
    "sKorOak_SoYourNameIsPlayer":
        "그렇구나…\\n네 이름은 {PLAYER}.",
    "sKorOak_WhatWasHisName":
        "이 아이는 내 손자란다.\\p"
        "아기 때부터\\n너의 라이벌이었지.\\p"
        "…음, 이름이 뭐였더라?",
    "sKorOak_YourRivalsNameWhatWasIt":
        "라이벌의 이름이\\n뭐였더라?",
    "sKorOak_ConfirmRivalName":
        "…어, {RIVAL}였던가?",
    "sKorOak_RememberRivalsName":
        "맞아! 이제 기억났다!\\n이름은 {RIVAL}!\\p",
    "sKorOak_LetsGo":
        "{PLAYER}!\\p"
        "너만의 포켓몬 전설이\\n이제 막 시작된다!\\p"
        "꿈과 모험이 가득한\\n포켓몬 세계로! 가자!",

    # POKeMON CENTER nurse -- the most repeated NPC dialogue in the game.
    # These reach the screen through ShowFieldMessage, the same path every NPC
    # msgbox uses, so they also prove out the NPC translation hook.
    "sKorNurse_Welcome":
        "포켓몬 센터에\\n오신 것을 환영합니다!\\p"
        "포켓몬을 완전히\\n회복시켜 드릴까요?",
    "sKorNurse_TakeYourPkmn":
        "네, 포켓몬을\\n잠시 맡아 두겠습니다.",
    "sKorNurse_SeeYouAgain":
        "또 오시기 바랍니다!",
    "sKorNurse_Restored":
        "기다려 주셔서 감사합니다.\\n"
        "포켓몬을 완전히\\l회복시켰습니다.",

    # PROF. OAK's lab -- the scene where the player is led in and given their
    # first POKeMON, plus MOM at home.
    "sKorLab_OakThreeMonsChooseOne":
        "오박사: {RIVAL}?\\n어디 보자…\\p"
        "아, 그렇지. 오라고\\n했었지! 잠깐 기다리렴!\\p"
        "여기다, {PLAYER}.\\p"
        "포켓몬이 세 마리 있단다.\\p"
        "하하!\\p"
        "포켓몬은 이 몬스터볼\\n안에 들어 있지.\\p"
        "나도 젊었을 때는\\n진지한 트레이너였단다.\\p"
        "지금은 나이가 들어\\n이 세 마리만 남았어.\\p"
        "하나를 가지렴.\\n자, 골라 보거라!",
    "sKorLab_OakBePatientRival":
        "오박사: 조금만 기다리렴,\\n{RIVAL}. 너도 하나 주마!",
    "sKorLab_OakWhichOneWillYouChoose":
        "오박사: 자, {PLAYER}.\\p"
        "저 세 개의 몬스터볼 안에\\n포켓몬이 들어 있단다.\\p"
        "어느 쪽을 고르겠니?",
    "sKorLab_OakHeyDontGoAwayYet":
        "오박사: 이런!\\n아직 가면 안 된다!",
    "sKorLab_OakThisMonIsEnergetic":
        "이 포켓몬은 정말\\n활기가 넘치는구나!",
    "sKorLab_ReceivedMonFromOak":
        "{PLAYER}은 오박사에게서\\n{STR_VAR_1}을 받았다!",

    "sKorMom_TakeQuickRest":
        "엄마: {PLAYER}!\\n잠깐 쉬어 가는 게 좋겠다.",
    "sKorMom_LookingGreat":
        "엄마: 아, 좋아!\\n너도 포켓몬도 건강해 보여.\\l조심해서 다녀오렴!",
    "sKorMom_AllBoysLeave":
        "엄마: …그래.\\n남자아이는 언젠가\\l집을 떠나는 법이지.\\p"
        "아 참. 옆집 오박사님이\\n너를 찾고 계셨단다.",
    "sKorMom_AllGirlsLeave":
        "엄마: …그래.\\n여자아이도 언젠가\\l여행을 꿈꾸는 법이지.\\p"
        "아 참. 옆집 오박사님이\\n너를 찾고 계셨단다.",
}

# Non-Hangul bytes, taken from charmap.txt.
PUNCT = {' ': 0x00, '!': 0xAB, '?': 0xAC, '.': 0xAD, '…': 0xB0, ',': 0xB8, ':': 0xF0}
MARKUP = [("\\n", [0xFE]), ("\\l", [0xFA]), ("\\p", [0xFB]),
          ("{PLAYER}", [0xFD, 0x01]), ("{RIVAL}", [0xFD, 0x06]),
          ("{STR_VAR_1}", [0xFD, 0x02]), ("{STR_VAR_2}", [0xFD, 0x03])]

def is_hangul(ch):
    return '가' <= ch <= '힣'

def dialogue_hangul(s):
    """Hangul syllables in a dialogue string, markup/punctuation stripped."""
    t = s
    for tok, _ in MARKUP:
        t = t.replace(tok, "")
    return [c for c in t if is_hangul(c)]

FIRST_GLYPH = 0x118   # first free glyph slot
# The Latin sheets are 256x2048 (2048 glyphs). Everything from 0x200 up is new
# space reached through the wide escape, which is what lifts the old ~232-slot
# ceiling that kept the translation "curated".
LAST_GLYPH = 0x7FF
GLYPH_W = 16          # advance width stored for Hangul cells

# name -> Korean text. These map to the English UI strings we localize.
STRINGS = {
    "sKorText_Option":        "설정",       # OPTION (title)
    "sKorText_TextSpeed":     "속도",       # TEXT SPEED
    "sKorText_BattleScene":   "전투연출",   # BATTLE SCENE
    "sKorText_BattleStyle":   "전투방식",   # BATTLE STYLE
    "sKorText_Sound":         "소리",       # SOUND
    "sKorText_ButtonMode":    "버튼모드",   # BUTTON MODE
    "sKorText_Frame":         "프레임",     # FRAME
    "sKorText_Language":      "언어",       # LANGUAGE
    "sKorText_Cancel":        "취소",       # CANCEL
    "sKorText_English":       "영어",       # ENGLISH
    "sKorText_Korean":        "한국어",     # KOREAN
    "sKorText_Slow":          "느림",       # SLOW
    "sKorText_Mid":           "보통",       # MID
    "sKorText_Fast":          "빠름",       # FAST
    "sKorText_On":            "켜기",       # ON
    "sKorText_Off":           "끄기",       # OFF
    "sKorText_Shift":         "시프트",     # SHIFT
    "sKorText_Set":           "세트",       # SET
    "sKorText_Mono":          "모노",       # MONO
    "sKorText_Stereo":        "스테레오",   # STEREO
}

# Collect unique glyphs in first-seen order. Korean strings (rendered with the
# Nanum font) come first so their slots are unchanged from before; the CJK
# language labels (rendered with Noto CJK) append after.
order = []
seen = set()
font_path_of = {}
for group, path in ((STRINGS, FONT), (CJK_STRINGS, FONT_CJK)):
    for s in group.values():
        for ch in s:
            if ch not in seen:
                seen.add(ch)
                order.append(ch)
                font_path_of[ch] = path
# The game-UI batch appends next, then Oak's dialogue, so previously assigned
# slots never move and older baked art stays valid.
for _s in GAME_STRINGS.values():
    for ch in dialogue_hangul(_s):
        if ch not in seen:
            seen.add(ch)
            order.append(ch)
            font_path_of[ch] = FONT
for s in DIALOGUE.values():
    for ch in dialogue_hangul(s):
        if ch not in seen:
            seen.add(ch)
            order.append(ch)
            font_path_of[ch] = FONT
# Slots run from FIRST_GLYPH up to the end of the enlarged sheets. Anything at
# or above 0x200 is emitted in the wide form (see encode_glyph).
assert len(order) <= (LAST_GLYPH + 1 - FIRST_GLYPH), (
    "too many syllables: %d, capacity %d" % (len(order), LAST_GLYPH + 1 - FIRST_GLYPH))
glyph_of = {ch: FIRST_GLYPH + i for i, ch in enumerate(order)}
print("unique glyphs:", len(order), "-> glyphs 0x%X..0x%X" % (FIRST_GLYPH, FIRST_GLYPH+len(order)-1))

# --- render into every font sheet -------------------------------------------
# The cell in the sheet is 16px tall, but the engine only ever DRAWS 14 rows:
# DecompressGlyph_Normal/_Male/_Female set gGlyphInfo.height = 14 and
# CopyGlyphToWindow clips to it (src/text.c, src/text_printer.c). Anything the
# generator puts on rows 14-15 is silently thrown away, which for Hangul means
# losing the bottom of the final consonant (받침) and turning 각 into 가. So the
# glyph is sized and centred to fit the drawable 14 rows, not the full cell.
CELL_H = 16
DRAW_H = 14

_font_cache = {}
def _font_for(ch):
    path = font_path_of[ch]
    if path not in _font_cache:
        # Sized to fit DRAW_H rows; CJK hanzi are denser and need one less.
        size = 13 if path == FONT_CJK else 14
        _font_cache[path] = ImageFont.truetype(path, size)
    return _font_cache[path]

def render_cell(px, ch, gid):
    font = _font_for(ch)
    col, row = gid % 16, gid // 16
    ox, oy = col * 16, row * CELL_H
    # clear the whole cell
    for y in range(CELL_H):
        for x in range(16):
            px[ox + x, oy + y] = 0
    # rasterize the syllable to a small grayscale then threshold to index 1
    tmp = Image.new("L", (16, CELL_H), 0)
    d = ImageDraw.Draw(tmp)
    bbox = d.textbbox((0, 0), ch, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    dx = (16 - w) // 2 - bbox[0]
    # Centre within the drawable rows only, and never let ink start so low that
    # the tail falls past row DRAW_H-1.
    dy = (DRAW_H - h) // 2 - bbox[1]
    if dy + bbox[1] + h > DRAW_H:
        dy = DRAW_H - h - bbox[1]
    if dy + bbox[1] < 0:
        dy = -bbox[1]
    d.text((dx, dy), ch, fill=255, font=font)
    tp = tmp.load()
    for y in range(DRAW_H):          # rows 14-15 would be clipped away anyway
        for x in range(16):
            if tp[x, y] >= 110:      # ink threshold
                px[ox + x, oy + y] = 1

for rel, _tables in FONT_TARGETS:
    path = os.path.join(ROOT, rel)
    sheet = Image.open(path)
    assert sheet.mode == "P" and sheet.size == (256, 2048), (rel, sheet.mode, sheet.size)
    spx = sheet.load()
    # Wipe every slot we own first. An earlier run may have used more glyphs
    # than this one, and leftover ink in a now-unreferenced slot would sit in
    # the sheet forever.
    for gid in range(FIRST_GLYPH, LAST_GLYPH + 1):
        ox, oy = (gid % 16) * 16, (gid // 16) * CELL_H
        for y in range(CELL_H):
            for x in range(16):
                spx[ox + x, oy + y] = 0
    for ch, gid in glyph_of.items():
        render_cell(spx, ch, gid)
    sheet.save(path)
    print("wrote", path)

# --- update the width tables ------------------------------------------------
txt = open(TEXTC).read()
for _rel, tables in FONT_TARGETS:
    for table in tables:
        m = re.search(r'(static const u8 ' + table + r'\[\]\s*=\s*\{)(.*?)(\};)', txt, re.S)
        assert m, "width table not found: " + table
        nums = re.findall(r'-?\d+', m.group(2))
        assert len(nums) == LAST_GLYPH + 1, (table, len(nums))
        nums = [int(n) for n in nums]
        for gid in glyph_of.values():
            nums[gid] = GLYPH_W
        # re-emit 16 per line
        lines = []
        for i in range(0, LAST_GLYPH + 1, 16):
            lines.append("    " + ", ".join(str(n) for n in nums[i:i + 16]) + ",")
        newbody = "\n" + "\n".join(lines) + "\n"
        txt = txt[:m.start()] + m.group(1) + newbody + m.group(3) + txt[m.end():]
        print("patched width table", table)
open(TEXTC, "w").write(txt)

# --- emit the C header ------------------------------------------------------
def encode_glyph(gid):
    """Bytes for one baked glyph.

    Narrow form (0xF9 n) only reaches glyph 0x100|n, so it stops at 0x1FF and
    cannot use n == 0xFF, which is the wide marker. Everything else goes out as
    the wide form (0xF9 0xFF hi lo), which carries a full 16-bit glyph id.
    """
    if gid < 0x200 and (gid & 0xFF) != 0xFF:
        return [0xF9, gid & 0xFF]
    return [0xF9, 0xFF, (gid >> 8) & 0xFF, gid & 0xFF]

def to_bytes(s):
    """Encode a UI string: Hangul syllables, punctuation and markup tokens."""
    out = []
    i = 0
    while i < len(s):
        for tok, code in MARKUP:      # \n \l \p {PLAYER} {RIVAL} {STR_VAR_n}
            if s.startswith(tok, i):
                out += code
                i += len(tok)
                break
        else:
            ch = s[i]
            # Any baked glyph, so this also serves the CJK language-select
            # labels, whose characters are not Hangul.
            if ch in glyph_of:
                out += encode_glyph(glyph_of[ch])
            elif ch in PUNCT:
                out.append(PUNCT[ch])
            else:
                raise SystemExit("unmapped char %r in UI string %r" % (ch, s))
            i += 1
    out.append(0xFF)              # EOS
    return out

with open(HDR, "w") as f:
    f.write("#ifndef GUARD_KOREAN_UI_STRINGS_H\n#define GUARD_KOREAN_UI_STRINGS_H\n\n")
    f.write("// Auto-generated by tools scratch gen_korean.py. Real Hangul UI strings\n")
    f.write("// encoded as extra-symbol escapes into the baked font glyph slots.\n\n")
    for name, s in STRINGS.items():
        b = to_bytes(s)
        arr = ", ".join("0x%02X" % x for x in b)
        f.write("static const u8 %s[] = { %s }; // %s\n" % (name, arr, s))
    f.write("\n#endif // GUARD_KOREAN_UI_STRINGS_H\n")
print("wrote", HDR)

# The Japanese/Chinese language-select labels go in their own header.
with open(LANGHDR, "w") as f:
    f.write("#ifndef GUARD_LANGSELECT_STRINGS_H\n#define GUARD_LANGSELECT_STRINGS_H\n\n")
    f.write("// Auto-generated by tools scratch gen_korean.py. Native-script names\n")
    f.write("// (Japanese/Chinese) for the boot language-select screen, baked into\n")
    f.write("// the normal font's free glyph slots.\n\n")
    for name, s in CJK_STRINGS.items():
        b = to_bytes(s)
        arr = ", ".join("0x%02X" % x for x in b)
        f.write("static const u8 %s[] = { %s }; // %s\n" % (name, arr, s))
    f.write("\n#endif // GUARD_LANGSELECT_STRINGS_H\n")
print("wrote", LANGHDR)

# --- emit the dialogue header ----------------------------------------------
def encode_dialogue(s):
    out = []
    i = 0
    while i < len(s):
        for tok, code in MARKUP:          # \n \l \p {PLAYER} {RIVAL}
            if s.startswith(tok, i):
                out += code
                i += len(tok)
                break
        else:
            ch = s[i]
            if is_hangul(ch):
                out += encode_glyph(glyph_of[ch])
            elif ch in PUNCT:
                out.append(PUNCT[ch])
            else:
                raise SystemExit("unmapped dialogue char %r in %r" % (ch, s))
            i += 1
    out.append(0xFF)                       # EOS
    return out

with open(DLGHDR, "w") as f:
    f.write("#ifndef GUARD_KOREAN_DIALOGUE_H\n#define GUARD_KOREAN_DIALOGUE_H\n\n")
    f.write("// Auto-generated by tools/korean_glyphs/gen_korean.py.\n")
    f.write("// Korean translations of PROF. OAK's opening speech, encoded with the\n")
    f.write("// engine's control codes (newline/scroll/page, name placeholders) and\n")
    f.write("// extra-symbol escapes into the baked Hangul glyph slots.\n\n")
    for name, s in DIALOGUE.items():
        b = encode_dialogue(s)
        arr = ", ".join("0x%02X" % x for x in b)
        f.write("static const u8 %s[] = { %s };\n\n" % (name, arr))
    f.write("#endif // GUARD_KOREAN_DIALOGUE_H\n")
print("wrote", DLGHDR)
print("TOTAL glyphs used: %d of %d slots (0x%X..0x%X)" % (len(order), LAST_GLYPH + 1 - FIRST_GLYPH, FIRST_GLYPH, LAST_GLYPH))

# --- emit the game-UI batch header ------------------------------------------
with open(GAMEHDR, "w") as f:
    f.write("#ifndef GUARD_KOREAN_GAME_STRINGS_H\n#define GUARD_KOREAN_GAME_STRINGS_H\n\n")
    f.write("// Auto-generated by tools/korean_glyphs/gen_korean.py -- do not edit.\n")
    f.write("// Korean for the game's high-frequency UI strings, encoded as extra-symbol\n")
    f.write("// escapes into the baked Hangul glyph slots. KOREAN_GAME_STRING_LIST pairs\n")
    f.write("// each one with the English symbol of the same name from include/strings.h.\n\n")
    for name, txt in GAME_STRINGS.items():
        data = ", ".join("0x%02X" % b for b in to_bytes(txt))
        f.write("static const u8 sKorUI_%s[] = {%s}; // %s\n" % (name, data, txt))
    f.write("\n#define KOREAN_GAME_STRING_LIST \\\n")
    for name in GAME_STRINGS:
        f.write("    X(%s) \\\n" % name)
    f.write("\n\n#endif // GUARD_KOREAN_GAME_STRINGS_H\n")
print("wrote", GAMEHDR, "(%d strings)" % len(GAME_STRINGS))

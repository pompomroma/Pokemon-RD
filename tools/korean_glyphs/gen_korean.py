#!/usr/bin/env python3
# Bakes real Hangul glyphs for a curated set of Korean UI strings into the
# normal font's free glyph slots (0x118+), sets their widths, and emits a C
# header of the strings as byte arrays that use the engine's extra-symbol
# escape (0xF9 <n> -> glyph 0x100|n).
import re, os
from PIL import Image, ImageFont, ImageDraw

ROOT = "/home/user/Pokemon-RD"
PNG = os.path.join(ROOT, "graphics/fonts/latin_normal.png")
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
# Oak's dialogue appends after the UI/CJK glyphs so existing slots never move.
for s in DIALOGUE.values():
    for ch in dialogue_hangul(s):
        if ch not in seen:
            seen.add(ch)
            order.append(ch)
            font_path_of[ch] = FONT
assert len(order) <= (0x200 - FIRST_GLYPH), "too many syllables for free slots"
glyph_of = {ch: FIRST_GLYPH + i for i, ch in enumerate(order)}
print("unique glyphs:", len(order), "-> glyphs 0x%X..0x%X" % (FIRST_GLYPH, FIRST_GLYPH+len(order)-1))

# --- render into the PNG ---------------------------------------------------
im = Image.open(PNG)
assert im.mode == "P" and im.size == (256, 512), (im.mode, im.size)
px = im.load()
_font_cache = {}
def _font_for(ch):
    path = font_path_of[ch]
    if path not in _font_cache:
        # CJK kanji/hanzi need a slightly smaller size to fit a 16x16 cell.
        size = 14 if path == FONT_CJK else 15
        _font_cache[path] = ImageFont.truetype(path, size)
    return _font_cache[path]

def render_cell(ch, gid):
    font = _font_for(ch)
    col, row = gid % 16, gid // 16
    ox, oy = col * 16, row * 16
    # clear the 16x16 cell
    for y in range(16):
        for x in range(16):
            px[ox + x, oy + y] = 0
    # rasterize the syllable to a small grayscale then threshold to index 1
    tmp = Image.new("L", (16, 16), 0)
    d = ImageDraw.Draw(tmp)
    bbox = d.textbbox((0, 0), ch, font=font)
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    dx = (16 - w) // 2 - bbox[0]
    dy = (16 - h) // 2 - bbox[1]
    d.text((dx, dy), ch, fill=255, font=font)
    tp = tmp.load()
    for y in range(16):
        for x in range(16):
            if tp[x, y] >= 110:      # ink threshold
                px[ox + x, oy + y] = 1

for ch, gid in glyph_of.items():
    render_cell(ch, gid)
im.save(PNG)
print("wrote", PNG)

# --- update the width table -------------------------------------------------
txt = open(TEXTC).read()
m = re.search(r'(static const u8 sFontNormalLatinGlyphWidths\[\]\s*=\s*\{)(.*?)(\};)', txt, re.S)
nums = re.findall(r'-?\d+', m.group(2))
assert len(nums) == 512, len(nums)
nums = [int(n) for n in nums]
for gid in glyph_of.values():
    nums[gid] = GLYPH_W
# re-emit 16 per line
lines = []
for i in range(0, 512, 16):
    lines.append("    " + ", ".join(str(n) for n in nums[i:i+16]) + ",")
newbody = "\n" + "\n".join(lines) + "\n"
txt = txt[:m.start()] + m.group(1) + newbody + m.group(3) + txt[m.end():]
open(TEXTC, "w").write(txt)
print("patched width table in", TEXTC)

# --- emit the C header ------------------------------------------------------
def to_bytes(s):
    out = []
    for ch in s:
        gid = glyph_of[ch]
        out.append(0xF9)          # CHAR_EXTRA_SYMBOL
        out.append(gid & 0xFF)    # low byte -> glyph 0x100|n
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
                out += [0xF9, glyph_of[ch] & 0xFF]
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
print("TOTAL glyphs used: %d of %d free slots" % (len(order), 0x200 - FIRST_GLYPH))

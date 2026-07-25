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

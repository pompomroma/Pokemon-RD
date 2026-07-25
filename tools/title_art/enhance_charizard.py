#!/usr/bin/env python3
"""Enhance the title-screen Charizard's shading detail.

The art is a 96x96 tile sheet whose tiles are assembled by a tilemap, and the
build pins it to exactly 135 tiles, so the sheet's size and tile layout must not
change -- only pixel values inside existing tiles.

Detail is added by (a) filling four unused palette slots with extra shades,
including a bright rim-light tone the original lacked, and (b) enhancing the
shading on the *composed* image (the only place neighbouring pixels are truly
adjacent) before writing the result back tile by tile.
"""
import struct, sys
from PIL import Image, ImageFilter

ROOT = "/home/user/Pokemon-RD"
SHEET = ROOT + "/graphics/title_screen/firered/box_art_mon.png"
TMAP = ROOT + "/graphics/title_screen/firered/box_art_mon.bin"
MAPW, MAPH = 32, 20

WRITE = "--write" in sys.argv
OUT = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None

sheet = Image.open(SHEET)
assert sheet.size == (96, 96) and sheet.mode == "P"
pal = list(sheet.getpalette())
sp = sheet.load()
SW = 96 // 8

# --- enrich the palette -----------------------------------------------------
# Slots 2..5 are unused (magenta/teal placeholders). Fill them with shades that
# extend and smooth the existing dark-maroon -> orange ramp, and add the bright
# highlight the original never had.
NEW = {
    2: (247, 178, 74),   # bright rim / fire highlight (new brightest tone)
    3: (140, 45, 0),     # between idx 10 and 11
    4: (194, 57, 0),     # between idx 12 and 13
    5: (216, 106, 0),    # between idx 14 and 15
}
for i, (r, g, b) in NEW.items():
    pal[i * 3:i * 3 + 3] = [r, g, b]

# Shade indices usable for the body, ordered dark -> bright.
BODY = [6, 7, 8, 9, 10, 3, 11, 12, 4, 13, 14, 5, 15, 2]
lum = {i: 0.299 * pal[i * 3] + 0.587 * pal[i * 3 + 1] + 0.114 * pal[i * 3 + 2] for i in BODY}
BODY.sort(key=lambda i: lum[i])

# --- compose ----------------------------------------------------------------
ents = struct.unpack("<%dH" % (MAPW * MAPH), open(TMAP, "rb").read()[:MAPW * MAPH * 2])
comp = Image.new("P", (MAPW * 8, MAPH * 8))
comp.putpalette(pal)
cp = comp.load()
# origin[(cx,cy)] -> (sheet_x, sheet_y) for write-back
origin = {}
for i, e in enumerate(ents):
    t, hf, vf = e & 0x3FF, (e >> 10) & 1, (e >> 11) & 1
    tx, ty = (t % SW) * 8, (t // SW) * 8
    bx, by = (i % MAPW) * 8, (i // MAPW) * 8
    for y in range(8):
        for x in range(8):
            sx, sy = (7 - x if hf else x), (7 - y if vf else y)
            cp[bx + x, by + y] = sp[tx + sx, ty + sy]
            origin.setdefault((bx + x, by + y), (tx + sx, ty + sy))

W, H = comp.size
before = comp.copy()

# --- build a luminance field over the body ---------------------------------
L = Image.new("L", (W, H), 0)
lp = L.load()
body_mask = [[False] * W for _ in range(H)]
for y in range(H):
    for x in range(W):
        v = cp[x, y]
        if v in lum:
            body_mask[y][x] = True
            lp[x, y] = int(round(lum[v]))
        else:
            lp[x, y] = 0

blur = L.filter(ImageFilter.GaussianBlur(1.1))
bp = blur.load()

# --- enhance ----------------------------------------------------------------
DETAIL = 0.55      # unsharp strength: crisper internal shading
RIM = 34.0         # rim-light strength on up-left facing silhouette edges
out = [[cp[x, y] for x in range(W)] for y in range(H)]

def is_body(x, y):
    return 0 <= x < W and 0 <= y < H and body_mask[y][x]

for y in range(H):
    for x in range(W):
        if not body_mask[y][x]:
            continue
        base = float(lp[x, y])
        # local contrast (unsharp): pulls out folds, scales, muscle shading
        val = base + DETAIL * (base - float(bp[x, y]))
        # rim light: brighten pixels on the upper/left boundary of the silhouette
        if not is_body(x, y - 1) or not is_body(x - 1, y):
            val += RIM
        # slight ambient occlusion where the body meets its lower/right edge
        if not is_body(x, y + 1) or not is_body(x + 1, y):
            val -= 10.0
        # snap to the nearest available shade
        best, bd = out[y][x], 1e9
        for i in BODY:
            d = abs(lum[i] - val)
            if d < bd:
                bd, best = d, i
        out[y][x] = best

for y in range(H):
    for x in range(W):
        cp[x, y] = out[y][x]

changed = sum(1 for y in range(H) for x in range(W) if before.load()[x, y] != cp[x, y])
body_px = sum(1 for y in range(H) for x in range(W) if body_mask[y][x])
print("body pixels: %d, changed: %d (%.1f%%)" % (body_px, changed, 100.0 * changed / max(body_px, 1)))

if OUT:
    n = 3
    side = Image.new("RGB", (W * n * 2 + 12, H * n), (18, 18, 22))
    side.paste(before.convert("RGB").resize((W * n, H * n), Image.NEAREST), (0, 0))
    side.paste(comp.convert("RGB").resize((W * n, H * n), Image.NEAREST), (W * n + 12, 0))
    side.save(OUT)
    print("wrote preview", OUT)

if WRITE:
    # write the enhanced pixels back into the sheet (tile layout untouched)
    for (cx, cy), (sx, sy) in origin.items():
        sp[sx, sy] = cp[cx, cy]
    sheet.putpalette(pal)
    sheet.save(SHEET)
    print("wrote", SHEET)

    # The game's palette is built from the JASC-PAL file (Makefile prefers
    # %.gbapal: %.pal), so the new shades must be written there too or they
    # render as the original magenta placeholders.
    PALF = ROOT + "/graphics/title_screen/firered/box_art_mon.pal"
    lines = open(PALF).read().splitlines()
    hdr = 3  # "JASC-PAL", "0100", count
    for i, (r, g, b) in NEW.items():
        lines[hdr + i] = "%d %d %d" % (r, g, b)
    open(PALF, "w").write("\n".join(lines) + "\n")
    print("wrote", PALF)

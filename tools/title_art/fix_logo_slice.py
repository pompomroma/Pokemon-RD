#!/usr/bin/env python3
"""Restore the sliced bottom of the POKeMON wordmark on the title screen.

The R6 retitle cleared composed rows 50-53 -- the wordmark's lower blue outline
and the TM mark -- to make room for FUSION RED at y=54, which left the logo
looking cut off. This rebuilds the composed logo as:

    rows  0-53 : upstream wordmark, complete (restores the slice)
    FUSION RED : moved down so it clears the restored outline
    subtitle   : unchanged

then re-derives the tile sheet + tilemap. The sheet is fixed at 256x56 = 224
tiles by its PNG size, so the rebuilt tile set must fit in 224 unique tiles.
"""
import struct, sys
from PIL import Image

ROOT = "/home/user/Pokemon-RD"
SP = "/tmp/claude-0/-home-user-Pokemon-RD/dfbdfa08-2b64-5975-b974-d3888a41fca4/scratchpad"
SHEET = ROOT + "/graphics/title_screen/firered/game_title_logo.png"
TMAP = ROOT + "/graphics/title_screen/firered/game_title_logo.bin"
PALF = ROOT + "/graphics/title_screen/firered/game_title_logo.pal"
MAPW, MAPH = 32, 20
BG = 0
WRITE = "--write" in sys.argv
OUT = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None

FUSION_TOP, FUSION_BOT = 54, 72   # current FUSION RED band
SHIFT = 6                         # push it down clear of the restored outline


def compose(base_png, base_bin, pal=None):
    sh = Image.open(base_png)
    shp = sh.load()
    SW = sh.size[0] // 8
    p = pal if pal else sh.getpalette()
    d = open(base_bin, "rb").read()
    ents = struct.unpack("<%dH" % (len(d) // 2), d)
    im = Image.new("P", (MAPW * 8, MAPH * 8))
    im.putpalette(p)
    ip = im.load()
    for i, e in enumerate(ents[: MAPW * MAPH]):
        t, hf, vf = e & 0x3FF, (e >> 10) & 1, (e >> 11) & 1
        tx, ty = (t % SW) * 8, (t // SW) * 8
        bx, by = (i % MAPW) * 8, (i // MAPW) * 8
        for y in range(8):
            for x in range(8):
                sx, sy = (7 - x if hf else x), (7 - y if vf else y)
                ip[bx + x, by + y] = shp[tx + sx, ty + sy]
    return im


cur = compose(SHEET, TMAP)
pal = list(Image.open(SHEET).getpalette())
# Upstream art, remapped into the *current* palette by colour so the restored
# rows use today's (polished) palette entries rather than stale indices.
up_png, up_bin = SP + "/rev/d7d3fa1_logo.png", SP + "/rev/d7d3fa1_logo.bin"
up = compose(up_png, up_bin)
up_pal = Image.open(up_png).getpalette()

cur_colors = {}
for i in range(256):
    cur_colors.setdefault(tuple(pal[i * 3:i * 3 + 3]), i)

def map_to_cur(idx_up):
    c = tuple(up_pal[idx_up * 3: idx_up * 3 + 3])
    if c in cur_colors:
        return cur_colors[c]
    best, bd = 0, 1e18
    for i in range(208):
        pc = tuple(pal[i * 3:i * 3 + 3])
        d = sum((pc[k] - c[k]) ** 2 for k in range(3))
        if d < bd:
            bd, best = d, i
    return best

W, H = cur.size
tgt = Image.new("P", (W, H))
tgt.putpalette(pal)
tp = tgt.load()
cp = cur.load()
upp = up.load()

# 1) blank everything, 2) lay the FUSION RED band down lower, 3) copy the rest
for y in range(H):
    for x in range(W):
        tp[x, y] = BG

# subtitle + anything below the FUSION band: copy from current unchanged
for y in range(FUSION_BOT + 1, H):
    for x in range(W):
        tp[x, y] = cp[x, y]

# FUSION RED shifted down
for y in range(FUSION_TOP, FUSION_BOT + 1):
    for x in range(W):
        v = cp[x, y]
        if v != BG:
            ny = y + SHIFT
            if ny < H:
                tp[x, ny] = v

# wordmark rows 0..53 restored from upstream (this is the slice fix)
for y in range(0, FUSION_TOP):
    for x in range(W):
        v = upp[x, y]
        if v != BG:
            tp[x, y] = map_to_cur(v)

# --- re-apply the wordmark polish ------------------------------------------
# The restored rows come from upstream art, so they arrive without the internal
# anti-aliasing added last round. Re-apply it here (same rule: never write the
# transparent backdrop, and only blend against non-background neighbours).
used_now = set(tgt.tobytes())
free = [i for i in range(208) if i not in used_now]
exact = {tuple(pal[i * 3:i * 3 + 3]): i for i in used_now}

def pal_index(c):
    if c in exact:
        return exact[c]
    best, bd = None, 1e18
    for col, idx in exact.items():
        d = sum((col[k] - c[k]) ** 2 for k in range(3))
        if d < bd:
            bd, best = d, idx
    if bd <= 12 or not free:
        return best
    n = free.pop(0)
    pal[n * 3:n * 3 + 3] = list(c)
    exact[c] = n
    return n

rgb = {}
for y in range(0, FUSION_TOP):
    for x in range(W):
        rgb[(x, y)] = tuple(pal[tp[x, y] * 3: tp[x, y] * 3 + 3])

aa = {}
for y in range(0, FUSION_TOP):
    for x in range(W):
        if tp[x, y] == BG:
            continue
        r, g, b = rgb[(x, y)]
        acc, wsum = [r * 4, g * 4, b * 4], 4
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < W and 0 <= ny < FUSION_TOP and tp[nx, ny] != BG:
                nr, ng, nb = rgb[(nx, ny)]
                acc[0] += nr; acc[1] += ng; acc[2] += nb
                wsum += 1
        k = 0.35
        aa[(x, y)] = pal_index((
            int(round(r + (acc[0] / wsum - r) * k)),
            int(round(g + (acc[1] / wsum - g) * k)),
            int(round(b + (acc[2] / wsum - b) * k))))
for (x, y), v in aa.items():
    tp[x, y] = v
tgt.putpalette(pal)
print("wordmark polish re-applied (%d px, %d palette slots left)" % (len(aa), len(free)))

# --- re-derive tiles --------------------------------------------------------
tiles = []
index = {}
tmap = []
for ty in range(MAPH):
    for tx in range(MAPW):
        key = bytes(tp[tx * 8 + x, ty * 8 + y] for y in range(8) for x in range(8))
        if key not in index:
            index[key] = len(tiles)
            tiles.append(key)
        tmap.append(index[key])

print("unique tiles needed: %d (sheet holds 224)" % len(tiles))
if len(tiles) > 224:
    print("TOO MANY TILES - aborting"); sys.exit(1)

if OUT:
    n = 3
    crop = (0, 0, 200, 90)
    a = cur.convert("RGB").crop(crop)
    b = tgt.convert("RGB").crop(crop)
    cw, ch = (crop[2] - crop[0]) * n, (crop[3] - crop[1]) * n
    side = Image.new("RGB", (cw, ch * 2 + 12), (18, 18, 22))
    side.paste(a.resize((cw, ch), Image.NEAREST), (0, 0))
    side.paste(b.resize((cw, ch), Image.NEAREST), (0, ch + 12))
    side.save(OUT)
    print("wrote preview", OUT)

if WRITE:
    sheet = Image.new("P", (256, 56))
    sheet.putpalette(pal)
    shp = sheet.load()
    for t, data in enumerate(tiles):
        ox, oy = (t % 32) * 8, (t // 32) * 8
        for y in range(8):
            for x in range(8):
                shp[ox + x, oy + y] = data[y * 8 + x]
    sheet.save(SHEET)
    open(TMAP, "wb").write(struct.pack("<%dH" % len(tmap), *tmap))
    print("wrote", SHEET, "and", TMAP)

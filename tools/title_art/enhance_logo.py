#!/usr/bin/env python3
"""Polish the title-screen logo: the POKeMON wordmark, the FUSION RED subtitle
and the author line.

Rules that keep this safe:
  * index 0 is the transparent backdrop -- background pixels are never written,
    so no coloured halo can appear around the art.
  * blending only ever samples non-background neighbours, so edges soften
    against the outline instead of bleeding green into the glyphs.
  * new shades are allocated from the free palette indices below 208 (the logo
    only gets 13 sub-palettes loaded; 208+ belongs to Charizard).
  * the tile sheet keeps its size and tile layout, so the tilemap stays valid.
"""
import struct, sys
from PIL import Image

ROOT = "/home/user/Pokemon-RD"
SHEET = ROOT + "/graphics/title_screen/firered/game_title_logo.png"
TMAP = ROOT + "/graphics/title_screen/firered/game_title_logo.bin"
PALF = ROOT + "/graphics/title_screen/firered/game_title_logo.pal"
MAPW, MAPH = 32, 20
BG = 0
PAL_LIMIT = 208

WRITE = "--write" in sys.argv
OUT = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None

sheet = Image.open(SHEET)
pal = list(sheet.getpalette())
sp = sheet.load()
SW = sheet.size[0] // 8

# --- compose ---------------------------------------------------------------
ents = struct.unpack("<%dH" % (MAPW * MAPH), open(TMAP, "rb").read()[:MAPW * MAPH * 2])
comp = Image.new("P", (MAPW * 8, MAPH * 8))
comp.putpalette(pal)
cp = comp.load()
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
rgb = [[tuple(pal[cp[x, y] * 3: cp[x, y] * 3 + 3]) for x in range(W)] for y in range(H)]

# --- palette allocator ------------------------------------------------------
used = set(comp.tobytes())
free = [i for i in range(PAL_LIMIT) if i not in used]
exact = {tuple(pal[i * 3:i * 3 + 3]): i for i in used}

def pal_index(c):
    """Nearest existing palette entry, or allocate a new one if far enough off."""
    if c in exact:
        return exact[c]
    best, bd = None, 1e18
    for idx, col in exact.items():
        d = (idx[0] - c[0]) ** 2 + (idx[1] - c[1]) ** 2 + (idx[2] - c[2]) ** 2
        if d < bd:
            bd, best = d, col
    if bd <= 12 or not free:          # close enough / out of slots
        return best
    n = free.pop(0)
    pal[n * 3:n * 3 + 3] = list(c)
    exact[c] = n
    return n

FUSION_TOP, FUSION_BOT = 54, 70       # "FUSION RED" band
def is_bg(x, y):
    return not (0 <= x < W and 0 <= y < H) or cp[x, y] == BG

out = {}
for y in range(H):
    for x in range(W):
        if cp[x, y] == BG:
            continue                    # never touch the transparent backdrop
        r, g, b = rgb[y][x]

        # 1) warm gradient across FUSION RED: near-white at the top of the
        #    glyphs falling to warm gold at the bottom, matching the wordmark
        #    and the game's warm grade instead of flat white.
        if FUSION_TOP <= y <= FUSION_BOT and (r > 180 and g > 180 and b > 180):
            t = (y - FUSION_TOP) / float(FUSION_BOT - FUSION_TOP)
            r = int(round(255 - 7 * t))
            g = int(round(252 - 40 * t))
            b = int(round(245 - 150 * t))

        # 2) internal anti-aliasing: average with non-background neighbours so
        #    fill/outline steps soften. Background is excluded, so glyph edges
        #    stay crisp against the backdrop.
        acc = [r * 4, g * 4, b * 4]
        wsum = 4
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if not is_bg(nx, ny):
                nr, ng, nb = rgb[ny][nx]
                if FUSION_TOP <= ny <= FUSION_BOT and nr > 180 and ng > 180 and nb > 180:
                    tt = (ny - FUSION_TOP) / float(FUSION_BOT - FUSION_TOP)
                    nr, ng, nb = 255 - int(7 * tt), 252 - int(40 * tt), 245 - int(150 * tt)
                acc[0] += nr; acc[1] += ng; acc[2] += nb
                wsum += 1
        avg = (acc[0] / wsum, acc[1] / wsum, acc[2] / wsum)
        k = 0.35
        c = (int(round(r + (avg[0] - r) * k)),
             int(round(g + (avg[1] - g) * k)),
             int(round(b + (avg[2] - b) * k)))
        out[(x, y)] = pal_index(c)

for (x, y), v in out.items():
    cp[x, y] = v
comp.putpalette(pal)

changed = sum(1 for (x, y), v in out.items() if before.load()[x, y] != v)
print("non-bg pixels: %d, changed: %d, new palette entries used: %d (free left %d)"
      % (len(out), changed, 74 - len(free), len(free)))

if OUT:
    n = 2
    side = Image.new("RGB", (W * n, H * n * 2 + 10), (18, 18, 22))
    side.paste(before.convert("RGB").resize((W * n, H * n), Image.NEAREST), (0, 0))
    side.paste(comp.convert("RGB").resize((W * n, H * n), Image.NEAREST), (0, H * n + 10))
    side.save(OUT)
    print("wrote preview", OUT)

if WRITE:
    for (cx, cy), (sx, sy) in origin.items():
        sp[sx, sy] = cp[cx, cy]
    sheet.putpalette(pal)
    sheet.save(SHEET)
    lines = open(PALF).read().splitlines()
    hdr = 3
    for i in range(256):
        lines[hdr + i] = "%d %d %d" % (pal[i * 3], pal[i * 3 + 1], pal[i * 3 + 2])
    open(PALF, "w").write("\n".join(lines) + "\n")
    print("wrote", SHEET, "and", PALF)

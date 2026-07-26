#!/usr/bin/env python3
"""Bake sun glints into the overworld tilesets.

The palette grade in src/palette.c decides the COLOUR of the light. This adds
the DETAIL: a sparkle on the points of each surface the sun actually catches, so
roofs, path stones, water and grass have highlights in the art itself rather than
just a warmer tint over flat shapes.

Three constraints make an automated pass over shared tile art safe, and all three
are enforced here:

1. SEAMS. The same 8x8 tile is placed all over the map, so a filter that reached
   across tile edges would make repeats disagree and show grid lines. Only the
   6x6 interior of a tile is ever written; the 1px border ring is read but never
   modified, so tiles still butt together exactly.

2. PALETTES. A tile is drawn with whichever palette its metatile names, and some
   tiles are used with several. Changing a pixel's index changes it under every
   one of those palettes, so tiles used with more than one palette are skipped
   entirely (they are 0-6% of tiles) and the rest are remapped only within their
   own palette.

3. FLIPPING. Metatiles may draw a tile x- or y-flipped, so a directional
   highlight (say "light from the upper left") would come from the wrong side on
   ~27% of placements. The operator used here -- promote strict local maxima of
   luma -- is symmetric under both flips, so a glint lands on the same physical
   bump however the tile is oriented.

Run with --write to apply; default is a dry run that only reports counts.
"""
import glob
import os
import struct
import sys
from collections import defaultdict

from PIL import Image

ROOT = "/home/user/Pokemon-RD/data/tilesets"
WRITE = "--write" in sys.argv

# A tile whose interior is mostly local maxima is dithered or noisy; glinting it
# would read as speckle rather than shine, so leave it alone.
MAX_CHANGES_PER_TILE = 10
# How far a candidate colour may drift in hue to count as "the same material,
# lit". Keeps a green blade promoting to a lighter green, never to a red.
MAX_HUE_DIST = 90
# Tiles are indexed across both sheets; see tile_palettes().
NUM_TILES_IN_PRIMARY = 640
# A crest needs this many strictly-darker neighbours. Requiring a *strict* local
# maximum almost never fires on flat-shaded pixel art (neighbours tie), and
# requiring none at all would glint every flat field. This picks the top of a
# shaded highlight while staying flip-symmetric.
MIN_DARKER_NEIGHBOURS = 2
# A crest standing proud of this many neighbours gets a double-strength glint.
STRONG_CREST_NEIGHBOURS = 5


def load_pal(path):
    t = open(path).read().split()
    n = int(t[2])
    v = list(map(int, t[3:3 + n * 3]))
    return [tuple(v[i * 3:i * 3 + 3]) for i in range(n)]


def luma(c):
    return (c[0] * 77 + c[1] * 151 + c[2] * 28) >> 8


def hue_dist(a, b):
    """Colour distance after normalising both to the same brightness."""
    la, lb = max(luma(a), 1), max(luma(b), 1)
    d = 0
    for k in range(3):
        na = a[k] * 128 // la
        nb = b[k] * 128 // lb
        d += (na - nb) ** 2
    return d


def build_promote(pal, used):
    """For each palette index, the index one step brighter in the same ramp."""
    promote = {}
    for i in used:
        if i == 0:
            continue
        best, bestd = None, None
        for j in used:
            if j == 0 or j == i:
                continue
            dl = luma(pal[j]) - luma(pal[i])
            if dl <= 0:
                continue
            if hue_dist(pal[i], pal[j]) > MAX_HUE_DIST:
                continue
            if bestd is None or dl < bestd:
                bestd, best = dl, j
        if best is not None:
            promote[i] = best
    return promote


def tile_palettes(tsdir, is_secondary):
    """tile id -> set of palette numbers it is drawn with, from metatiles.bin.

    Metatiles address primary and secondary tiles in one space: ids below
    NUM_TILES_IN_PRIMARY index the primary tiles.png, ids at or above it index
    the secondary sheet starting from 0. Each sheet only cares about its own
    half, so the other half is dropped and secondary ids are rebased.
    """
    mt = os.path.join(tsdir, "metatiles.bin")
    if not os.path.exists(mt):
        return None
    d = open(mt, "rb").read()
    n = len(d) // 2
    vals = struct.unpack("<%dH" % n, d[:n * 2])
    t2p = defaultdict(set)
    for v in vals:
        tid = v & 0x3FF
        if is_secondary:
            if tid < NUM_TILES_IN_PRIMARY:
                continue
            tid -= NUM_TILES_IN_PRIMARY
        elif tid >= NUM_TILES_IN_PRIMARY:
            continue
        t2p[tid].add((v >> 12) & 0xF)
    return t2p


def process(tsdir):
    png = os.path.join(tsdir, "tiles.png")
    paldir = os.path.join(tsdir, "palettes")
    if not (os.path.exists(png) and os.path.isdir(paldir)):
        return None
    is_secondary = os.sep + "secondary" + os.sep in tsdir
    t2p = tile_palettes(tsdir, is_secondary)
    if not t2p:
        return None

    im = Image.open(png).convert("P")
    W, H = im.size
    px = im.load()
    cols = W // 8
    pals = {}
    for p in set(x for s in t2p.values() for x in s):
        f = os.path.join(paldir, "%02d.pal" % p)
        if os.path.exists(f):
            pals[p] = load_pal(f)

    out = [[px[x, y] for x in range(W)] for y in range(H)]
    changed = 0
    tiles_touched = 0

    for tid, palset in t2p.items():
        if len(palset) != 1:
            continue  # constraint 2: shared across palettes, leave alone
        p = next(iter(palset))
        if p not in pals:
            continue
        pal = pals[p]
        ty, tx = divmod(tid, cols)
        ox, oy = tx * 8, ty * 8
        if oy + 8 > H or ox + 8 > W:
            continue

        used = set()
        for y in range(8):
            for x in range(8):
                used.add(px[ox + x, oy + y])
        promote = build_promote(pal, used)
        if not promote:
            continue

        picks = []
        for y in range(1, 7):          # constraint 1: interior only
            for x in range(1, 7):
                i = px[ox + x, oy + y]
                if i == 0 or i not in promote:
                    continue
                li = luma(pal[i])
                # constraint 3: "no neighbour brighter, several darker" is
                # symmetric under x/y flip, so the glint lands on the same
                # physical bump whichever way the tile is drawn.
                is_max = True
                darker = 0
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        if dx == 0 and dy == 0:
                            continue
                        ln = luma(pal[px[ox + x + dx, oy + y + dy]])
                        if ln > li:
                            is_max = False
                            break
                        if ln < li:
                            darker += 1
                    if not is_max:
                        break
                if is_max and darker >= MIN_DARKER_NEIGHBOURS:
                    # Flip-safe glinting can only ever mark a sparse set of
                    # pixels, so make each one count: a crest that stands proud
                    # of most of its neighbours takes two steps up the ramp
                    # instead of one, which reads as a sparkle rather than a
                    # barely-there tint shift.
                    v = promote[i]
                    if darker >= STRONG_CREST_NEIGHBOURS and v in promote:
                        v = promote[v]
                    picks.append((ox + x, oy + y, v))

        if not picks or len(picks) > MAX_CHANGES_PER_TILE:
            continue
        for x, y, v in picks:
            out[y][x] = v
        changed += len(picks)
        tiles_touched += 1

    if WRITE and changed:
        for y in range(H):
            for x in range(W):
                px[x, y] = out[y][x]
        im.save(png)
    return tiles_touched, changed


total_tiles = total_px = 0
rows = []
for d in sorted(glob.glob(ROOT + "/*/*/")):
    r = process(d)
    if r and r[1]:
        tt, ch = r
        total_tiles += tt
        total_px += ch
        rows.append((os.path.relpath(d, ROOT).rstrip("/"), tt, ch))

for name, tt, ch in rows:
    print("%-34s %4d tiles glinted, %5d px" % (name, tt, ch))
print("\n%d tilesets, %d tiles, %d pixels %s"
      % (len(rows), total_tiles, total_px, "(written)" if WRITE else "(dry run)"))

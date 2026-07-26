#!/usr/bin/env python3
"""Add detail to the battle backgrounds.

Battle terrain is a *tileset*: the same 8x8 tiles are placed many times by the
tilemap, so any filter that reaches across tile edges would make repeats
disagree and show seams. This sharpens only the 6x6 interior of each tile and
never touches the 1px border ring, so tiles still butt together perfectly while
their insides gain texture (grass blades, rock facets, water ripples).

Colours are requantised to each terrain's existing palette, and a single spare
palette slot (where one exists) is filled with a mid tone to smooth banding.
The palette is written to the .pal file too, since the Makefile builds gbapal
from .pal, not from the PNG.
"""
import glob, os, sys
from collections import Counter
from PIL import Image

ROOT = "/home/user/Pokemon-RD/graphics/battle_terrain"
WRITE = "--write" in sys.argv
STRENGTH = 0.55   # interior local-contrast amount


def load_pal(path):
    lines = open(path).read().split()
    # JASC-PAL: "JASC-PAL", "0100", count, then r g b triples
    n = int(lines[2])
    vals = list(map(int, lines[3:3 + n * 3]))
    return [tuple(vals[i * 3:i * 3 + 3]) for i in range(n)]


def save_pal(path, cols):
    out = ["JASC-PAL", "0100", str(len(cols))]
    out += ["%d %d %d" % c for c in cols]
    open(path, "w").write("\n".join(out) + "\n")


def nearest(cols, c, allowed):
    best, bd = allowed[0], 1e18
    for i in allowed:
        p = cols[i]
        d = (p[0] - c[0]) ** 2 + (p[1] - c[1]) ** 2 + (p[2] - c[2]) ** 2
        if d < bd:
            bd, best = d, i
    return best


def enhance(dirpath):
    png = os.path.join(dirpath, "terrain.png")
    palf = os.path.join(dirpath, "terrain.pal")
    if not (os.path.exists(png) and os.path.exists(palf)):
        return None
    im = Image.open(png)
    px = im.load()
    W, H = im.size
    cols = load_pal(palf)
    used = sorted(set(im.tobytes()))
    free = [i for i in range(len(cols)) if i not in used]

    # Fill one spare slot with a mid tone between the two most common colours,
    # giving the requantiser a smoother ramp to land on.
    if free:
        common = [c for c, _ in Counter(im.tobytes()).most_common(2)]
        if len(common) == 2:
            a, b = cols[common[0]], cols[common[1]]
            cols[free[0]] = tuple((a[k] + b[k]) // 2 for k in range(3))
            used.append(free[0])

    changed = 0
    out = [[px[x, y] for x in range(W)] for y in range(H)]
    for ty in range(H // 8):
        for tx in range(W // 8):
            ox, oy = tx * 8, ty * 8
            # interior only: never the border ring, so repeats stay seamless
            for y in range(1, 7):
                for x in range(1, 7):
                    cx, cy = ox + x, oy + y
                    c = cols[px[cx, cy]]
                    # local mean of the 8 neighbours (all inside this tile's 8x8)
                    acc = [0, 0, 0]
                    for dy in (-1, 0, 1):
                        for dx in (-1, 0, 1):
                            n = cols[px[cx + dx, cy + dy]]
                            acc[0] += n[0]; acc[1] += n[1]; acc[2] += n[2]
                    mean = [v / 9.0 for v in acc]
                    want = tuple(
                        max(0, min(255, int(round(c[k] + STRENGTH * (c[k] - mean[k])))))
                        for k in range(3))
                    ni = nearest(cols, want, used)
                    if ni != px[cx, cy]:
                        changed += 1
                    out[cy][cx] = ni

    if WRITE:
        for y in range(H):
            for x in range(W):
                px[x, y] = out[y][x]
        flat = []
        for c in cols:
            flat += list(c)
        im.putpalette(flat)
        im.save(png)
        save_pal(palf, cols)
    return os.path.basename(dirpath.rstrip("/")), changed, W * H


total = 0
for d in sorted(glob.glob(ROOT + "/*/")):
    r = enhance(d)
    if r:
        name, changed, px_count = r
        total += changed
        print("%-12s %5d px sharpened of %d" % (name, changed, px_count))
print("total pixels changed:", total, "(written)" if WRITE else "(dry run)")

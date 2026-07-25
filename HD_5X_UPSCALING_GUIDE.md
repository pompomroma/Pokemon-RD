# FireRed Fusion Edition — 5× HD Upscaling & Smoothing Guide

## Why this is a guide, not a ROM setting

The Game Boy Advance renders at a **hardware-fixed 240×160**. That resolution is
burned into the video hardware (`DISPLAY_WIDTH`/`DISPLAY_HEIGHT`) — a ROM cannot
change its own resolution or PPI, and there is no render-scale to "multiply by
5". The only way to actually get a **5× (≈1200×800), smoothed, molded, higher-PPI
image** is to upscale the emulator's output with an HD filter. Those filters
(xBRZ / HQx / Super-xBR) don't just enlarge — they **round corners, anti-alias
edges, and mold the pixel art into smooth higher-detail-looking graphics**, which
is exactly the "immensely smoothen / polish / mold" look.

Pair this with the in-ROM changes shipped alongside it (battle backgrounds are
now rendered **crisp** — the depth-of-field blur was removed — and the whole game
uses the warm, smooth, polished color grade).

**Target:** integer scale **5×** = **1200×800**, plus an xBRZ-class smoothing
filter.

---

## RetroArch (best result — recommended)

RetroArch has the strongest upscaling/shader chain.

1. Load the ROM with the **mGBA** core (Load Core → mGBA), or **gpSP** / **VBA
   Next**.
2. **Settings → Video → Scaling**
   - Integer Scale: **ON** (keeps pixels clean before the filter), or set a
     Windowed/Fullscreen resolution of **1200×800** (or larger, e.g. 1920×1280).
3. **Main Menu → Shaders → Load Shader Preset** and pick one:
   - `shaders/…/xbrz/5xbrz.glslp` (or `6xbrz`) — **best "molding/smoothing"**;
     rounds and cleans the pixel art.
   - `hqx/hq4x.glslp` — softer, cartoon-smooth.
   - `super-xbr` or `scalefx` — sharper "HD redraw" feel.
   - Add `anti-aliasing/…` or a light `crt/` preset on top for extra polish.
4. Save: **Shaders → Save Preset → Save Game/Core Preset** so it auto-loads.

This yields a 5×, anti-aliased, molded image while staying pixel-faithful.

---

## mGBA (standalone)

1. **Tools → Settings → Video**
   - Set the window/frame size to **5×** (240×160 → 1200×800). Use
     **Fullscreen** with "Lock aspect ratio" on if you want it maximized.
   - Enable **Bilinear filtering** for smoothing (this is the built-in smooth
     option; standalone mGBA does not include xBRZ — use RetroArch's mGBA core
     for xBRZ).
2. Optional: **Video → Frame blending** for softer motion.

Standalone mGBA gives you 5× + bilinear smoothing out of the box; for the full
xBRZ "molding" use RetroArch above.

---

## VBA-M

1. **Options → Video → Render Method**: OpenGL or Direct3D.
2. **Options → Video → Scaling Filter (or "Filter")**: choose
   **xBRZ 5x / 6x**, **HQ4x**, **Super Eagle**, or **2xSaI** — these smooth and
   mold the art.
3. **Options → Video → Display size**: **5×** (or Fullscreen).
4. Optional **Interframe blending**: Smart, for softer edges.

---

## Mobile emulators (Manic EMU, Delta, Lemuroid, RetroArch mobile, …)

1. Open the emulator's **Video / Display settings**.
2. Turn on the **HD / Smoothing / "Enhanced" filter** (naming varies: "Smooth",
   "HQx", "xBRZ", "Anti-alias", "Retina/HD").
3. Set **scaling to Fill / 5× / Max** so the 240×160 frame is drawn at your
   device's full pixel density (that IS the higher-PPI result on the panel).
4. On RetroArch-based mobile: load the **mGBA core** + an **xBRZ 5x** shader as
   in the RetroArch section.

---

## Honest limits

- These filters are **display-side interpolation**. They make the image bigger,
  smoother, anti-aliased and "molded", and they read as far higher detail/PPI —
  but they don't add data the ROM doesn't have; a filter infers the extra pixels.
- A literal 5× **in the ROM** is not possible on GBA hardware, and hand-redrawing
  every tile for true native detail needs a pixel artist (an automated pass on
  the 8×8, 16-colour tile atlas breaks seamless tiling and muddies the art, so it
  is intentionally not shipped).
- The in-ROM half of this upgrade — **crisp (un-blurred) battle backgrounds** and
  the **warm, smooth, polished grade** — is baked into the ROM and needs no setup.

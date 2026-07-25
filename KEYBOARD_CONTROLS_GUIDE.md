# FireRed Fusion Edition — Keyboard Controls on Mobile Emulators

## How keyboard play actually works

The game reads input from the Game Boy Advance's **hardware key register**
(`REG_KEYINPUT`), which only ever reports the console's **10 buttons**:

`A  B  L  R  START  SELECT  ↑ ↓ ← →`

A GBA ROM has no way to see a keyboard — there is no keyboard device on the
hardware. Instead, **the emulator maps your keyboard keys onto those 10 buttons**,
and the game receives them as ordinary button presses. That means:

- **Every feature of this hack is fully playable by keyboard** — nothing needs a
  touchscreen, and no menu is touch-only.
- Keyboard support is enabled by **mapping keys in your emulator's settings**
  (below), not by anything in the ROM.

Connect a **Bluetooth or USB-C/OTG keyboard** to your phone or tablet, then map
the keys once. Most mobile emulators also let you keep the on-screen touch
buttons active at the same time.

---

## Recommended key map

Designed so the two most-used buttons sit under your right hand and the special
controls of this hack are easy to reach.

| GBA button | Suggested key | Used for |
|---|---|---|
| **A** | `X` (or `Z`, `Space`, `Enter`) | Confirm, talk, select move |
| **B** | `Z` (or `X`, `Backspace`) | Cancel, **hold to run** |
| **↑ ↓ ← →** | `Arrow keys` (or `W A S D`) | Move, navigate menus |
| **START** | `Enter` (or `Return`) | Start menu, **form change in battle** |
| **SELECT** | `Right Shift` (or `Backspace`) | Reorder moves in battle |
| **L** | `Q` (or `A`) | **Z-Move slot**, help/registered item |
| **R** | `W` (or `S`) | **Z-Move slot**, registered item |

> **Both L and R open the Z-Move slot.** Shoulder buttons are the most awkward
> keys to reach on many phone-keyboard layouts, so this hack accepts **either**
> one — map whichever is comfortable. (Exception: if you set
> **OPTIONS → BUTTON MODE → L=A**, then L acts as the A button, so use **R** for
> the Z-Move.)

---

## This hack's special controls

| Action | Button | Where |
|---|---|---|
| **Form change** (huge stat boost, once per battle) | **START** | Move-select screen |
| **Z-Move** (5th move slot) | **L or R**, or **→** from the right column | Move-select screen |
| Reorder moves | **SELECT** | Move-select screen |
| Dimension Hole / Battle Hub / Trade Hub | **START** menu | Overworld |
| Language select | **↑ ↓** + **A** | Boot screen |

---

## Per-emulator setup

### RetroArch (Android / iOS) — most flexible
1. Load the ROM with the **mGBA** core.
2. **Settings → Input → Port 1 Controls**.
3. Select each GBA button and press the key you want to bind.
   - Tip: set **Device Type** to your keyboard, or use
     **Settings → Input → Keyboard Mapping** if shown.
4. **Settings → Input → Hotkeys** — bind Menu/Fast-Forward while you're there.
5. Back out; the map saves automatically (or **Configuration → Save Current
   Configuration**).

### Delta (iOS)
1. Open **Settings (gear) → Controllers**.
2. Select your connected **hardware keyboard**.
3. Tap each GBA button and press the key to assign it.
4. Optionally set **Controller Opacity** to 0 to hide the on-screen buttons.

### Manic EMU (iOS)
1. Connect the keyboard, then open **Settings → Controller / External Controller**.
2. Choose the keyboard and assign each GBA button by pressing a key.
3. If a "Keyboard" profile already exists, verify **L** and **R** are mapped —
   they're the ones most often left blank, and this hack uses them for Z-Moves
   (either one works).

### Lemuroid (Android)
1. Connect the keyboard.
2. **Settings → Gamepad / Input settings → (your keyboard) → Edit bindings**.
3. Press a key for each GBA button.

### My Boy! / John GBA / Pizza Boy (Android)
1. **Menu → Settings → Key mappings** (or **Input settings → Key mapping**).
2. Tap each GBA button, then press the keyboard key to bind it.
3. Save/exit. Some builds have a separate **"External controller"** page — map
   there if the on-screen list ignores your keyboard.

### mGBA (desktop/laptop keyboards)
**Tools → Settings → Controllers → Keyboard**, then click each button and press
a key.

---

## Troubleshooting

- **Keyboard does nothing:** the emulator may be treating it as a text-input
  device. Look for an **"External controller" / "Hardware keyboard"** toggle and
  enable it, then re-map.
- **Some keys don't register together:** cheap keyboards have limited key
  rollover. Avoid binding several actions to keys in the same row/cluster — e.g.
  keep the D-pad on arrows and A/B on `Z`/`X`.
- **Arrow keys scroll the emulator UI instead of the game:** tap the game screen
  once to give it focus, or hide the on-screen overlay.
- **L or R won't map:** you only need **one** of them for Z-Moves — this hack
  accepts either. Map whichever your keyboard/emulator allows.
- **Running doesn't work:** running is **hold B** (the Running Shoes), not a
  separate key. In this hack normal walking is already bike-fast.

---

## Honest note

Nothing here is a ROM limitation being worked around — the ROM already accepts
all input as standard GBA buttons, which is exactly what emulator keyboard
mapping produces. The only in-ROM change made for keyboard comfort is that the
**Z-Move slot accepts L as well as R**, so an awkward-to-map shoulder key never
blocks a feature.

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

> **If A and B do nothing — see the fix directly below.** This is the single most
> common Manic EMU keyboard problem and it is a mapping/iOS issue, not a game bug.

---

## FIX: "A and B buttons don't work" (Manic EMU / iOS keyboards)

The game cannot tell a keyboard from a gamepad — it only ever receives the GBA's
10 buttons. So when A and B do nothing but the D-pad works, the A/B **bindings**
are not reaching the emulator. Work through these in order:

**1. Re-bind A and B to plain letter keys.**
The usual cause is that A/B are bound to keys **iOS itself swallows**. Never bind
A or B to `Space`, `Enter`/`Return`, `Tab`, `Esc`, arrow keys, or anything with
`Cmd`. Use plain letters:
- **A → `X`**, **B → `Z`** (or A → `K`, B → `J`).

Open the keyboard/controller profile, clear the old A and B bindings, then set
them fresh to those letters.

**2. Turn OFF iOS Full Keyboard Access.**
`iOS Settings → Accessibility → Keyboards → Full Keyboard Access → OFF`.
When this is on, iOS intercepts keys for interface navigation and the emulator
never receives them — this alone can kill A/B.

**3. Make sure the keyboard is being used as a *controller*, not text input.**
In Manic EMU look for **External Controller / Hardware Keyboard / Controller**
and confirm your keyboard is selected there and the profile is **enabled** for
the GBA core. If the keyboard is only registered as a text device, letters do
nothing in-game.

**4. Check for a duplicate/conflicting binding.**
If the key you assigned to A is *also* assigned to an emulator hotkey (menu,
fast-forward, save state) or to another GBA button, the emulator can swallow it.
Each key should appear exactly once in the whole profile.

**5. Re-seat the keyboard.**
Disconnect/reconnect the Bluetooth keyboard (or toggle Bluetooth), then reopen
the game. iOS sometimes attaches a keyboard *after* the emulator has started and
the emulator doesn't pick it up until relaunch.

**6. Verify with the on-screen buttons.**
Tap the on-screen A and B once. If those work, the game and ROM are fine and the
problem is purely the keyboard profile — repeat step 1.

### You are not locked out of the game meanwhile
The boot **language screen accepts A, B *or* START**, and the title screen accepts
the same three. So even with A and B unmapped you can always reach the game with
**START** and open **OPTIONS** to adjust controls.

You can also set **OPTIONS → BUTTON MODE → L=A**, which makes the **L** button act
as **A** — a working substitute if your A key can't be bound at all.

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

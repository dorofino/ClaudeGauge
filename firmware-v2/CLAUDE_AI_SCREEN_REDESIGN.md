# Claude.ai Screen Redesign — v2 Firmware (lcars-esp32)

## Goal

Redesign the firmware-v2 Claude.ai screen to match the v1-style layout:
large donut gauges with countdown clocks, vertical bars, divider, and full frame chrome.

## Architecture

- lcars-esp32 engine draws frame (elbows, sidebar, top/bottom bars, title) before `onDraw()`
- TNG theme: topbar = `LCARS_AMBER`, bottombar = `LCARS_TAN`
- Engine renders title "CLAUDE.AI USAGE" in topbar (LCARS_FONT_SM, black on amber)
- Screen receives full sprite + content rect (x=54, y=22, w=264, h=130 for 320x170)
- Screen CAN draw anywhere on sprite — used for chrome on bars

## Screen Layout (absolute coordinates, 320x170)

```text
0                54                    168                      320
+----------------+---------------------+--------------------------+ 0
| ELBOW(amber)   | CLAUDE.AI USAGE     |        CLAUDE  *        | 18 (topbar)
+-sidebar--------+                     |                          | 22 (content top)
|                |  2:22:00        ||  |   7d 0h0m               | ~24 (clocks, lg 28px)
|  segment 1     |                 ||  |                          |
|                |   +--------+   || | |    +--------+           |
|  segment 2     |   |  58%   |   || | |    |  45%   |           | ~100 (gauge centers)
|                |   +--------+   ||   |    +--------+           |
|  segment 3     |                 ||  |                          |
|                |                     |                          | 152 (content bottom)
| ELBOW(lavend.) | 1/2  SIGNAL .||  REFRESH IN ========----     | 154-170 (bottombar)
+----------------+---------------------+--------------------------+
```

## Content Elements (inside onDraw)

### 5-Hour Section (Left Half)

| Element          | X   | Y   | Details                                            |
| ---------------- | --- | --- | -------------------------------------------------- |
| 5h clock text    | 96  | 24  | LCARS_FONT_LG (28px), TC_DATUM, LCARS_WHITE        |
| 5h donut gauge   | cx=96  | cy=100 | r=38, thickness=7, LCARS_ICE fg, LCARS_BAR_TRACK bg |
| 5h pct text      | 96  | 96  | LCARS_FONT_MD, MC_DATUM centered in donut, white    |
| 5h countdown bar | 144 | 24  | w=10, h=120, fills bottom-up, LCARS_ICE             |

### 7-Day Section (Right Half)

| Element          | X    | Y   | Details                                                  |
| ---------------- | ---- | --- | -------------------------------------------------------- |
| 7d clock text    | 244  | 24  | LCARS_FONT_LG (28px), TC_DATUM, LCARS_PEACH             |
| 7d donut gauge   | cx=244 | cy=100 | r=38, thickness=7, LCARS_LAVENDER fg, LCARS_BAR_TRACK bg |
| 7d pct text      | 244  | 96  | LCARS_FONT_MD, MC_DATUM centered in donut, white         |
| 7d countdown bar | 292  | 24  | w=10, h=120, fills bottom-up, LCARS_LAVENDER             |

### Divider

| Element          | X   | Y  | Details                       |
| ---------------- | --- | -- | ----------------------------- |
| Vertical divider | 168 | 24 | w=2, h=120, LCARS_AMBER       |

## Frame Chrome (drawn on engine's bars by the screen)

| Element        | Position      | Details                                                              |
| -------------- | ------------- | -------------------------------------------------------------------- |
| "CLAUDE" text  | x=294, y=9    | LCARS_FONT_MD, MR_DATUM, LCARS_PEACH on theme->barTop bg            |
| Spark icon     | x=308, y=9    | Orange circle r=3 + 4 cross rays + 4 diagonal rays                  |
| Page chip      | x=54, y=155   | 28x14 pill, LCARS_PEACH bg, black text "1/2", builtin font          |
| "SIGNAL" label | x=170, y=162  | builtin font, MR_DATUM, LCARS_TEXT_DIM on theme->barBottom bg        |
| Signal bars    | x=174, y=157  | 4 bars (h=3,6,9,12px, w=3, gap=2), colored by wifi_rssi             |
| Refresh pill   | x=200, y=155  | 116x14 pill, LCARS_AMBER bg, "REFRESH IN" + segmented bar inside    |

## Countdown Clock Format

```text
>= 24h:  "Xd Yh"     (e.g., "7d 0h")
>= 1h:   "H:MM:SS"    (e.g., "2:22:00")
< 1h:    "M:SS"        (e.g., "45:30")
<= 0:    "0:00"
```

## Signal Bars Logic

```text
RSSI > -55  → 4 bars lit
RSSI > -65  → 3 bars lit
RSSI > -75  → 2 bars lit
RSSI > -85  → 1 bar lit
else        → 0 bars lit
Lit color: LCARS_ICE, Unlit color: LCARS_TEXT_DIM
```

## Refresh Pill Internal Layout

```text
|<-- 116px total -->|
+-------------------------------------------+
| REFRESH IN  ||||||||||||--------           |
+-------------------------------------------+
  ^text(x+4)  ^bar(x+64)         ^end(x+w-7)

Segment: 4px wide, 1px gap, h-4 tall
Filled = LCARS_PEACH, Unfilled = LCARS_BLACK
Percent = time elapsed / REFRESH_INTERVAL_MS
```

## Color Reference

| Purpose      | Constant          | Hex     |
| ------------ | ----------------- | ------- |
| 5h gauge     | LCARS_ICE         | #99CCFF |
| 7d gauge     | LCARS_LAVENDER    | #CC99CC |
| Divider      | LCARS_AMBER       | #FF9900 |
| Clock 5h     | LCARS_WHITE       | #F5F6FA |
| Clock 7d     | LCARS_PEACH       | #FF8866 |
| Chrome text  | LCARS_PEACH       | #FF8866 |
| Spark icon   | LCARS_ORANGE      | #FF8800 |
| Page chip bg | LCARS_PEACH       | #FF8866 |
| Signal lit   | LCARS_ICE         | #99CCFF |
| Signal unlit | LCARS_TEXT_DIM    | #888888 |
| Refresh pill | LCARS_AMBER       | #FF9900 |
| Bar track    | LCARS_BAR_TRACK   | #1A1A2E |
| Gauge pct    | LCARS_WHITE       | #F5F6FA |

## Files to Modify

1. **`firmware-v2/src/screens/claudeai_screen.h`** — Add screen info, chrome helpers, change title
2. **`firmware-v2/src/screens/claudeai_screen.cpp`** — Complete rewrite of onDraw()
3. **`firmware-v2/src/main.cpp`** — Pass screen index/total to screen
4. **`designer/index.html`** — Update buildClaudeAiV2() and v2 frame chrome

## What Gets Removed

- Per-model bars (Opus/Sonnet progress bars) — not in target layout
- Extra credits section — not in target layout
- Small gauge layout (r=20, labels to right) — replaced by large gauges

## What Gets Added

- Large donut gauges (r=38, thickness=7) with centered percentage text
- Countdown clocks in H:MM:SS format (LCARS_FONT_LG, 28px)
- Vertical countdown bars (10px wide, 120px tall, fill bottom-up)
- Vertical amber divider (2px wide)
- Frame chrome: "CLAUDE" text, spark icon, page chip, signal bars, refresh countdown pill

#!/usr/bin/env python3
"""
LCARS Dashboard Preview Renderer
Renders all screens to PNG images at exact pixel coordinates,
matching the ESP32 firmware layouts.
  - Firmware v1: 6 screens (overview, models, code, status)
  - Firmware v2: 2 screens (claude.ai subscription, system status)
Edit coordinates here, preview instantly with: python tools/preview.py
Output: tools/preview_*.png + preview_all.png
"""

from PIL import Image, ImageDraw, ImageFont
import os, math

# ============================================================
# Display constants (must match ui_widgets.h)
# ============================================================
SCR_W = 320
SCR_H = 170

SIDEBAR_W = 34
ELBOW_R = 14
TOPBAR_H = 18
BOTBAR_H = 16
BAR_GAP = 3

CONTENT_X = SIDEBAR_W + ELBOW_R + 4   # 52
CONTENT_Y = TOPBAR_H + 12             # 30
CONTENT_W = SCR_W - CONTENT_X - 4     # 264
CONTENT_H = SCR_H - CONTENT_Y - BOTBAR_H - 4  # 120

CX, CY, CW, CH = CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H

TOTAL_SCREENS = 6

# Line spacing (based on actual GFX font yAdvance)
H_SM = 16
H_MD = 24
H_LG = 34
H_XL = 44
H_TOK = 25  # Cost/token row: label+value line + bar below

# ============================================================
# Colors (RGB tuples from RGB565 values in colors.h)
# ============================================================
CLR_BG         = (0, 0, 0)
CLR_AMBER      = (255, 153, 0)
CLR_SALMON     = (255, 119, 119)
CLR_PEACH      = (255, 204, 153)
CLR_LAVENDER   = (204, 153, 204)
CLR_BLUE       = (153, 153, 255)
CLR_ORANGE     = (255, 102, 0)
CLR_RED        = (204, 68, 68)
CLR_SKY        = (153, 204, 255)
CLR_TAN        = (212, 170, 119)
CLR_TEXT_BRIGHT= (255, 255, 255)
CLR_TEXT_DIM   = (76, 76, 76)
CLR_BAR_BG     = (26, 26, 26)
CLR_STATUS_OK  = CLR_SKY
CLR_STATUS_ERR = CLR_RED

# ============================================================
# Font loading
# ============================================================
FONT_PATH = os.path.join(os.path.dirname(__file__), "Antonio.ttf")

def load_fonts():
    """Load Antonio font at 4 sizes matching the ESP32 GFX fonts."""
    try:
        return {
            'sm': ImageFont.truetype(FONT_PATH, 12),
            'md': ImageFont.truetype(FONT_PATH, 18),
            'lg': ImageFont.truetype(FONT_PATH, 28),
            'xl': ImageFont.truetype(FONT_PATH, 36),
        }
    except Exception as e:
        print(f"Warning: Could not load Antonio font: {e}")
        print("Falling back to default font")
        f = ImageFont.load_default()
        return {'sm': f, 'md': f, 'lg': f, 'xl': f}

# Built-in font 1 approximation for chips/page numbers (8px)
def load_builtin():
    try:
        return ImageFont.truetype(FONT_PATH, 8)
    except:
        return ImageFont.load_default()

# ============================================================
# Drawing helpers
# ============================================================
class Screen:
    def __init__(self, fonts):
        self.img = Image.new('RGB', (SCR_W, SCR_H), CLR_BG)
        self.draw = ImageDraw.Draw(self.img)
        self.fonts = fonts
        self.builtin = load_builtin()

    def text(self, x, y, text, font_key, color, datum='TL'):
        """Draw text with datum alignment."""
        font = self.fonts[font_key]
        bbox = font.getbbox(text)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        ty_offset = bbox[1]

        if datum == 'TL':
            dx, dy = x, y - ty_offset
        elif datum == 'TR':
            dx, dy = x - tw, y - ty_offset
        elif datum == 'MC':
            dx = x - tw // 2
            dy = y - th // 2 - ty_offset
        elif datum == 'ML':
            dx = x
            dy = y - th // 2 - ty_offset
        elif datum == 'MR':
            dx = x - tw
            dy = y - th // 2 - ty_offset
        else:
            dx, dy = x, y - ty_offset

        self.draw.text((dx, dy), text, fill=color, font=font)

    def builtin_text(self, x, y, text, color, datum='TL'):
        """Draw text with the small built-in font."""
        bbox = self.builtin.getbbox(text)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        ty_offset = bbox[1]

        if datum == 'MC':
            dx = x - tw // 2
            dy = y - th // 2 - ty_offset
        elif datum == 'MR':
            dx = x - tw
            dy = y - th // 2 - ty_offset
        else:
            dx, dy = x, y - ty_offset

        self.draw.text((dx, dy), text, fill=color, font=self.builtin)

    def fill_rect(self, x, y, w, h, color):
        self.draw.rectangle([x, y, x+w-1, y+h-1], fill=color)

    def fill_round_rect(self, x, y, w, h, r, color):
        self.draw.rounded_rectangle([x, y, x+w-1, y+h-1], radius=r, fill=color)

    def hline(self, x, y, w, color):
        self.draw.line([(x, y), (x+w-1, y)], fill=color)

    def vline(self, x, y, h, color):
        self.draw.line([(x, y), (x, y+h-1)], fill=color)

    def draw_line(self, x1, y1, x2, y2, color):
        self.draw.line([(x1, y1), (x2, y2)], fill=color)

    def fill_circle(self, cx, cy, r, color):
        self.draw.ellipse([cx-r, cy-r, cx+r, cy+r], fill=color)

    def progress_bar(self, x, y, w, h, pct, fg, bg):
        r = h // 2
        self.fill_round_rect(x, y, w, h, r, bg)
        fw = int(w * min(max(pct, 0), 1))
        if fw > h:
            self.fill_round_rect(x, y, fw, h, r, fg)
        elif fw > 0:
            self.fill_circle(x + r, y + r, r, fg)

    def section_header(self, x, y, label, color):
        # Offset adjusted from firmware y+5 to y+3 to compensate for
        # PIL vs ESP32 VLW font vertical metric differences
        self.fill_rect(x, y+3, 3, 8, color)
        self.text(x+6, y, label, 'sm', color)

    def status_row(self, x, y, w, label, value, val_color):
        self.text(x, y, label, 'sm', CLR_LAVENDER)
        self.text(x+w, y, value, 'sm', val_color, 'TR')

    def label(self, x, y, text_str, color, font_key='sm'):
        self.text(x, y, text_str, font_key, color)

    def cost_value(self, x, y, cost, font_key, color):
        if cost >= 1000:
            s = f"${cost:.0f}"
        else:
            s = f"${cost:.2f}"
        self.text(x, y, s, font_key, color)

    def cost_row(self, x, y, w, label, cost_usd, max_cost, color):
        """Matches firmware drawCostRow: dot + label + cost + progress bar."""
        # Colored dot
        self.fill_circle(x + 2, y + 5, 2, color)
        # Label
        self.text(x + 10, y - 3, label, 'sm', color)
        # Cost value right-aligned
        if cost_usd >= 100.0:
            cost_str = f"${cost_usd:.0f}"
        elif cost_usd >= 10.0:
            cost_str = f"${cost_usd:.1f}"
        else:
            cost_str = f"${cost_usd:.2f}"
        self.text(x + w, y - 3, cost_str, 'sm', CLR_PEACH, 'TR')
        # Progress bar below text
        pct = cost_usd / max_cost if max_cost > 0 else 0
        self.progress_bar(x, y + 17, w, 4, pct, color, CLR_BAR_BG)

    def accept_bar(self, x, y, w, label, accepted, rejected, color):
        total = accepted + rejected
        pct = accepted / total if total > 0 else 0
        self.text(x, y, label, 'sm', CLR_PEACH)
        self.text(x+42, y, f"{int(pct*100)}%", 'sm', CLR_TEXT_BRIGHT)
        self.progress_bar(x+72, y+3, w-72, 6, pct, color, CLR_BAR_BG)

    def signal_bars(self, x, y, rssi):
        bars = 0
        if rssi > -55: bars = 4
        elif rssi > -65: bars = 3
        elif rssi > -75: bars = 2
        elif rssi > -85: bars = 1
        for i in range(4):
            bh = 3 + i * 3
            bx = x + i * 5
            by = y + 12 - bh
            c = CLR_STATUS_OK if i < bars else CLR_TEXT_DIM
            self.fill_rect(bx, by, 3, bh, c)

# ============================================================
# Quarter circle for elbows
# ============================================================
def fill_quarter_circle(scr, cx, cy, r, color, quadrant):
    for y in range(r+1):
        x = int(math.sqrt(r*r - y*y))
        if quadrant == 0:
            scr.hline(cx-x, cy-y, x, color)
        elif quadrant == 1:
            scr.hline(cx, cy-y, x, color)
        elif quadrant == 2:
            scr.hline(cx, cy+y, x, color)
        elif quadrant == 3:
            scr.hline(cx-x, cy+y, x, color)

# ============================================================
# LCARS Frame (matches ui_widgets.cpp drawLcarsFrame)
# ============================================================
def draw_lcars_frame(scr, title, page, total_pages, countdown=285, rssi=-58):
    SW, R, TH, BH = SIDEBAR_W, ELBOW_R, TOPBAR_H, BOTBAR_H

    # ===== TOP-LEFT ELBOW =====
    scr.fill_rect(0, 0, SW, TH + R, CLR_AMBER)
    scr.fill_rect(SW, 0, R, TH, CLR_AMBER)
    fill_quarter_circle(scr, SW, TH, R, CLR_BG, 2)

    # ===== SIDEBAR SEGMENTS =====
    seg_top = TH + R + 2
    seg_bot = SCR_H - BH - R - 2
    seg_h = seg_bot - seg_top
    seg_colors = [CLR_SALMON, CLR_LAVENDER, CLR_BLUE]
    seg_nums = ["01", "02", "03"]
    each_h = (seg_h - 4) // 3

    for i in range(3):
        sy = seg_top + i * (each_h + 2)
        scr.fill_rect(0, sy, SW-4, each_h, seg_colors[i])
        scr.fill_round_rect(SW-8, sy, 8, each_h, 4, seg_colors[i])
        # Segment numbers (small builtin font)
        scr.builtin_text(2, sy + each_h // 2 - 4, seg_nums[i], CLR_BG)

    # ===== BOTTOM-LEFT ELBOW =====
    scr.fill_rect(0, SCR_H-BH-R, SW, BH+R, CLR_TAN)
    scr.fill_rect(SW, SCR_H-BH, R, BH, CLR_TAN)
    fill_quarter_circle(scr, SW, SCR_H-BH, R, CLR_BG, 1)

    # ===== TOP BAR =====
    bar_x = SW + R + 4
    bar_y = 1
    bar_h = TH - 2

    # Title left-aligned, vertically centered
    scr.text(bar_x, bar_y + bar_h//2 - 1, title, 'md', CLR_AMBER, 'ML')

    # Right side: "CLAUDE" text + spark logo
    logo_y = bar_y + bar_h // 2
    logo_x = SCR_W - 10
    # Spark logo
    scr.fill_circle(logo_x, logo_y, 4, CLR_ORANGE)
    scr.fill_rect(logo_x - 1, logo_y - 7, 2, 3, CLR_ORANGE)
    scr.fill_rect(logo_x - 1, logo_y + 5, 2, 3, CLR_ORANGE)
    scr.fill_rect(logo_x - 7, logo_y - 1, 3, 2, CLR_ORANGE)
    scr.fill_rect(logo_x + 5, logo_y - 1, 3, 2, CLR_ORANGE)
    scr.draw_line(logo_x - 5, logo_y - 5, logo_x - 3, logo_y - 3, CLR_ORANGE)
    scr.draw_line(logo_x + 3, logo_y - 3, logo_x + 5, logo_y - 5, CLR_ORANGE)
    scr.draw_line(logo_x - 5, logo_y + 5, logo_x - 3, logo_y + 3, CLR_ORANGE)
    scr.draw_line(logo_x + 3, logo_y + 3, logo_x + 5, logo_y + 5, CLR_ORANGE)
    # "CLAUDE" text to the left of logo
    scr.text(logo_x - 16, logo_y - 1, "CLAUDE", 'md', CLR_PEACH, 'MR')

    # ===== BOTTOM BAR =====
    bar_x = SW + R + 4
    bar_y = SCR_H - BH + 1
    bar_h = BH - 2

    # Page chip
    page_str = f"{page+1}/{total_pages}"
    scr.fill_round_rect(bar_x, bar_y, 28, bar_h, bar_h//2, CLR_PEACH)
    scr.fill_rect(bar_x, bar_y, bar_h//2, bar_h, CLR_PEACH)  # square left edge
    scr.builtin_text(bar_x + 14, bar_y + bar_h // 2, page_str, CLR_BG, 'MC')

    # Signal indicator position (left of countdown pill)
    sig_x = SCR_W - 120 - 4
    pill_end = sig_x - 70

    # Decorative pills
    mid_x = bar_x + 34
    lav_w = min(40, pill_end - mid_x - 24)
    if lav_w > 8:
        scr.fill_round_rect(mid_x, bar_y, lav_w, bar_h, bar_h//2, CLR_LAVENDER)
        scr.fill_rect(mid_x + lav_w - bar_h//2, bar_y, bar_h//2, bar_h, CLR_LAVENDER)
        mid_x += lav_w + 4
        sal_w = min(20, pill_end - mid_x)
        if sal_w > 8:
            scr.fill_round_rect(mid_x, bar_y, sal_w, bar_h, bar_h//2, CLR_SALMON)
            scr.fill_rect(mid_x, bar_y, bar_h//2, bar_h, CLR_SALMON)

    # Signal bars + "SIGNAL" text
    scr.signal_bars(sig_x - 20, bar_y + 1, rssi)
    scr.builtin_text(sig_x - 24, bar_y + bar_h // 2, "SIGNAL", CLR_TEXT_DIM, 'MR')

    # Countdown pill
    if countdown >= 60:
        cd_str = f"NEXT REFRESH {countdown // 60}m{countdown % 60:02d}s"
    else:
        cd_str = f"NEXT REFRESH {countdown}s"
    cd_w = 120
    scr.fill_round_rect(SCR_W - cd_w, bar_y, cd_w, bar_h, bar_h//2, CLR_AMBER)
    scr.fill_rect(SCR_W - bar_h//2, bar_y, bar_h//2, bar_h, CLR_AMBER)  # square right edge
    scr.builtin_text(SCR_W - cd_w // 2, bar_y + bar_h // 2, cd_str, CLR_BG, 'MC')

def format_count(value):
    if value >= 1_000_000_000: return f"{value/1e9:.1f}B"
    if value >= 1_000_000: return f"{value/1e6:.1f}M"
    if value >= 1_000: return f"{value/1e3:.1f}K"
    return str(value)

def shorten_model_name(full_name):
    """Matches firmware shortenModelName: 'claude-sonnet-4-20250514' -> 'Sonnet 4'"""
    name = full_name.replace("claude-", "")
    # Remove date suffix
    parts = name.split("-")
    if len(parts) > 1 and len(parts[-1]) == 8 and parts[-1].isdigit():
        parts = parts[:-1]
    name = "-".join(parts)
    # Capitalize first letter
    if name:
        name = name[0].upper() + name[1:]
    # Replace hyphens with spaces for readability
    name = name.replace("-", " ")
    if len(name) > 12:
        name = name[:10] + ".."
    return name

# ============================================================
# Screen 1: USAGE OVERVIEW
# ============================================================
def draw_overview(fonts):
    scr = Screen(fonts)
    draw_lcars_frame(scr, "USAGE OVERVIEW", 0, TOTAL_SCREENS)

    # === Left side: Cost display ===
    y = CY - 4

    scr.label(CX, y, "TODAY", CLR_AMBER)
    y += H_SM - 4

    scr.cost_value(CX, y, 0.00, 'xl', CLR_PEACH)
    y += H_XL + 6

    scr.label(CX, y, "THIS MONTH", CLR_AMBER)
    y += H_SM

    scr.cost_value(CX, y, 51.27, 'lg', CLR_SKY)

    # === Vertical divider ===
    div_x = CX + CW//2 - 4
    scr.vline(div_x, CY, CH, CLR_AMBER)
    scr.vline(div_x+1, CY, CH, CLR_AMBER)

    # === Right side: Cost Breakdown ===
    tok_x = div_x + 8
    tok_w = CW - (tok_x - CX) - 2
    y = CY - 4

    scr.label(tok_x, y, "COST BREAKDOWN", CLR_AMBER)
    y += H_SM + 2

    cost_colors = [CLR_AMBER, CLR_LAVENDER, CLR_BLUE, CLR_SALMON]
    models = [
        ("claude-opus-4", 2.71),
        ("claude-sonnet-4", 1.36),
        ("claude-haiku-3.5", 0.45),
    ]
    max_cost = max(m[1] for m in models) if models else 0.01

    for i, (name, cost) in enumerate(models):
        short = shorten_model_name(name)
        scr.cost_row(tok_x, y, tok_w, short, cost, max_cost, cost_colors[i % 4])
        y += H_TOK

    return scr.img

# ============================================================
# Screen 2: TODAY MODEL ANALYSIS
# ============================================================
def draw_models(fonts, monthly=False):
    scr = Screen(fonts)
    if monthly:
        draw_lcars_frame(scr, "MONTHLY MODELS", 2, TOTAL_SCREENS)
        mult = 15
    else:
        draw_lcars_frame(scr, "TODAY MODEL ANALYSIS", 1, TOTAL_SCREENS)
        mult = 1

    models = [
        ("opus-4", 250000 * mult, 80000 * mult, CLR_AMBER),
        ("sonnet-4", 180000 * mult, 60000 * mult, CLR_LAVENDER),
        ("haiku-3.5", 50000 * mult, 20000 * mult, CLR_BLUE),
    ]
    max_total = max(i+o for _, i, o, _ in models)

    y = CY - 4
    for name, inp, out, color in models:
        # Colored indicator rect
        scr.fill_rect(CX, y+4, 3, 8, color)
        scr.text(CX+6, y, name, 'sm', color)
        scr.text(CX+CW, y, f"IN:{format_count(inp)} OUT:{format_count(out)}", 'sm', CLR_PEACH, 'TR')
        y += H_SM
        pct = (inp+out) / max_total
        scr.progress_bar(CX, y, CW, 4, pct, color, CLR_BAR_BG)
        y += 8

    return scr.img

# ============================================================
# Screen 3/5: CODE ANALYTICS (today or monthly)
# ============================================================
def draw_code(fonts, monthly=False):
    scr = Screen(fonts)
    if monthly:
        draw_lcars_frame(scr, "MONTHLY CODE", 4, TOTAL_SCREENS)
        mult = 4
    else:
        draw_lcars_frame(scr, "TODAY CODE ANALYTICS", 3, TOTAL_SCREENS)
        mult = 1

    left_x = CX
    left_w = CW//2 - 8
    y = CY - 4

    sessions = 12 * mult
    lines_add = 340 * mult
    lines_rem = 120 * mult
    commits = 8 * mult
    prs = 3 * mult
    est_cost = 4.52 * mult

    scr.section_header(left_x, y, "SESSIONS", CLR_AMBER)
    scr.text(left_x+left_w, y, str(sessions), 'md', CLR_TEXT_BRIGHT, 'TR')
    y += H_MD

    scr.section_header(left_x, y, "LINES", CLR_AMBER)
    y += H_SM
    scr.text(left_x+left_w, y, f"+{lines_add} / -{lines_rem}", 'sm', CLR_SKY, 'TR')
    y += H_SM + 2

    scr.status_row(left_x, y, left_w, "COMMITS", str(commits), CLR_PEACH)
    y += H_SM + 2
    scr.status_row(left_x, y, left_w, "PULL REQS", str(prs), CLR_PEACH)
    y += H_SM + 2
    scr.label(left_x, y, "EST. COST", CLR_AMBER)
    scr.text(left_x+left_w, y, f"${est_cost:.2f}", 'sm', CLR_SKY, 'TR')

    # Divider
    div_x = CX + CW//2 - 2
    scr.vline(div_x, CY, CH, CLR_AMBER)

    # Right column
    rx = div_x + 6
    rw = CW - (rx - CX) - 2
    ry = CY - 4

    scr.section_header(rx, ry, "TOOL ACCEPTANCE", CLR_AMBER)
    ry += H_SM + 2
    scr.accept_bar(rx, ry, rw, "EDIT", 85, 15, CLR_SKY)
    ry += H_SM + 2
    scr.accept_bar(rx, ry, rw, "WRITE", 70, 30, CLR_LAVENDER)
    ry += H_SM + 4

    scr.hline(rx, ry, rw, CLR_TEXT_DIM)
    ry += 16
    scr.section_header(rx, ry, "USERS", CLR_AMBER)
    ry += H_SM + 2
    scr.status_row(rx, ry, rw, "user@exam...", f"${est_cost:.2f}", CLR_PEACH)

    return scr.img

# ============================================================
# Screen 6: SYSTEM STATUS
# ============================================================
def draw_status(fonts):
    scr = Screen(fonts)
    draw_lcars_frame(scr, "SYSTEM STATUS", 5, TOTAL_SCREENS)

    left_x = CX
    left_w = CW//2 - 8
    y = CY - 4

    scr.section_header(left_x, y, "NETWORK", CLR_AMBER)
    y += H_SM
    scr.status_row(left_x, y, left_w, "WiFi", "CONNECTED", CLR_STATUS_OK)
    y += H_SM
    scr.status_row(left_x, y, left_w, "IP", "192.168.1.36", CLR_PEACH)
    y += H_SM
    scr.status_row(left_x, y, left_w-22, "Signal", "-58 dBm", CLR_PEACH)
    scr.signal_bars(left_x+left_w-18, y+1, -58)
    y += H_SM + 6

    scr.section_header(left_x, y, "API", CLR_LAVENDER)
    y += H_SM
    scr.status_row(left_x, y, left_w, "Status", "OK", CLR_STATUS_OK)
    y += H_SM
    scr.status_row(left_x, y, left_w, "Fetched", "2m ago", CLR_PEACH)

    # Divider
    div_x = CX + CW//2 - 2
    scr.vline(div_x, CY, CH, CLR_AMBER)

    # Right column
    rx = div_x + 6
    rw = CW - (rx - CX) - 2
    ry = CY - 4

    scr.section_header(rx, ry, "SYSTEM", CLR_BLUE)
    ry += H_SM
    scr.status_row(rx, ry, rw, "Heap", "201 KB", CLR_PEACH)
    ry += H_SM
    scr.status_row(rx, ry, rw, "PSRAM", "8068 KB", CLR_PEACH)
    ry += H_SM
    scr.status_row(rx, ry, rw, "Uptime", "0h 8m", CLR_PEACH)
    ry += H_SM + 6

    scr.section_header(rx, ry, "FIRMWARE", CLR_SALMON)
    ry += H_SM
    scr.label(rx, ry, "LCARS v2.0", CLR_TEXT_DIM)
    ry += H_SM
    scr.label(rx, ry, "T-Display-S3", CLR_TEXT_DIM)

    return scr.img

# ============================================================
# V2 Helpers
# ============================================================
CLR_ICE        = (153, 204, 255)
CLR_TOMATO     = (204, 68, 68)
CLR_BAR_TRACK  = (26, 26, 26)
CLR_GOLD       = CLR_AMBER

def draw_donut_gauge(scr, cx, cy, r_outer, thickness, pct, fg, bg):
    """Draw a donut gauge arc from 12 o'clock, clockwise."""
    # Background ring
    for angle_deg in range(360):
        a = math.radians(angle_deg - 90)
        for t in range(thickness):
            ri = r_outer - t
            px = int(cx + ri * math.cos(a))
            py = int(cy + ri * math.sin(a))
            if 0 <= px < SCR_W and 0 <= py < SCR_H:
                scr.img.putpixel((px, py), bg)
    # Foreground arc
    fill_deg = int(360 * min(max(pct, 0), 1))
    for angle_deg in range(fill_deg):
        a = math.radians(angle_deg - 90)
        for t in range(thickness):
            ri = r_outer - t
            px = int(cx + ri * math.cos(a))
            py = int(cy + ri * math.sin(a))
            if 0 <= px < SCR_W and 0 <= py < SCR_H:
                scr.img.putpixel((px, py), fg)

def draw_vertical_bar(scr, x, y, w, h, pct, fg, bg):
    """Draw a vertical bar that fills bottom-up."""
    scr.fill_rect(x, y, w, h, bg)
    fill_h = int(h * min(max(pct, 0), 1))
    if fill_h > 0:
        scr.fill_rect(x, y + h - fill_h, w, fill_h, fg)

# ============================================================
# V2 Screen 1: CLAUDE.AI (subscription donut gauges)
# ============================================================
def draw_v2_claudeai(fonts):
    scr = Screen(fonts)
    draw_lcars_frame(scr, "CLAUDE.AI", 0, 2)

    pct5h = 0.32
    pct7d = 0.18
    color5h = CLR_ICE  # <50% = ICE
    color7d = CLR_LAVENDER  # <50% = LAVENDER

    # ── LEFT: 5-hour limit ──
    # Countdown clock
    scr.text(92, 22, "2:22:00", 'lg', CLR_TEXT_BRIGHT, 'MC')

    # Donut gauge
    draw_donut_gauge(scr, 95, 103, 40, 10, pct5h, color5h, CLR_BAR_TRACK)
    # Percentage text centered in donut
    scr.text(95, 101, "32%", 'md', CLR_TEXT_BRIGHT, 'MC')

    # Vertical countdown bar
    draw_vertical_bar(scr, 149, 24, 10, 120, 0.65, color5h, CLR_BAR_TRACK)

    # ── Vertical divider ──
    scr.fill_rect(171, 24, 2, 120, CLR_AMBER)

    # ── RIGHT: 7-day limit ──
    scr.text(239, 22, "6d 23h", 'lg', CLR_PEACH, 'MC')

    draw_donut_gauge(scr, 244, 104, 40, 10, pct7d, color7d, CLR_BAR_TRACK)
    scr.text(244, 102, "18%", 'md', CLR_TEXT_BRIGHT, 'MC')

    draw_vertical_bar(scr, 304, 24, 10, 120, 0.85, color7d, CLR_BAR_TRACK)

    return scr.img

# ============================================================
# V2 Screen 2: SYSTEM STATUS
# ============================================================
def draw_v2_status(fonts):
    scr = Screen(fonts)
    draw_lcars_frame(scr, "SYSTEM STATUS", 1, 2)

    x = 54
    w = 264
    y = 22

    # Status rows (label left, value right-aligned)
    rows = [
        ("WIFI", "CONNECTED", CLR_ICE),
        ("SIGNAL", "-45 DBM", CLR_ICE),
        ("IP", "192.168.1.36", CLR_AMBER),
        ("UPTIME", "2H 14M", CLR_AMBER),
        ("REFRESH IN", "45s", CLR_AMBER),
    ]
    for label, value, color in rows:
        scr.text(x, y, label, 'sm', CLR_LAVENDER)
        scr.text(x + w, y, value, 'sm', color, 'TR')
        y += 15

    # Firmware version
    scr.text(x, y, "FIRMWARE", 'sm', CLR_LAVENDER)
    scr.text(x + w, y, "LCARS v2.0", 'sm', CLR_TEXT_DIM, 'TR')
    y += 17

    # Separator line
    scr.hline(x, y, w, CLR_TEXT_DIM)
    y += 3

    # Data cascade (decorative animated bars)
    cascade_h = 37
    import random
    random.seed(42)  # Deterministic for consistent previews
    bar_y = y
    bar_x = x
    for i in range(30):
        bw = random.randint(8, 60)
        bh = 2
        bx = bar_x + random.randint(0, w - 60)
        by = bar_y + (i * (cascade_h // 30))
        alpha = 1.0 - (i / 30)
        r = int(CLR_AMBER[0] * alpha)
        g = int(CLR_AMBER[1] * alpha)
        b = int(CLR_AMBER[2] * alpha)
        if r > 20:
            scr.fill_rect(bx, by, bw, bh, (r, g, b))

    return scr.img

# ============================================================
# Main: render all screens, save as PNG, show combined
# ============================================================
def main():
    fonts = load_fonts()
    out_dir = os.path.dirname(__file__)

    # Firmware v1 screens (6 screens)
    v1_screens = [
        ("v1_1_overview",       lambda: draw_overview(fonts)),
        ("v1_2_models_today",   lambda: draw_models(fonts, monthly=False)),
        ("v1_3_models_monthly", lambda: draw_models(fonts, monthly=True)),
        ("v1_4_code_today",     lambda: draw_code(fonts, monthly=False)),
        ("v1_5_code_monthly",   lambda: draw_code(fonts, monthly=True)),
        ("v1_6_status",         lambda: draw_status(fonts)),
    ]

    # Firmware v2 screens (2 screens)
    v2_screens = [
        ("v2_1_claudeai",  lambda: draw_v2_claudeai(fonts)),
        ("v2_2_status",    lambda: draw_v2_status(fonts)),
    ]

    all_screens = v1_screens + v2_screens
    v1_images = []
    v2_images = []

    for name, draw_fn in all_screens:
        img = draw_fn()
        path = os.path.join(out_dir, f"preview_{name}.png")
        scaled = img.resize((SCR_W*3, SCR_H*3), Image.NEAREST)
        scaled.save(path)
        if name.startswith("v1_"):
            v1_images.append(img)
        else:
            v2_images.append(img)
        print(f"  Saved {path}")

    # ── Combined grid: v1 (3x2) on top, v2 (2x1) on bottom ──
    gap = 4
    label_h = 20  # Height for version labels

    # V1 grid: 3 columns x 2 rows
    v1_cols, v1_rows = 3, 2
    v1_grid_w = SCR_W * v1_cols + gap * (v1_cols - 1)
    v1_grid_h = SCR_H * v1_rows + gap * (v1_rows - 1)

    # V2 grid: 2 columns x 1 row
    v2_cols = 2
    v2_grid_w = SCR_W * v2_cols + gap * (v2_cols - 1)
    v2_grid_h = SCR_H

    # Total canvas
    total_w = max(v1_grid_w, v2_grid_w)
    total_h = label_h + v1_grid_h + gap + label_h + v2_grid_h
    grid = Image.new('RGB', (total_w, total_h), (30, 30, 30))
    draw_grid = ImageDraw.Draw(grid)

    # Load label font
    try:
        label_font = ImageFont.truetype(FONT_PATH, 14)
    except:
        label_font = ImageFont.load_default()

    # V1 label
    draw_grid.text((4, 2), "FIRMWARE V1 — 7 DASHBOARD SCREENS", fill=CLR_AMBER, font=label_font)
    y_offset = label_h
    for idx, img in enumerate(v1_images):
        col = idx % v1_cols
        row = idx // v1_cols
        x = col * (SCR_W + gap)
        y = y_offset + row * (SCR_H + gap)
        grid.paste(img, (x, y))

    # V2 label
    v2_label_y = y_offset + v1_grid_h + gap
    draw_grid.text((4, v2_label_y + 2), "FIRMWARE V2 — CLAUDE.AI FOCUSED", fill=CLR_LAVENDER, font=label_font)
    v2_y_offset = v2_label_y + label_h
    for idx, img in enumerate(v2_images):
        x = idx * (SCR_W + gap)
        grid.paste(img, (x, v2_y_offset))

    grid_path = os.path.join(out_dir, "preview_all.png")
    grid_scaled = grid.resize((total_w * 2, total_h * 2), Image.NEAREST)
    grid_scaled.save(grid_path)
    print(f"  Saved {grid_path} (combined grid)")

    print("\nDone! Open the PNG files to preview layout.")
    print("Edit this script, re-run to iterate.")

if __name__ == "__main__":
    main()

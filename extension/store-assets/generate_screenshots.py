"""Generate Chrome Web Store screenshots with proper key icon (no broken ◈)."""
from PIL import Image, ImageDraw, ImageFont
import os

W, H = 1280, 800
BG = (13, 17, 23)         # #0d1117
CARD_BG = (22, 27, 34)    # #161b22
ORANGE = (255, 153, 68)   # #ff9944
GREEN = (0, 255, 136)     # #00ff88
TEXT = (230, 237, 243)     # #e6edf3
DIM = (139, 148, 158)     # #8b949e
DARK = (48, 54, 61)       # #30363d
DARKER = (33, 38, 45)     # #21262d
POPUP_BG = (26, 26, 46)   # #1a1a2e
STATUS_BG = (26, 58, 42)  # #1a3a2a
STATUS_TEXT = (127, 223, 127) # #7fdf7f
KEY_BG = (13, 13, 26)     # #0d0d1a
HINT_DIM = (102, 102, 102)
RED = (248, 81, 73)       # #f85149

TOOLBAR_H = 48
BORDER_R = 12

def get_font(size, bold=False):
    """Get a suitable font."""
    candidates = [
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
    ]
    if bold:
        for f in ["C:/Windows/Fonts/segoeuib.ttf", "C:/Windows/Fonts/arialbd.ttf"]:
            if os.path.exists(f):
                return ImageFont.truetype(f, size)
    for f in candidates:
        if os.path.exists(f):
            return ImageFont.truetype(f, size)
    return ImageFont.load_default()

def get_mono_font(size):
    """Get a monospace font."""
    for f in ["C:/Windows/Fonts/consola.ttf", "C:/Windows/Fonts/cour.ttf"]:
        if os.path.exists(f):
            return ImageFont.truetype(f, size)
    return ImageFont.load_default()

def draw_rounded_rect(draw, xy, fill, radius=8, outline=None):
    """Draw a rounded rectangle."""
    x1, y1, x2, y2 = xy
    draw.rounded_rectangle(xy, radius=radius, fill=fill, outline=outline)

def draw_key_icon(draw, cx, cy, size, color):
    """Draw a simple key icon."""
    s = size
    # Key head (circle with hole)
    head_r = s * 0.3
    draw.ellipse(
        [cx - head_r, cy - head_r, cx + head_r, cy + head_r],
        outline=color, width=max(2, int(s * 0.08))
    )
    # Inner hole
    hole_r = head_r * 0.35
    draw.ellipse(
        [cx - hole_r, cy - hole_r, cx + hole_r, cy + hole_r],
        outline=color, width=max(1, int(s * 0.06))
    )
    # Key shaft
    shaft_start_x = cx + head_r - 2
    shaft_end_x = cx + s * 0.55
    shaft_y = cy
    draw.line(
        [(shaft_start_x, shaft_y), (shaft_end_x, shaft_y)],
        fill=color, width=max(2, int(s * 0.08))
    )
    # Key teeth
    tooth_len = s * 0.15
    for i, tx in enumerate([shaft_end_x - s * 0.12, shaft_end_x]):
        draw.line(
            [(tx, shaft_y), (tx, shaft_y + tooth_len)],
            fill=color, width=max(2, int(s * 0.07))
        )

def draw_toolbar(draw, url_text, width):
    """Draw a browser toolbar."""
    draw.rectangle([0, 0, width, TOOLBAR_H], fill=(30, 30, 40))
    draw.line([(0, TOOLBAR_H), (width, TOOLBAR_H)], fill=DARK, width=1)
    font = get_font(14)
    draw.text((20, TOOLBAR_H // 2 - 7), url_text, fill=DIM, font=font)

def draw_circle(draw, cx, cy, r, fill=None, outline=None, width=1):
    """Draw a circle."""
    draw.ellipse([cx - r, cy - r, cx + r, cy + r], fill=fill, outline=outline, width=width)

def draw_checkmark(draw, cx, cy, size, color):
    """Draw a checkmark."""
    s = size * 0.4
    pts = [
        (cx - s * 0.6, cy),
        (cx - s * 0.1, cy + s * 0.5),
        (cx + s * 0.7, cy - s * 0.4),
    ]
    draw.line(pts, fill=color, width=max(2, int(size * 0.12)))


# ============================================================
# Screenshot 1: Extension popup on claude.ai
# ============================================================
def generate_screenshot1():
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)

    # Browser toolbar
    draw_toolbar(draw, "claude.ai", W)

    # Background content (simulating claude.ai)
    font_lg = get_font(28)
    font_md = get_font(16)
    draw.text((70, 250), "You're logged into claude.ai", fill=DIM, font=font_lg)
    draw.text((70, 300), "Click the extension icon to get", fill=DIM, font=font_md)
    draw.text((70, 324), "your session key with one click.", fill=DIM, font=font_md)

    # Extension popup (top right)
    popup_x = 780
    popup_y = 68
    popup_w = 380
    popup_h = 340

    # Draw the triangle pointer from toolbar to popup
    tri_x = popup_x + popup_w // 2 + 40
    draw.polygon(
        [(tri_x, popup_y - 15), (tri_x - 10, popup_y), (tri_x + 10, popup_y)],
        fill=POPUP_BG
    )
    # Draw a line from toolbar to triangle
    draw.line([(tri_x + 30, TOOLBAR_H), (tri_x, popup_y - 15)], fill=ORANGE, width=2)

    # Popup background
    draw_rounded_rect(draw, (popup_x, popup_y, popup_x + popup_w, popup_y + popup_h),
                      fill=POPUP_BG, radius=10, outline=(60, 60, 80))

    px, py = popup_x + 24, popup_y + 22

    # Key icon + title
    font_title = get_font(17, bold=True)
    draw_key_icon(draw, px + 10, py + 8, 22, ORANGE)
    draw.text((px + 32, py - 2), "Claude Session Key", fill=ORANGE, font=font_title)
    py += 36

    # Status: "Session key found!"
    status_y = py
    draw_rounded_rect(draw, (px, status_y, px + popup_w - 48, status_y + 34),
                      fill=STATUS_BG, radius=6)
    font_status = get_font(13)
    draw.text((px + 12, status_y + 9), "Session key found!", fill=STATUS_TEXT, font=font_status)
    py += 48

    # Key display box
    key_y = py
    draw_rounded_rect(draw, (px, key_y, px + popup_w - 48, key_y + 50),
                      fill=KEY_BG, radius=6, outline=(51, 51, 51))
    font_mono = get_mono_font(12)
    draw.text((px + 12, key_y + 10), "sk-ant-sid01-abc123def456...", fill=(170, 170, 170), font=font_mono)
    draw.text((px + 12, key_y + 28), "ghi789jkl012mno345pqr678", fill=(170, 170, 170), font=font_mono)
    py += 64

    # Copy button
    btn_y = py
    draw_rounded_rect(draw, (px, btn_y, px + popup_w - 48, btn_y + 40),
                      fill=ORANGE, radius=6)
    font_btn = get_font(15, bold=True)
    btn_text = "Copy to Clipboard"
    bbox = draw.textbbox((0, 0), btn_text, font=font_btn)
    tw = bbox[2] - bbox[0]
    draw.text((px + (popup_w - 48 - tw) // 2, btn_y + 10), btn_text, fill=(13, 13, 26), font=font_btn)
    py += 54

    # Hint text
    font_hint = get_font(12)
    draw.text((px, py), "Next: Open your device's web portal", fill=HINT_DIM, font=font_hint)
    draw.text((px, py + 18), "and paste this key into the Session", fill=HINT_DIM, font=font_hint)
    draw.text((px, py + 36), "Key field.", fill=HINT_DIM, font=font_hint)

    img.save(os.path.join(os.path.dirname(__file__), "screenshot1.png"), "PNG")
    print("screenshot1.png saved")


# ============================================================
# Screenshot 2: Device config page with auto-fill
# ============================================================
def generate_screenshot2():
    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)

    # Browser toolbar
    draw_toolbar(draw, "http://192.168.1.72", W)

    # Main card
    card_x = (W - 480) // 2
    card_y = TOOLBAR_H + 30
    card_w = 480
    card_h = H - TOOLBAR_H - 60
    draw_rounded_rect(draw, (card_x, card_y, card_x + card_w, card_y + card_h),
                      fill=CARD_BG, radius=12, outline=DARK)

    cx = card_x + 32  # content left
    cw = card_w - 64  # content width
    cy = card_y + 28  # current y

    # Title: Key icon + "ClaudeGauge"
    font_title = get_font(20, bold=True)
    title_text = "ClaudeGauge"
    bbox = draw.textbbox((0, 0), title_text, font=font_title)
    title_w = bbox[2] - bbox[0]
    title_x = card_x + (card_w - title_w - 30) // 2
    draw_key_icon(draw, title_x, cy + 10, 24, ORANGE)
    draw.text((title_x + 22, cy), title_text, fill=ORANGE, font=font_title)
    cy += 26

    # Subtitle
    font_sub = get_font(13)
    sub_text = "Configuration Portal"
    bbox = draw.textbbox((0, 0), sub_text, font=font_sub)
    sub_w = bbox[2] - bbox[0]
    draw.text((card_x + (card_w - sub_w) // 2, cy), sub_text, fill=DIM, font=font_sub)
    cy += 28

    # Progress bar: WiFi ✓ --- API Key ✓ --- Session (3)
    dot_r = 16
    progress_cx = card_x + card_w // 2
    dot_positions = [progress_cx - 100, progress_cx, progress_cx + 100]
    dot_labels = ["WiFi", "API Key", "Session"]

    # Lines
    draw.line([(dot_positions[0] + dot_r, cy + dot_r),
               (dot_positions[1] - dot_r, cy + dot_r)], fill=GREEN, width=2)
    draw.line([(dot_positions[1] + dot_r, cy + dot_r),
               (dot_positions[2] - dot_r, cy + dot_r)], fill=GREEN, width=2)

    # Dots
    for i, dx in enumerate(dot_positions):
        if i < 2:  # completed
            draw_circle(draw, dx, cy + dot_r, dot_r, fill=GREEN)
            draw_checkmark(draw, dx, cy + dot_r, dot_r * 2, (13, 17, 23))
        else:  # current
            draw_circle(draw, dx, cy + dot_r, dot_r, fill=BG, outline=ORANGE, width=2)
            font_dot = get_font(13, bold=True)
            draw.text((dx - 4, cy + dot_r - 8), "3", fill=ORANGE, font=font_dot)

    cy += dot_r * 2 + 8

    # Step labels
    font_label = get_font(11)
    for i, (dx, label) in enumerate(zip(dot_positions, dot_labels)):
        color = GREEN if i < 2 else ORANGE
        bbox = draw.textbbox((0, 0), label, font=font_label)
        lw = bbox[2] - bbox[0]
        draw.text((dx - lw // 2, cy), label, fill=color, font=font_label)

    cy += 30

    # Extension callout card
    ext_x = cx
    ext_y = cy
    ext_h = 160
    draw_rounded_rect(draw, (ext_x, ext_y, ext_x + cw, ext_y + ext_h),
                      fill=(40, 30, 15), radius=8, outline=(255, 153, 68, 100))
    # Dashed border effect - just use solid with alpha
    draw_rounded_rect(draw, (ext_x, ext_y, ext_x + cw, ext_y + ext_h),
                      fill=None, radius=8, outline=ORANGE)

    ey = ext_y + 14

    # Extension title
    font_ext_title = get_font(15, bold=True)
    draw.text((ext_x + 14, ey), "Recommended: Browser Extension", fill=ORANGE, font=font_ext_title)
    # Puzzle piece emoji before title
    draw.text((ext_x + 14 - 2, ey - 1), "\U0001F50C", fill=ORANGE, font=get_font(12))
    ey += 24

    # Extension description
    font_ext = get_font(12)
    draw.text((ext_x + 14, ey), "Install our Chrome extension for one-click", fill=DIM, font=font_ext)
    ey += 18
    draw.text((ext_x + 14, ey), "session key setup. No developer tools needed.", fill=DIM, font=font_ext)
    ey += 28

    # Auto-fill button (injected by extension)
    btn_y = ey
    draw_rounded_rect(draw, (ext_x + 14, btn_y, ext_x + cw - 14, btn_y + 38),
                      fill=ORANGE, radius=6)
    font_btn = get_font(15, bold=True)
    btn_text = "Auto-fill from Claude.ai"
    bbox = draw.textbbox((0, 0), btn_text, font=font_btn)
    tw = bbox[2] - bbox[0]
    draw.text((ext_x + 14 + (cw - 28 - tw) // 2, btn_y + 9), btn_text, fill=(13, 17, 23), font=font_btn)
    ey += 48

    # Hint under button
    font_tiny = get_font(11)
    draw.text((ext_x + 14, ey), "\u2139 This button appears when extension is installed",
              fill=GREEN, font=font_tiny)

    cy = ext_y + ext_h + 18

    # Session Key label
    font_lbl = get_font(13)
    draw.text((cx, cy), "Session Key", fill=DIM, font=font_lbl)
    cy += 22

    # Input field with value
    input_h = 38
    draw_rounded_rect(draw, (cx, cy, cx + cw, cy + input_h),
                      fill=(22, 27, 34), radius=6, outline=DARK)
    font_input = get_font(14)
    # Show partially masked key
    draw.text((cx + 12, cy + 10), "sk-ant-sid01-", fill=TEXT, font=font_input)
    # Dots for masked part
    draw.text((cx + 130, cy + 10), "\u2022" * 20, fill=TEXT, font=font_input)
    cy += input_h + 14

    # Save button
    save_h = 38
    draw_rounded_rect(draw, (cx, cy, cx + cw, cy + save_h),
                      fill=ORANGE, radius=6)
    font_save = get_font(15, bold=True)
    save_text = "Save Session & Restart"
    bbox = draw.textbbox((0, 0), save_text, font=font_save)
    tw = bbox[2] - bbox[0]
    draw.text((cx + (cw - tw) // 2, cy + 9), save_text, fill=(13, 17, 23), font=font_save)

    img.save(os.path.join(os.path.dirname(__file__), "screenshot2.png"), "PNG")
    print("screenshot2.png saved")


if __name__ == "__main__":
    generate_screenshot1()
    generate_screenshot2()

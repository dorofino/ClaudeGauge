#!/usr/bin/env python3
"""
Convert TTF font to Adafruit GFX compatible C header for TFT_eSPI.
Generates bitmap font data for embedding in firmware.
"""
import sys
import struct
from PIL import Image, ImageDraw, ImageFont

def render_glyph(font, char):
    """Render a single glyph and return its bitmap and metrics."""
    # Get the bounding box for this character
    bbox = font.getbbox(char)
    if bbox is None:
        return None

    x0, y0, x1, y1 = bbox
    w = x1 - x0
    h = y1 - y0

    if w == 0 or h == 0:
        return None

    # Create image and draw the character
    img = Image.new('L', (w + 4, h + 4), 0)
    draw = ImageDraw.Draw(img)
    draw.text((-x0 + 2, -y0 + 2), char, fill=255, font=font)

    # Crop to actual content
    bbox2 = img.getbbox()
    if bbox2 is None:
        return None

    img = img.crop(bbox2)
    w, h = img.size

    # Convert to 1-bit bitmap (packed bytes)
    bitmap = []
    for row in range(h):
        for col in range(w):
            pixel = img.getpixel((col, row))
            bit_index = len(bitmap) * 8 + 0  # not used directly
            if col % 8 == 0:
                if col > 0 or row > 0:
                    pass
            # Pack bits
            pass

    # Better approach: pack into bytes MSB first
    bits = []
    for row in range(h):
        for col in range(w):
            pixel = img.getpixel((col, row))
            bits.append(1 if pixel > 127 else 0)

    # Pad to byte boundary
    while len(bits) % 8 != 0:
        bits.append(0)

    bitmap_bytes = []
    for i in range(0, len(bits), 8):
        byte = 0
        for j in range(8):
            if i + j < len(bits):
                byte |= (bits[i + j] << (7 - j))
        bitmap_bytes.append(byte)

    # Get advance width
    advance = font.getlength(char)

    return {
        'width': w,
        'height': h,
        'xOffset': bbox2[0] - 2 + x0,
        'yOffset': bbox2[1] - 2 + y0,
        'xAdvance': int(advance),
        'bitmap': bitmap_bytes
    }


def generate_font_header(ttf_path, size, name, output_path):
    """Generate a C header file with Adafruit GFX font data."""
    font = ImageFont.truetype(ttf_path, size)

    first_char = 32  # space
    last_char = 126  # ~

    glyphs = {}
    all_bitmap = []
    glyph_data = []

    for code in range(first_char, last_char + 1):
        char = chr(code)
        g = render_glyph(font, char)

        if g is None:
            # Empty glyph (like space)
            advance = int(font.getlength(char))
            glyph_data.append({
                'bitmapOffset': len(all_bitmap),
                'width': 0,
                'height': 0,
                'xAdvance': max(advance, size // 4),
                'xOffset': 0,
                'yOffset': 0
            })
        else:
            glyph_data.append({
                'bitmapOffset': len(all_bitmap),
                'width': g['width'],
                'height': g['height'],
                'xAdvance': g['xAdvance'],
                'xOffset': g['xOffset'],
                'yOffset': g['yOffset']
            })
            all_bitmap.extend(g['bitmap'])

    # Calculate yAdvance (line height)
    ascent = 0
    descent = 0
    for code in range(first_char, last_char + 1):
        idx = code - first_char
        gd = glyph_data[idx]
        if gd['height'] > 0:
            top = gd['yOffset']
            bottom = gd['yOffset'] + gd['height']
            ascent = min(ascent, top)
            descent = max(descent, bottom)
    y_advance = descent - ascent + 1

    # Write C header
    with open(output_path, 'w') as f:
        f.write(f"// LCARS font: {name} ({size}px)\n")
        f.write(f"// Generated from {ttf_path}\n")
        f.write(f"// Characters {first_char}-{last_char}\n\n")
        f.write("#include <Adafruit_GFX.h>\n\n")

        # Bitmap array
        f.write(f"const uint8_t {name}Bitmaps[] PROGMEM = {{\n")
        for i, b in enumerate(all_bitmap):
            if i % 16 == 0:
                f.write("  ")
            f.write(f"0x{b:02X}")
            if i < len(all_bitmap) - 1:
                f.write(", ")
            if i % 16 == 15:
                f.write("\n")
        f.write("\n};\n\n")

        # Glyph array
        f.write(f"const GFXglyph {name}Glyphs[] PROGMEM = {{\n")
        for i, gd in enumerate(glyph_data):
            code = first_char + i
            char_repr = chr(code) if 33 <= code <= 126 else f"0x{code:02X}"
            f.write(f"  {{ {gd['bitmapOffset']:5d}, {gd['width']:3d}, {gd['height']:3d}, "
                    f"{gd['xAdvance']:3d}, {gd['xOffset']:4d}, {gd['yOffset']:4d} }}")
            if i < len(glyph_data) - 1:
                f.write(",")
            f.write(f"  // '{char_repr}'\n")
        f.write("};\n\n")

        # Font struct
        f.write(f"const GFXfont {name} PROGMEM = {{\n")
        f.write(f"  (uint8_t  *){name}Bitmaps,\n")
        f.write(f"  (GFXglyph *){name}Glyphs,\n")
        f.write(f"  0x{first_char:02X}, 0x{last_char:02X},\n")
        f.write(f"  {y_advance}\n")
        f.write("};\n")

    print(f"Generated {output_path}: {len(all_bitmap)} bitmap bytes, "
          f"{len(glyph_data)} glyphs, yAdvance={y_advance}")


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    ttf = os.path.join(script_dir, "Antonio.ttf")
    include_dir = os.path.join(script_dir, "..", "include")

    # Generate 3 sizes for LCARS UI
    generate_font_header(ttf, 12, "LcarsFont12", os.path.join(include_dir, "lcars_font_12.h"))
    generate_font_header(ttf, 18, "LcarsFont18", os.path.join(include_dir, "lcars_font_18.h"))
    generate_font_header(ttf, 28, "LcarsFont28", os.path.join(include_dir, "lcars_font_28.h"))

    print("\nAll fonts generated successfully!")

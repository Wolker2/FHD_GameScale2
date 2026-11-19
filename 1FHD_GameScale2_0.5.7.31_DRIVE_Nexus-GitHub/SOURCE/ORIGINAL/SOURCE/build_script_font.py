#!/usr/bin/env python3
"""Build native AFT v1 fonts; requires Pillow and fonttools only for rebuilding.

The game loads the resulting AFT files directly, without installed Windows fonts.
Source: unmodified Lobster-Regular.ttf, SIL OFL 1.1 (Fonts/OFL.txt).
The derived bitmap family is named FHD Script Bold, not the reserved source name.
"""
from pathlib import Path
import math
import struct
import unicodedata

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "Fonts/Lobster-Regular.ttf"
OUTPUT = ROOT.parent / "DATA/Fonts"
SIZES = (14, 15, 18, 20, 22, 24)
SUPERSAMPLE = 4
CORE = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюяІіЇїЄєҐґ0123456789"


def encode_plane(image):
    """Byte-alpha RLE: 16-byte header, skip/literal runs, zero row terminator."""
    w, h = image.size
    pixels = image.tobytes()
    stream = bytearray()
    for y in range(h):
        row = pixels[y * w:(y + 1) * w]
        if not any(row):
            stream.append(128)  # Empty row, with no separate terminator.
            continue
        x = 0
        while x < w:
            literal = row[x] != 0
            end = x + 1
            while end < w and end - x < 127 and (row[end] != 0) == literal:
                end += 1
            stream.append((end - x) | (128 if literal else 0))
            if literal:
                stream.extend(row[x:end])
            x = end
        stream.append(0)
    return struct.pack("<4I", len(stream), w, h, 0) + stream


def rasterize(font, char, top_limit, bottom_limit):
    """Render real outline glyphs with a small bold expansion before downsampling."""
    scale = SUPERSAMPLE
    stroke = 1  # 1/4 logical pixel; source outlines already have a heavy script face.
    left, top, right, bottom = font.getbbox(char, anchor="ls", stroke_width=stroke)
    if right <= left or bottom <= top:
        return 0, 0, None
    # Baseline and x origin remain aligned to the logical pixel grid.
    x0, y0 = math.floor(left / scale), math.floor(top / scale)
    x1, y1 = math.ceil(right / scale), math.ceil(bottom / scale)
    image = Image.new("L", ((x1 - x0) * scale, (y1 - y0) * scale))
    ImageDraw.Draw(image).text((-x0 * scale, -y0 * scale), char, font=font,
                              fill=255, anchor="ls", stroke_width=stroke, stroke_fill=255)
    image = image.resize((x1 - x0, y1 - y0), Image.Resampling.LANCZOS)
    # Remove insignificant ringing: clear transparent texels, keep crisp solid strokes.
    image = image.point(lambda a: 0 if a < 8 else 255 if a > 247 else a)
    bounds = image.getbbox()
    if not bounds:
        return 0, 0, None
    image = image.crop(bounds)
    x0 += bounds[0]
    y0 += bounds[1]
    # Rare accented letters must not move every Russian/English line's baseline.
    # Only outlying glyphs are fitted; the core alphabet keeps its natural shape.
    fitted_top = max(y0, top_limit)
    fitted_bottom = min(y0 + image.height, bottom_limit)
    if fitted_top != y0 or fitted_bottom != y0 + image.height:
        image = image.resize((image.width, max(1, fitted_bottom - fitted_top)),
                             Image.Resampling.LANCZOS)
        y0 = fitted_top
    return x0, y0, image


def build(size, chars):
    font = ImageFont.truetype(str(SOURCE), size * SUPERSAMPLE,
                             layout_engine=ImageFont.Layout.BASIC)
    bounds = [font.getbbox(c, anchor="ls", stroke_width=1) for c in CORE]
    top = math.floor(min(b[1] for b in bounds) / SUPERSAMPLE)
    bottom = math.ceil(max(b[3] for b in bounds) / SUPERSAMPLE)
    cap_height = math.ceil(-font.getbbox("А", anchor="ls")[1] / SUPERSAMPLE)
    # Native GetLineHeight adds two pixels to the stored metric.
    line_height = bottom - top + 2
    header = struct.pack("<4s7i", b"aft\0", 1, len(chars), cap_height, 0,
                         line_height - 2, 0, 0)
    records, payload = bytearray(), bytearray()
    payload_start = 32 + 64 * len(chars)
    for code in chars:
        char = chr(code)
        advance = max(1, round(font.getlength(char) / SUPERSAMPLE))
        x, y, image = rasterize(font, char, top, bottom)
        if image:
            encoded = encode_plane(image)
            alpha = (x, y, image.width, image.height, payload_start + len(payload), len(encoded))
            payload.extend(encoded)
        else:
            alpha = (0,) * 6
        # Alpha plane includes opaque pixels; no duplicate mask plane is needed.
        records.extend(struct.pack("<I15i", code, 0, advance, 0, *((0,) * 6), *alpha))
    path = OUTPUT / f"FHD_Script_Bold_{size}.aft"
    path.write_bytes(header + records + payload)
    return path, line_height


def main():
    cmap = TTFont(SOURCE).getBestCmap()
    chars = sorted(c for c in cmap if 32 <= c <= 0xFFFF
                   and unicodedata.category(chr(c)) not in ("Cc", "Cf", "Mn", "Me"))
    required = set(CORE + " !?.,:;-+()%№«»–—…€")
    assert all(ord(c) in chars for c in required), "Missing a required game glyph"
    OUTPUT.mkdir(exist_ok=True)
    for size in SIZES:
        path, line_height = build(size, chars)
        print(f"{path.name}: {len(chars)} glyphs, line={line_height}, {path.stat().st_size} bytes")


if __name__ == "__main__":
    main()

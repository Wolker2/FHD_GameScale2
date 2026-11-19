#!/usr/bin/env python3
"""Build regular versions of both script and print families, without emboldening.

Rebuild requirements only: Pillow + fonttools. The game reads bundled AFT files.
Source fonts and their licenses are in SOURCE/Fonts.
"""
from pathlib import Path
import math
import struct
import unicodedata

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont
from build_script_font import CORE, SUPERSAMPLE, encode_plane

ROOT = Path(__file__).resolve().parent
OUTPUT = ROOT.parent / "DATA/Fonts"
FAMILIES = (
    ("Script", "Lobster-Regular.ttf", (14, 15, 18, 20, 22, 24)),
    ("Print", "DejaVuSans.ttf", (13, 14, 16, 18, 20, 22)),
)


def rasterize(font, char, top_limit, bottom_limit):
    # No stroke, dilation, synthetic bold or thinning: use the source Regular face.
    left, top, right, bottom = font.getbbox(char, anchor="ls")
    if right <= left or bottom <= top:
        return 0, 0, None
    scale = SUPERSAMPLE
    x0, y0 = math.floor(left / scale), math.floor(top / scale)
    x1, y1 = math.ceil(right / scale), math.ceil(bottom / scale)
    image = Image.new("L", ((x1 - x0) * scale, (y1 - y0) * scale))
    ImageDraw.Draw(image).text((-x0 * scale, -y0 * scale), char, font=font,
                              fill=255, anchor="ls", stroke_width=0)
    image = image.resize((x1 - x0, y1 - y0), Image.Resampling.LANCZOS)
    image = image.point(lambda a: 0 if a < 8 else 255 if a > 247 else a)
    bounds = image.getbbox()
    if not bounds:
        return 0, 0, None
    image = image.crop(bounds)
    x0 += bounds[0]
    y0 += bounds[1]
    fitted_top = max(y0, top_limit)
    fitted_bottom = min(y0 + image.height, bottom_limit)
    if fitted_top != y0 or fitted_bottom != y0 + image.height:
        image = image.resize((image.width, max(1, fitted_bottom - fitted_top)),
                             Image.Resampling.LANCZOS)
        y0 = fitted_top
    return x0, y0, image


def build(family, source, size, chars):
    font = ImageFont.truetype(str(source), size * SUPERSAMPLE,
                             layout_engine=ImageFont.Layout.BASIC)
    bounds = [font.getbbox(c, anchor="ls") for c in CORE]
    top = math.floor(min(b[1] for b in bounds) / SUPERSAMPLE)
    bottom = math.ceil(max(b[3] for b in bounds) / SUPERSAMPLE)
    cap_height = math.ceil(-font.getbbox("А", anchor="ls")[1] / SUPERSAMPLE)
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
        records.extend(struct.pack("<I15i", code, 0, advance, 0, *((0,) * 6), *alpha))
    path = OUTPUT / f"FHD_{family}_Regular_{size}.aft"
    path.write_bytes(header + records + payload)
    return path, line_height


def main():
    script_cmap = TTFont(ROOT / "Fonts/Lobster-Regular.ttf").getBestCmap()
    for family, source_name, sizes in FAMILIES:
        source = ROOT / "Fonts" / source_name
        cmap = TTFont(source).getBestCmap()
        wanted = set(script_cmap)
        if family == "Print":
            wanted |= set(range(32, 256)) | set(range(0x400, 0x530))
            wanted |= {0x20AC, 0x20B4, 0x20BD, 0x2116, 0x2190, 0x2191, 0x2192, 0x2193,
                       0x2212, 0x2264, 0x2265}
        chars = sorted(c for c in wanted & cmap.keys() if 32 <= c <= 0xFFFF
                       and unicodedata.category(chr(c)) not in ("Cc", "Cf", "Mn", "Me"))
        required = set(CORE + " !?.,:;-+()%№«»–—…€" + ("₽₴←↑→↓" if family == "Print" else ""))
        assert all(ord(c) in chars for c in required), "Missing a required game glyph"
        for size in sizes:
            path, line_height = build(family, source, size, chars)
            print(f"{path.name}: {len(chars)} glyphs, line={line_height}, {path.stat().st_size} bytes")


if __name__ == "__main__":
    main()

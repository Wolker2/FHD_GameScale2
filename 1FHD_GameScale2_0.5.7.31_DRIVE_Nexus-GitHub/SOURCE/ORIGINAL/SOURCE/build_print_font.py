#!/usr/bin/env python3
"""Build the upright FHD Print Bold family from bundled DejaVu Sans Bold.

Requirements for rebuilding only: Pillow + fonttools.
Playing uses native AFT files and requires no installed Windows font.
Copyright and license: Fonts/DejaVu-LICENSE.txt.
"""
from pathlib import Path
import math
import struct
import unicodedata

from PIL import ImageFont
from fontTools.ttLib import TTFont
from build_script_font import CORE, SUPERSAMPLE, encode_plane, rasterize

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "Fonts/DejaVuSans-Bold.ttf"
OUTPUT = ROOT.parent / "DATA/Fonts"
SIZES = (13, 14, 16, 18, 20, 22)


def build(size, chars):
    font = ImageFont.truetype(str(SOURCE), size * SUPERSAMPLE,
                             layout_engine=ImageFont.Layout.BASIC)
    bounds = [font.getbbox(c, anchor="ls", stroke_width=1) for c in CORE]
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
    path = OUTPUT / f"FHD_Print_Bold_{size}.aft"
    path.write_bytes(header + records + payload)
    return path, line_height


def main():
    cmap = TTFont(SOURCE).getBestCmap()
    script_cmap = TTFont(ROOT / "Fonts/Lobster-Regular.ttf").getBestCmap()
    # Preserve the script variant's alphabet, plus Latin-1, extended Cyrillic,
    # currencies and direction arrows used by interface text.
    wanted = set(script_cmap) | set(range(32, 256)) | set(range(0x400, 0x530))
    wanted |= {0x20AC, 0x20B4, 0x20BD, 0x2116, 0x2190, 0x2191, 0x2192, 0x2193,
               0x2212, 0x2264, 0x2265}
    chars = sorted(c for c in wanted & cmap.keys() if 32 <= c <= 0xFFFF
                   and unicodedata.category(chr(c)) not in ("Cc", "Cf", "Mn", "Me"))
    required = set(CORE + " !?.,:;-+()%№«»–—…€₽₴←↑→↓")
    assert all(ord(c) in chars for c in required), "Missing a required game glyph"
    OUTPUT.mkdir(exist_ok=True)
    for size in SIZES:
        path, line_height = build(size, chars)
        print(f"{path.name}: {len(chars)} glyphs, line={line_height}, {path.stat().st_size} bytes")


if __name__ == "__main__":
    main()

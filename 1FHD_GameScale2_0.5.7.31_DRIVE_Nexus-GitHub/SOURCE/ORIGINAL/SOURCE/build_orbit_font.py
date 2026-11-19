#!/usr/bin/env python3
"""Build native FHD Orbit bitmap fonts for the Exo 2 menu option.

Pillow + fonttools are required only for rebuilding. No system-font installation
or additional runtime component is needed to play.
Source outline: Exo2-Variable.ttf, SIL OFL 1.1 (Fonts/Exo2-OFL.txt).
"""
from pathlib import Path
from io import BytesIO
import math
import struct
import unicodedata

from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
from PIL import ImageFont
from build_script_font import CORE, SUPERSAMPLE, encode_plane
from build_regular_fonts import rasterize

ROOT = Path(__file__).resolve().parent
OUTPUT = ROOT.parent / "DATA/Fonts"
SOURCE = ROOT / "Fonts/Exo2-Variable.ttf"
FALLBACK = ROOT / "Fonts/DejaVuSans.ttf"
SIZES = (13, 14, 16, 18, 20, 22)


def build(name, outline, size, chars, cmap):
    font = ImageFont.truetype(BytesIO(outline), size * SUPERSAMPLE,
                             layout_engine=ImageFont.Layout.BASIC)
    fallback = ImageFont.truetype(str(FALLBACK), size * SUPERSAMPLE,
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
        face = font if code in cmap else fallback
        advance = max(1, round(face.getlength(char) / SUPERSAMPLE))
        x, y, image = rasterize(face, char, top, bottom)
        if image:
            encoded = encode_plane(image)
            alpha = (x, y, image.width, image.height, payload_start + len(payload), len(encoded))
            payload.extend(encoded)
        else:
            alpha = (0,) * 6
        records.extend(struct.pack("<I15i", code, 0, advance, 0, *((0,) * 6), *alpha))
    path = OUTPUT / f"FHD_Orbit_{name}_{size}.aft"
    path.write_bytes(header + records + payload)
    return path, line_height


def main():
    cmap = TTFont(SOURCE).getBestCmap()
    fallback_cmap = TTFont(FALLBACK).getBestCmap()
    wanted = set(cmap) | set(range(32, 256)) | set(range(0x400, 0x530))
    wanted |= {0x20AC, 0x20B4, 0x20BD, 0x2116, 0x2190, 0x2191, 0x2192, 0x2193,
               0x2212, 0x2264, 0x2265}
    chars = sorted(c for c in wanted & (cmap.keys() | fallback_cmap.keys())
                   if 32 <= c <= 0xFFFF and unicodedata.category(chr(c))
                   not in ("Cc", "Cf", "Mn", "Me"))
    required = set(CORE + " !?.,:;-+()%№«»–—…€₽₴←↑→↓")
    assert all(ord(c) in chars for c in required)
    # All body letters and digits use the new family, not the fallback.
    assert all(ord(c) in cmap for c in CORE)
    for name, weight in (("Medium", 500), ("Semibold", 600)):
        font = instantiateVariableFont(TTFont(SOURCE), {"wght": weight}, inplace=True)
        memory = BytesIO()
        font.save(memory)
        for size in SIZES:
            path, line_height = build(name, memory.getvalue(), size, chars, cmap)
            print(f"{path.name}: {len(chars)} glyphs, line={line_height}, {path.stat().st_size} bytes")


if __name__ == "__main__":
    main()

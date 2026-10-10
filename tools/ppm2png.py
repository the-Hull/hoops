#!/usr/bin/env python3
"""Convert the self-test's screenshots (240x240 binary PPMs) to 480x480 PNGs.

Usage: ppm2png.py <in_dir> <out_dir>

Every <in_dir>/<stem>.ppm becomes <out_dir>/<stem>.png, scaled 2x with
nearest-neighbour sampling (the watch's pixels stay sharp). out_dir is
created when missing. Needs Pillow (in the repo's .venv).
"""

import sys
from pathlib import Path

from PIL import Image

SCALE = 2


def main(argv):
    if len(argv) != 3:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    in_dir, out_dir = Path(argv[1]), Path(argv[2])
    ppms = sorted(in_dir.glob("*.ppm"))
    if not ppms:
        print(f"no .ppm files in {in_dir}", file=sys.stderr)
        return 1
    out_dir.mkdir(parents=True, exist_ok=True)
    for ppm in ppms:
        with Image.open(ppm) as img:
            rgb = img.convert("RGB")
            big = rgb.resize((rgb.width * SCALE, rgb.height * SCALE), Image.NEAREST)
            out = out_dir / (ppm.stem + ".png")
            big.save(out, optimize=True)
            print(f"{ppm.name} -> {out} ({big.width}x{big.height})")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

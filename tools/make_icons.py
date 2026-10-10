#!/usr/bin/env python3
"""Draw the Hoops app icon (a basketball) and write the three icon PNGs.

Usage: .venv/bin/python3 tools/make_icons.py

The ball is drawn once at 240x240 (orange disc, black seams: one vertical,
one horizontal, two side arcs; transparent outside the disc) and downsampled
with LANCZOS to:

  Resources/icon_60x60.png   watch launcher icon (app_merging.py -normal_icon)
  Resources/icon_30x30.png   small watch icon     (app_merging.py -small_icon)
  store/icon.png             60x60, the copy the store zip takes

All three are RGBA PNGs. The watch packs them to ABGR2222 (2 bits per
channel), so the orange lands on its nearest palette entry there. The output
is deterministic: no randomness, and Pillow writes no timestamp chunk.
Paths are relative to the repository root (the parent of tools/).
"""

from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

MASTER = 240          # drawing size in px
MARGIN = 6            # transparent ring around the disc at MASTER size
SEAM = 3              # seam width at MASTER size (px); see also SEAM_MIN_PX
SEAM_MIN_PX = 1.5     # seams are widened so they stay >= this many px wide
                      # at the smallest output (else LANCZOS washes them out)
ORANGE = (0xE0, 0x60, 0x10, 0xFF)
BLACK = (0x00, 0x00, 0x00, 0xFF)

OUTPUTS = (
    ("Resources/icon_60x60.png", 60),
    ("Resources/icon_30x30.png", 30),
    ("store/icon.png", 60),
)


def seam_width(size):
    """Seam width at MASTER size so that the seams are visible at `size`."""
    return max(SEAM, round(SEAM_MIN_PX * MASTER / size))


def draw_ball(seam):
    c = MASTER / 2
    r = c - MARGIN
    box = (c - r, c - r, c + r, c + r)

    # Disc mask: opaque inside the ball, transparent corners.
    mask = Image.new("L", (MASTER, MASTER), 0)
    ImageDraw.Draw(mask).ellipse(box, fill=255)

    ball = Image.new("RGBA", (MASTER, MASTER), ORANGE)
    d = ImageDraw.Draw(ball)
    # Vertical and horizontal seams through the centre.
    d.line((c, 0, c, MASTER), fill=BLACK, width=seam)
    d.line((0, c, MASTER, c), fill=BLACK, width=seam)
    # Two side arcs: circles centred left and right of the ball whose inner
    # edges bulge towards the centre, like the seams of a real basketball.
    arc_r = r * 1.15
    off = r * 1.62
    for cx in (c - off, c + off):
        d.ellipse((cx - arc_r, c - arc_r, cx + arc_r, c + arc_r),
                  outline=BLACK, width=seam)

    alpha = ImageChops.multiply(ball.getchannel("A"), mask)
    ball.putalpha(alpha)
    return ball


def main():
    root = Path(__file__).resolve().parent.parent
    for rel, size in OUTPUTS:
        img = draw_ball(seam_width(size)).resize((size, size), Image.LANCZOS)
        out = root / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        img.save(out, format="PNG", optimize=True)
        print(f"wrote {rel} ({size}x{size})")


if __name__ == "__main__":
    main()

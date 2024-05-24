import fxconv
import sys
import os

from PIL import Image, ImageChops

# Add ../submodules to Python path automatically
sys.path.append(os.path.join(os.path.dirname(__file__), "../submodules/rectpack"))
import rectpack

def convert(input, output, params, target):
    match params["custom-type"]:
        case "kbd-sprite": return convert_kbd_sprite(input, params["keycodes"])
    return None

# Assuming that im has a rectangle of pixels all satisfying a predicate P and
# (x,y) is part of that rectangle, find out the outline of the rectangle. If
# check=True, assert that the area satisfying P around (x,y) is indeed a
# rectangle. Returns (rx, ry, rw, rh).
def _image_get_rectangle(im, x, y, P, check=True):
    px = im.load()
    assert P(px[x, y])
    x0, x1, y0, y1 = x, x, y, y

    # Expand if all four directions
    while x0 > 0 and P(px[x0-1, y]):
        x0 -= 1
    while x1 < im.width-1 and P(px[x1+1, y]):
        x1 += 1
    while y0 > 0 and P(px[x, y0-1]):
        y0 -= 1
    while y1 < im.height-1 and P(px[x, y1+1]):
        y1 += 1

    if check:
        # Check that the entire rectangle satisfies the predicate
        assert all(P(px[x, y]) for y in range(y0,y1+1) for x in range(x0,x1+1))
        # Check that neighbouring pixels don't
        if x0 > 0:
            assert all(not P(px[x0-1, y]) for y in range(y0,y1+1))
        if x1 < im.width-1:
            assert all(not P(px[x1+1, y]) for y in range(y0,y1+1))
        if y0 > 0:
            assert all(not P(px[x, y0-1]) for x in range(x0,x1+1))
        if y1 < im.height-1:
            assert all(not P(px[x, y1+1]) for x in range(x0,x1+1))

    return (x0, y0, x1-x0+1, y1-y0+1)

# Replace all pixels matching predicate/color P0 in the rectangle r=(x,y,w,h)
# of im with given color/transform f0.
def _image_replace_pixels(im, r, P0, f0):
    px = im.load()
    if r is None:
        rx, ry, rw, rh = 0, 0, im.width, im.height
    else:
        rx, ry, rw, rh = r

    P = P0 if callable(P0) else lambda c: c == P0
    f = f0 if callable(f0) else lambda c: f0

    for y in range(ry, ry+rh):
        for x in range(rx, rx+rw):
            if P(px[x, y]):
                px[x, y] = f(px[x, y])

def _kbdsprite_extract_rectangles(im):
    px = im.load()
    bgs, keys = [], []

    COLOR_NONE = (0xff, 0xff, 0xff)
    COLOR_TEXT = (0x00, 0x00, 0x00)
    COLOR_KEY  = (0xc0, 0xc0, 0xc0)
    COLOR_BG   = (0xda, 0xda, 0xda)
    assert all(px[x,y] in [COLOR_NONE, COLOR_TEXT, COLOR_KEY, COLOR_BG]
               for y in range(im.height) for x in range(im.width))

    # Predicates identifying pixels in background and key rectangles
    P_bg  = lambda c: c in [COLOR_TEXT, COLOR_KEY, COLOR_BG]
    P_key = lambda c: c in [COLOR_TEXT, COLOR_KEY]

    # First, find all background rectangles
    for y in range(im.height):
        for x in range(im.width):
            if px[x, y] == COLOR_BG:
                r = _image_get_rectangle(im, x, y, P_bg)
                _image_replace_pixels(im, r, COLOR_BG, COLOR_NONE)
                bgs.append(r)

    # Then, find all key rectangles
    for y in range(im.height):
        for x in range(im.width):
            if px[x, y] == COLOR_KEY:
                r = _image_get_rectangle(im, x, y, P_key)
                _image_replace_pixels(im, r, COLOR_KEY, COLOR_NONE)
                keys.append(r)

    return bgs, keys

def _kbdsprite_keycodes(model):
    if model == "cg":
        return [
            0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
                                                0x86,
            0x81, 0x82, 0x83, 0x84,       0x85,       0x76,
            0x71, 0x72, 0x73, 0x74,             0x75,

            0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
            0x51, 0x52, 0x53, 0x54, 0x55, 0x56,

            0x41, 0x42, 0x43, 0x44,       0x07,
            0x31, 0x32, 0x33, 0x34, 0x35,
            0x21, 0x22, 0x23, 0x24, 0x25,
            0x11, 0x12, 0x13, 0x14, 0x15 ]
    raise Exception(f"unknown keycode set {model}")

def convert_kbd_sprite(input, keycodeset):
    # Find background (light gray) and key (dark gray) rectangles
    im = Image.open(input)
    BGS, KEYS = _kbdsprite_extract_rectangles(im)

    # Join key rectangles with their keycode
    CODES = _kbdsprite_keycodes(keycodeset)
    KEYS = [r + (c,) for r, c in zip(KEYS, CODES)]

    # Clean the image to keep only the black (label text)
    im = im.convert("RGBA")
    _image_replace_pixels(im, None, lambda c: c != (0,0,0,255), (0,0,0,0))

    # Find the bounding rectangles of individual key labels
    KEYLABELS = []
    for x, y, w, h, code in KEYS:
        # Select the label and crop is as small as possible. Since we only have
        # 3 bits for the label y relative to the key y, crop no more than 8
        # pixels (limit is hit only for a few keys like '.')
        key = im.crop((x, y, x+w, y+h))
        lx, ly, lright, lbottom = key.getbbox()
        if ly >= 8:
            ly = 7
        lw = lright-lx
        lh = lbottom-ly
        assert lx < 16 and ly < 8
        KEYLABELS.append((lx, ly, lw, lh))

    # Standard expected solution is 160x30
    SPRITE_WIDTH = 160
    SPRITE_HEIGHT = 24

    while True:
        packer = rectpack.newPacker(rotation=False)
        packer.add_bin(SPRITE_WIDTH, SPRITE_HEIGHT)
        for index, (lx, ly, lw, lh) in enumerate(KEYLABELS):
            packer.add_rect(lw, lh, index)
        packer.pack()
        if len(packer) == 1 and len(packer[0]) == len(KEYLABELS):
            break
        SPRITE_HEIGHT += 1

    print(os.path.basename(input) + ":",
          f"Packed labels in {SPRITE_WIDTH}x{SPRITE_HEIGHT}")

    im2 = Image.new("RGBA", (SPRITE_WIDTH, SPRITE_HEIGHT))

    # Sanity check that the loop below will indeed fill in the information for
    # every rectangle
    assert({ r[5] for r in packer.rect_list() } == set(range(len(KEYS))))
    DATA_KEYS = [None] * len(KEYS)

    # Encode keys
    for (_, sx, sy, sw, sh, index) in packer.rect_list():
        x, y, w, h, code = KEYS[index]
        lx, ly, lw, lh = KEYLABELS[index]
        assert sw == lw and sh == lh

        # Copy from source (x+lx, y+ly, lw, lh) to sprite (sx, sy)
        im2.paste(im.crop((x+lx, y+ly, x+lx+lw, y+ly+lh)), (sx, sy))

        # Fill in key data
        def bits(value, n):
            assert 0 <= value < (1 << n)
            return value
        DATA_KEYS[index] = bytes([
            x, y, w, h, sx,
            (bits(sy, 5) << 3) + bits(0, 3), # 3 free bits here!
            (bits(lw, 5) << 3) + bits(ly, 3),
            (bits(lh, 4) << 4) + bits(lx, 4),
            bits(code, 8),
        ])

    o = fxconv.ObjectData()
    o += fxconv.ptr(fxconv.convert_bopti_cg(im2, {"profile": "p4_rgb565a"}))
    o += fxconv.u16(len(BGS))
    o += fxconv.u16(len(KEYS))
    o += fxconv.ptr(b"".join(bytes([x, y, w, h]) for x, y, w, h in BGS))
    o += fxconv.ptr(b"".join(DATA_KEYS))
    return o

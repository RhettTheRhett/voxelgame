from PIL import Image
import random

path = r"d:\Dev\voxelgame\assets\textures\blocks\blocksatlas.png"
im = Image.open(path).convert("RGBA")


def clear_tile(tx, ty):
    x0, y0 = tx * 16, ty * 16
    for y in range(16):
        for x in range(16):
            im.putpixel((x0 + x, y0 + y), (0, 0, 0, 0))


def put(tx, ty, lx, ly, rgba):
    im.putpixel((tx * 16 + lx, ty * 16 + ly), rgba)


# Leaves (3,0): leafy canopy with transparent holes
clear_tile(3, 0)
rng = random.Random(42)
greens = [(46, 120, 40), (58, 140, 48), (34, 100, 32), (70, 155, 55), (40, 110, 36)]
for y in range(16):
    for x in range(16):
        hole = False
        if (x + y * 3) % 7 == 0 and rng.random() < 0.55:
            hole = True
        if (x * 5 + y) % 11 == 0 and rng.random() < 0.4:
            hole = True
        if x in (0, 15) or y in (0, 15):
            if rng.random() < 0.35:
                hole = True
        if hole:
            put(3, 0, x, y, (0, 0, 0, 0))
        else:
            g = greens[(x * 3 + y * 5) % len(greens)]
            n = rng.randint(-8, 8)
            put(
                3,
                0,
                x,
                y,
                (
                    max(0, min(255, g[0] + n)),
                    max(0, min(255, g[1] + n)),
                    max(0, min(255, g[2] + n)),
                    255,
                ),
            )

# Red mushroom (3,1): stem + spotted cap on transparent
clear_tile(3, 1)
stem = (230, 220, 200, 255)
stem_dark = (200, 185, 160, 255)
for y in range(9, 16):
    for x in range(6, 10):
        put(3, 1, x, y, stem_dark if x in (6, 9) else stem)

cap = (180, 40, 40, 255)
cap_dark = (140, 25, 25, 255)
spots = {(4, 4), (8, 3), (11, 5), (6, 6), (10, 7), (5, 7)}
for y in range(1, 10):
    for x in range(2, 14):
        cx, cy = 7.5, 5.5
        rx, ry = 5.5, 4.2
        nx = (x - cx) / rx
        ny = (y - cy) / ry
        if nx * nx + ny * ny <= 1.0:
            if (x, y) in spots:
                put(3, 1, x, y, (245, 240, 230, 255))
            else:
                put(3, 1, x, y, cap_dark if ny > 0.35 else cap)

im.save(path)
print("atlas updated: leaves (3,0), red_mushroom (3,1)")

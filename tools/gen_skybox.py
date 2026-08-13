#!/usr/bin/env python3
"""Generate a seamless skybox cubemap for HexaCraft.

The six shipped faces were painted independently, so their cloud patterns did
not line up and the cube edges were plainly visible in game (see
docs/bug_evidence/08_skybox_seams_double_clouds.png). Here every pixel's colour
is a pure function of its *direction* from the origin, so two faces that share
an edge look up the same direction and get the same colour — the seams cannot
exist. Clouds come from 3D value noise sampled on the direction vector, which is
continuous over the whole sphere for the same reason.

The lower hemisphere is haze, not ground: the old bottom.png was flat green
(centre pixel 157,205,114), which is what produced the "world ends in a green
disc" look past the render distance. Fading it to the horizon colour instead
lets distant terrain dissolve into fog against a matching background.

Usage:  python3 tools/gen_skybox.py [outdir]     (default: assets/srcs)
"""
import sys, pathlib
import numpy as np
from PIL import Image

SIZE = 512

# Must match the noon sky/fog colour in src/main.cpp, or terrain fading into fog
# meets the skybox at a visible colour step on the horizon.
HORIZON = np.array([0.45, 0.65, 0.95])
ZENITH  = np.array([0.20, 0.40, 0.82])
GROUND  = np.array([0.38, 0.52, 0.72])   # below-horizon haze, slightly darker
CLOUD   = np.array([1.00, 1.00, 1.00])


def hash3(ix, iy, iz):
    """Deterministic per-lattice-point pseudo-random in [0,1)."""
    n = ix * 374761393 + iy * 668265263 + iz * 2147483647
    n = (n ^ (n >> 13)) * 1274126177
    n = n ^ (n >> 16)
    return (n & 0x7FFFFFFF) / float(0x7FFFFFFF)


def value_noise3(x, y, z):
    """Trilinear value noise. Continuous everywhere, so it never seams."""
    ix, iy, iz = np.floor(x).astype(np.int64), np.floor(y).astype(np.int64), np.floor(z).astype(np.int64)
    fx, fy, fz = x - ix, y - iy, z - iz
    # smoothstep for C1 continuity
    ux, uy, uz = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy), fz * fz * (3 - 2 * fz)
    c = {}
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                c[(dx, dy, dz)] = hash3(ix + dx, iy + dy, iz + dz)
    x00 = c[(0,0,0)] * (1-ux) + c[(1,0,0)] * ux
    x10 = c[(0,1,0)] * (1-ux) + c[(1,1,0)] * ux
    x01 = c[(0,0,1)] * (1-ux) + c[(1,0,1)] * ux
    x11 = c[(0,1,1)] * (1-ux) + c[(1,1,1)] * ux
    y0 = x00 * (1-uy) + x10 * uy
    y1 = x01 * (1-uy) + x11 * uy
    return y0 * (1-uz) + y1 * uz


def fbm(d, octaves=5, freq=2.6, gain=0.5):
    total = np.zeros(d.shape[:-1])
    amp, f = 1.0, freq
    norm = 0.0
    for _ in range(octaves):
        total += amp * value_noise3(d[..., 0] * f, d[..., 1] * f, d[..., 2] * f)
        norm += amp
        amp *= gain
        f *= 2.0
    return total / norm


def sky(d):
    """d: (...,3) unit directions -> (...,3) linear RGB in [0,1]."""
    y = d[..., 1]
    up = np.clip(y, 0.0, 1.0)

    # Vertical gradient, biased so most of the visible band is the paler horizon
    t = up ** 0.55
    col = HORIZON[None, None, :] * (1 - t)[..., None] + ZENITH[None, None, :] * t[..., None]

    # Below the horizon: fade to haze rather than a hard ground colour
    below = np.clip(-y, 0.0, 1.0) ** 0.6
    col = col * (1 - below)[..., None] + GROUND[None, None, :] * below[..., None]

    # Clouds: banded fbm, thinning toward the zenith and cut off below the horizon
    n = fbm(d)
    cover = np.clip((n - 0.46) / 0.30, 0.0, 1.0)      # coverage threshold
    cover = cover * cover * (3 - 2 * cover)           # soften edges
    band = np.clip((y - 0.015) / 0.13, 0.0, 1.0) * np.clip((0.95 - up) / 0.5, 0.0, 1.0)
    a = (cover * band)[..., None]
    # Slight grey in the thicker parts so clouds read as volume, not paper
    shade = 1.0 - 0.13 * cover[..., None]
    col = col * (1 - a) + CLOUD[None, None, :] * shade * a
    return np.clip(col, 0.0, 1.0)


def face_dirs(face, size):
    """OpenGL cubemap face -> unit direction per pixel.
    s runs left->right, t runs top->bottom, both over [-1, 1]."""
    lin = (np.arange(size) + 0.5) / size * 2.0 - 1.0
    s, t = np.meshgrid(lin, lin)               # s: x across, t: y down
    one = np.ones_like(s)
    d = {
        0: (one,  -t, -s),   # +X right
        1: (-one, -t,  s),   # -X left
        2: (s,    one,  t),  # +Y top
        3: (s,   -one, -t),  # -Y bottom
        4: (s,    -t,  one), # +Z front
        5: (-s,   -t, -one), # -Z back
    }[face]
    v = np.stack(d, axis=-1)
    return v / np.linalg.norm(v, axis=-1, keepdims=True)


def main():
    outdir = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "assets/srcs")
    outdir.mkdir(parents=True, exist_ok=True)
    names = ["right", "left", "top", "bottom", "front", "back"]
    for i, name in enumerate(names):
        d = face_dirs(i, SIZE)
        rgb = (sky(d) ** (1 / 1.15) * 255).astype(np.uint8)   # mild gamma lift
        path = outdir / f"{name}.png"
        Image.fromarray(rgb, "RGB").save(path)
        print(f"[gen_skybox] wrote {path} ({SIZE}x{SIZE})")


if __name__ == "__main__":
    main()

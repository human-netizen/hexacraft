# HexaCraft — Comprehensive Implementation Plan

Based on reference visuals: the world should feel like a real single-player Minecraft-style game built with hexagonal prisms. The player walks around, places/breaks blocks, explores biomes, and interacts with objects.

---

## COMPLETED (Phases 1–9)

- [x] Hexagonal prism geometry, GLFW + GLAD + shader pipeline
- [x] Custom myRotate() (Rodrigues' formula)
- [x] Procedural terrain (FBM noise, biomes: grass, sand, stone, water)
- [x] Trees (trunk + randomized leaf canopy, colorful, clustered in forest zone)
- [x] Camera: Mouse look, Pitch(X)/Yaw(Y)/Roll(Z), Bird's Eye(B), Rotate around look-at(F), 4-viewport(V)
- [x] Lighting: Directional(1), Point(2), Spot(3), Emissive ores, Ambient(5)/Diffuse(6)/Specular(7), Master(L)
- [x] Day-night cycle (T)
- [x] Player character (hierarchical joints: head, body, arms, legs, faces movement dir)
- [x] Fan (G), Door (O), Clock with rotating hands
- [x] Drivable MineCar (arrow keys)
- [x] Torches with flickering point lights
- [x] Curvy objects: Sphere, Cone, Bezier curve, Spline curve, Ruled surface
- [x] Stone ruins structure, 6 biome zones (A–F)
- [x] 3D voxel grid (80x80x16) — terrain stored as block types, rendered from grid
- [x] Block breaking (left-click) and placement (right-click)
- [x] Ray casting (30 unit range) with wireframe block highlight
- [x] Hotbar with 9 block types (scroll to cycle, numpad 1-9 quick select)
- [x] HUD: crosshair, hotbar display, health bar, stamina bar (screen-space)
- [x] Player movement: WASD walks on terrain, Space jumps, gravity, ground collision
- [x] Camera modes (C key): Third-Person / First-Person / Free-Fly
- [x] Direct mouse look (cursor captured, like Minecraft)

---

## CURRENT CONTROLS

### Player Movement
| Key | Action |
|-----|--------|
| W / S | Walk Forward / Backward |
| A / D | Strafe Left / Right |
| Space | Jump |
| E / R | Fly Up / Down |
| Mouse Move | Look around |

### Camera
| Key | Action |
|-----|--------|
| C | Cycle: Third-Person > First-Person > Free-Fly |
| X | Pitch up (Shift+X = down) |
| Y | Yaw right (Shift+Y = left) |
| Z | Roll CW (Shift+Z = CCW) |
| B | Toggle Bird's Eye View |
| F | Rotate around look-at (Free-Fly only) |
| V | Toggle 4-Viewport split |

### Block Building
| Key | Action |
|-----|--------|
| Left Click | Break block |
| Right Click | Place block |
| Scroll | Cycle hotbar slot |
| Ctrl + Scroll | Zoom in/out |
| Numpad 1-9 | Quick-select hotbar slot |

### Lighting
| Key | Action |
|-----|--------|
| 1 | Directional light on/off |
| 2 | Point lights on/off |
| 3 | Spot light on/off |
| 5 | Ambient on/off |
| 6 | Diffuse on/off |
| 7 | Specular on/off |
| L | Master light on/off |

### Other
| Key | Action |
|-----|--------|
| T | Cycle day/night: Night > Dawn > Noon > Dusk |
| G | Toggle rotating fan |
| O | Toggle door open/close |
| Arrow Keys | Drive MineCar (Up/Down = accel, Left/Right = steer) |
| ESC | Quit |

---

## Phase 10: Enhanced World & Visuals

### 10A: Bigger, Better Trees (match reference)
- **Thick trunks**: 2–3 hex columns wide, 5–7 hex tall
- **Massive canopies**: 5–6 hex radius irregular cloud of leaf blocks
- **Colors**: Random per-tree from palette [red, orange, yellow, cyan, blue, purple, bright green, dark green]
- **Forest zone (Zone B)**: Dense cluster of 15–25 large trees, close together forming a canopy
- **Standalone trees**: 2–3 large trees scattered in plains for landmarks
- Trees stored in blockGrid as WOOD (trunk) and LEAF (canopy) blocks — player can chop them

### 10B: Terrain Improvements
- **Stepped terrain**: Reference shows distinct height steps (like Minecraft). Each biome has clear plateaus.
  - Grass biome: Rolling hills 1–4 hex tall with flat tops
  - Sand biome: Flat, height 0–1
  - Stone biome: Elevated plateaus 3–5 hex tall with cliff edges
- **Dirt layers**: Below grass top, 2–3 layers of dirt blocks visible on cliff sides
- **Larger world**: Extend grid to -50 to 50 (100x100) if performance allows

### 10C: Water
- Water blocks at height 0–1 forming ponds/lakes (3–5 clusters)
- Slightly transparent blue (use alpha blending)
- Player cannot place blocks in water (or can, to build bridges)
- Water rendered with slight wave animation (vertex Y offset using sin wave)

### 10D: Ruins (Enhanced)
- Bigger structure: 8 pillars, connecting beams, partial walls, broken sections
- Some blocks missing for ruined look
- Vines (green leaf blocks on sides)
- Placed in Zone D, visible from far away

### 10E: Sky & Atmosphere
- **Sky gradient**: Blue at top → lighter at horizon (fragment shader based on screen Y)
- **Fog**: Distance-based, blends objects into sky color at far distances
- **Clouds**: Flat white hex clusters floating at y=20, slowly drifting
- **Sun/Moon**: Bright yellow/white sphere in sky, position tied to day-night cycle angle
- **Stars at night**: Small bright dots scattered in sky dome (only visible when dayFactor < 0.3)

---

## Phase 11: Advanced Features (from course requirements)

### 11A: Fractal Tree Leaves
- L-system or recursive branching for tree canopy structure
- Instead of random blob, branches split and grow leaves at tips
- At least 1 tree uses fractal generation (showcase tree near spawn)

### 11B: Flying Bird with Random Spline Motion
- Bird model: Small body (1 hex) + 2 wing hexes that flap up/down
- Path: Catmull-Rom spline through 5–8 random waypoints in the sky
- When reaching end of spline, generate new random waypoints and new spline
- Bird loops endlessly, visible in sky
- 2–3 birds at different heights/speeds

### 11C: Collision Detection
- **Player–terrain**: Player cannot walk through solid blocks (already partly handled by snap-to-ground)
- **Player–object**: Cannot walk through fan, door, ruins walls
- **Car–terrain**: Car cannot drive through blocks, follows terrain height
- Method: AABB (axis-aligned bounding box) per object. Before moving, check if new position collides.

### 11D: Wine Glass Object
- Use the "wine glass making program" export points
- Generate surface of revolution from the profile curve
- Place on a table/pedestal near the clock (Zone E)

### 11E: Gouraud vs Phong Shading Toggle
- Key `H`: Toggle between Gouraud (per-vertex) and Phong (per-fragment) shading
- Gouraud: Compute lighting in vertex shader, pass color to fragment
- Phong: Current approach (lighting in fragment shader)
- Best showcased in 4-viewport mode: two viewports Gouraud, two viewports Phong

### 11F: Textures (from requirements)
- At least 2 textured objects:
  1. Simple texture: Apply a texture (e.g., stone brick pattern) to ruins blocks without surface color blending
  2. Blended texture: Apply texture to ground blocks blended with biome surface color
- Textured curvy objects: Apply texture to the sphere (globe/crystal) and cone (tent fabric)
- Requires: texture loading (stb_image.h), UV coordinates in vertex data, sampler2D in fragment shader

---

## Phase 12: Polish & Game Feel

### 12A: Sound (Optional)
- Background ambient music
- Block break/place sound effects
- Walking footstep sounds

### 12B: Save/Load (Optional)
- Save blockGrid to file
- Load on startup if save file exists
- Player can build something, close game, and come back to it

### 12C: Performance
- Frustum culling: Only render blocks within camera view
- Face culling: Skip internal faces between adjacent solid blocks
- Chunk system: Divide world into 16x16 chunks, only update VAO when chunk changes

---

## KEY DISTANCES (hex grid units)

| Object           | Grid Position        | Zone          |
|-----------------|---------------------|---------------|
| Player spawn    | col=3, row=5        | A (plains)    |
| Car spawn       | col=5, row=-5       | C (sand)      |
| Forest start    | col=-5 to 10, row=18-28 | B (forest) |
| Ruins           | col=20, row=10      | D (east)      |
| Fan             | col=-15, row=5      | E (west)      |
| Door            | col=-18, row=10     | E (west)      |
| Clock           | col=-15, row=15     | E (west)      |
| Water pond 1    | col=8, row=12       | F             |
| Water pond 2    | col=-8, row=8       | F             |
| Standalone tree | col=5, row=10       | A (landmark)  |
| Sphere/pedestal | col=18, row=12      | D (ruins)     |
| Cone/tent       | col=8, row=-7       | C (sand)      |
| Bezier arch     | col=22, row=8       | D (ruins)     |
| Spline fence    | col=0, row=8        | A (plains)    |
| Ruled surface   | col=-12, row=12     | E (west)      |
| Wine glass      | col=-16, row=15     | E (near clock)|
| Showcase tree   | col=0, row=12       | A (fractal)   |

---

## IMPLEMENTATION ORDER (remaining)

1. **Phase 10** (World visuals) — Better trees, terrain, water, sky. Visual impact.
2. **Phase 11** (Course requirements) — Fractal trees, birds, collision, wine glass, Gouraud/Phong, textures.
3. **Phase 12** (Polish) — Performance, save/load, sound.

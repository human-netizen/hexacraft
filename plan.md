# HexaCraft — Comprehensive Implementation Plan

Based on reference visuals: the world should feel like a real single-player Minecraft-style game built with hexagonal prisms. The player walks around, places/breaks blocks, explores biomes, and interacts with objects.

---

## COMPLETED (Phases 1–9)

- [x] Hexagonal prism geometry, GLFW + GLAD + shader pipeline
- [x] Custom myRotate() (Rodrigues' formula)
- [x] Procedural terrain (FBM noise, biomes: grass, sand, stone, water)
- [x] Trees (trunk + randomized leaf canopy, colorful, clustered in forest zone)
- [x] Camera: Pitch(X)/Yaw(Y)/Roll(Z), Bird's Eye(B), Rotate around look-at(F), 4-viewport(V)
- [x] Lighting: Directional(1), Point(2), Spot(3), Emissive ores, Ambient(5)/Diffuse(6)/Specular(7), Master(L)
- [x] Day-night cycle (T)
- [x] Player character (hierarchical joints: head, body, arms, legs, faces movement dir)
- [x] Fan (G), Door (O), Clock with rotating hands
- [x] Drivable MineCar (arrow keys)
- [x] Torches with flickering point lights
- [x] Curvy objects: Sphere, Cone, Bezier curve, Spline curve, Ruled surface
- [x] Stone ruins structure, 6 biome zones (A–F)
- [x] 3D voxel grid (80x80x16) — terrain stored as block types, rendered from grid
- [x] Block breaking (left-click) and placement (middle-click / Shift+right-click)
- [x] Ray casting (30 unit range) with wireframe block highlight
- [x] Hotbar with 9 block types (scroll to cycle, numpad 1-9 quick select)
- [x] HUD: crosshair, hotbar display, health bar, stamina bar (screen-space)
- [x] Player movement: WASD walks on terrain, Space jumps, gravity, ground collision
- [x] Camera modes (C key): Third-Person / First-Person / Free-Fly

---

## Phase 7: Block Placement & Breaking (Minecraft Core Mechanic)

This is the KEY feature that makes it feel like a real game. The player can look at a hex block, highlight it, and place/break blocks.

### 7A: Crosshair & Block Raycasting
- **Crosshair overlay**: Draw a simple `+` at screen center using 2 thin quads (rendered in screen space, no depth test)
- **Ray casting**: From camera position along camera front direction, step through the hex grid to find which block the player is looking at
  - Use DDA-style ray march: step in small increments (0.1 units) along ray, convert world pos → hex grid col/row/height, check if a solid block exists there
  - Max ray distance: 8 hex units (like Minecraft's reach)
- **Block highlight**: When a block is found, draw a wireframe hex outline around it (slightly larger, bright white/yellow, `GL_LINE` mode)
  - This tells the player which block they're targeting

### 7B: Block Breaking
- **Left mouse click**: Remove the targeted block
  - Store the world as a 3D grid: `blockGrid[col][row][height]` — each cell is a block type (AIR, GRASS, DIRT, SAND, STONE, WATER, WOOD, LEAF, etc.)
  - On click, set `blockGrid[target] = AIR`
  - The terrain generation at startup fills this grid; after that, only the grid is used for rendering
- **Breaking animation (optional)**: Block flashes 3 times over 0.3s before disappearing

### 7C: Block Placement
- **Right mouse click** (when NOT in camera-look mode): Place a block on the face adjacent to the targeted block
  - Determine which face of the hex the ray hit (top, bottom, or one of 6 sides)
  - Place new block in the neighboring cell on that face
- **Block type selection**: Number keys or scroll wheel to cycle through a hotbar of block types
  - Available blocks: Grass, Dirt, Sand, Stone, Wood (log), Leaf, Ore (diamond), Ore (gold), Water, Glass
  - Currently selected block shown in HUD hotbar

### 7D: World Storage as 3D Voxel Grid
- **Data structure**: `int blockGrid[GRID_W][GRID_D][GRID_H]` where:
  - `GRID_W` = 80 columns (col -40 to 39)
  - `GRID_D` = 80 rows (row -40 to 39)
  - `GRID_H` = 16 height layers (y = 0 to 15)
  - Each cell = block type enum (AIR=0, GRASS=1, DIRT=2, SAND=3, STONE=4, WATER=5, WOOD=6, LEAF=7, ORE_DIAMOND=8, ORE_GOLD=9, GLASS=10)
- **Terrain init**: At startup, iterate over the grid and fill based on current biome/noise logic, storing into blockGrid instead of computing on-the-fly
- **Rendering change**: `renderTerrain()` reads from blockGrid instead of recomputing noise each frame
  - Only render non-AIR blocks
  - (Optional optimization: skip faces between two adjacent solid blocks — but not required for now)

### Key Bindings
| Key | Action |
|-----|--------|
| Left Click | Break targeted block |
| Right Click | Place selected block (when not in camera-look mode) |
| Scroll Wheel | Cycle hotbar selection |
| 1-9 (numpad row) | Quick-select hotbar slot |

---

## Phase 8: HUD & Game Interface

Make it look like a real game with on-screen UI elements.

### 8A: Hotbar (Bottom Center)
- 9 slots rendered as semi-transparent rectangles at bottom of screen
- Each slot shows a small colored hex representing the block type
- Currently selected slot has bright border/highlight
- Rendered in screen space (orthographic projection, no depth test)

### 8B: Health & Stamina Bars (Top Left)
- **Health bar**: Red bar, 10 units (purely decorative for now, or decreases when falling from height)
- **Stamina/mana bar**: Green bar below health
- Rendered as colored quads in screen space

### 8C: Minimap (Bottom Right Corner)
- Top-down view of terrain in a small square (200x200 px)
- Each hex rendered as a tiny colored dot based on biome
- Player position shown as white dot
- Car position shown as red dot
- Rendered using a separate small viewport + orthographic projection looking straight down

### 8D: Crosshair
- White `+` at screen center, always visible
- 2 thin quads, ~20px each direction
- Rendered last, with depth test disabled

---

## Phase 9: Player as First/Third Person Character

### 9A: Third-Person Camera Mode (default)
- Camera follows behind and above the player character
- Player visible on screen (like reference top-right image)
- Camera offset: ~5 units behind, ~3 units above player
- Player always faces direction of movement
- Toggle: `P` key switches between first-person and third-person

### 9B: First-Person Camera Mode
- Camera is at player's head position
- Player model hidden (or only arms visible)
- Crosshair visible, block placement/breaking works from eye level

### 9C: Player Movement (on ground)
- WASD moves the player character on the terrain (not flying)
- Player snaps to terrain height (walks on top of blocks)
- Walking animation: arms and legs swing (already have hierarchical joints)
- Jump: Spacebar — simple parabolic arc, land on blocks
- Gravity: Player falls if no block below

### 9D: Camera Modes Preserved
- `C` key: Cycle between Free-Fly camera (current WASD), Third-Person follow, First-Person
- Free-Fly mode: WASD moves camera freely (current behavior, for inspection/building)
- In player modes: WASD moves the player, mouse controls camera angle around player

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

## IMPLEMENTATION ORDER (recommended)

1. **Phase 7** (Block system) — This is the biggest change. Convert terrain to voxel grid, add raycasting, placement/breaking.
2. **Phase 8** (HUD) — Crosshair, hotbar, health bars. Makes it look like a game immediately.
3. **Phase 10** (World visuals) — Better trees, terrain, water, sky. Visual impact.
4. **Phase 9** (Player modes) — Third/first person, ground movement, jump.
5. **Phase 11** (Course requirements) — Fractal trees, birds, collision, wine glass, Gouraud/Phong, textures.
6. **Phase 12** (Polish) — Performance, save/load, sound.

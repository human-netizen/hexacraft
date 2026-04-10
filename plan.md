# HexaCraft — Complete Implementation Plan

Based on `req.md` (full Minecraft-style feature set) and `gfx_req.md` (course assignment requirements). The world uses hexagonal prisms instead of cubes. Player spawns into a procedurally generated voxel world with first-person gameplay.

---

## COMPLETED (Phases 1–9)

- [x] Hexagonal prism geometry, GLFW + GLAD + shader pipeline
- [x] Custom myRotate() (Rodrigues' formula) — **[COURSE REQ]**
- [x] Procedural terrain (FBM noise, biomes: grass, sand, stone, water)
- [x] Trees (trunk + randomized leaf canopy, colorful, clustered in forest zone)
- [x] Camera: Mouse look, Pitch(X)/Yaw(Y)/Roll(Z), Bird's Eye(B), Rotate around look-at(F), 4-viewport(V) — **[COURSE REQ]**
- [x] Flying simulator movement: WASD + E(up) + R(down) — **[COURSE REQ]**
- [x] Lighting: Directional(1), Point(2), Spot(3), Emissive ores — **[COURSE REQ]**
- [x] Ambient(5)/Diffuse(6)/Specular(7) toggles, Master(L) — **[COURSE REQ]**
- [x] Day-night cycle (T)
- [x] Player character (hierarchical joints: head, body, arms, legs — complex hierarchical movement) — **[COURSE REQ]**
- [x] Rotating fan (G) — **[COURSE REQ]**
- [x] Door open/close (O toggle) — **[COURSE REQ]**
- [x] Clock with rotating hands — **[COURSE REQ: Taj sir]**
- [x] Drivable MineCar (arrow keys) — dynamic equipment simulation — **[COURSE REQ]**
- [x] Curvy objects: Sphere, Cone, Bezier curve, Spline curve, Ruled surface — **[COURSE REQ]**
- [x] Stone ruins structure, 6 biome zones (A–F)
- [x] 3D voxel grid (80x80x16) — terrain stored as block types, rendered from grid
- [x] Block breaking (left-click) and placement (right-click)
- [x] Ray casting (30 unit range) with wireframe block highlight
- [x] Hotbar with 9 block types (scroll to cycle, numpad 1-9 quick select)
- [x] HUD: crosshair, hotbar display, health bar, stamina bar (screen-space)
- [x] Player movement: WASD walks on terrain, Space jumps, gravity, ground collision
- [x] Camera modes (C key): Third-Person / First-Person / Free-Fly
- [x] Direct mouse look (cursor captured, like Minecraft)
- [x] Print all controls to console — **[COURSE REQ]**

---

## REMAINING COURSE REQUIREMENTS (MUST DO)

These items from `gfx_req.md` are **not yet implemented** and must be completed:

| Requirement | Status | Planned Phase |
|-------------|--------|---------------|
| Textures: Simple texture without surface color | NOT DONE | Phase 10 |
| Textures: Blended texture with surface color | NOT DONE | Phase 10 |
| Textured curvy objects (sphere + cone) | NOT DONE | Phase 10 |
| Wine glass from "wine glass making program" | NOT DONE | Phase 10 |
| Fractal tree leaves | NOT DONE | Phase 10 |
| Random motion for birds (spline path) | NOT DONE | Phase 10 |
| Windows that open/close | NOT DONE | Phase 10 |
| Collision detection (Masud sir emphasis) | NOT DONE | Phase 10 |
| Gouraud vs Phong shading toggle | NOT DONE | Phase 10 |
| Light source motion (tied to sun position) | PARTIAL (day-night exists, need sun object driving light dir) | Phase 11 |

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
| H | Toggle Gouraud/Phong shading *(Phase 10)* |
| ESC | Quit |

---

## Phase 10: Course Requirements Completion (PRIORITY)

*All items in this phase are required by `gfx_req.md`. Must be done first.*

### 10A: Textures **[COURSE REQ]**
- **Simple texture without surface color**: Load a texture (e.g., stone brick pattern) using stb_image.h. Apply to ruins blocks with UV coordinates. Fragment shader samples texture only, no blending with vertex color.
- **Blended texture with surface color**: Apply texture to ground/terrain blocks, blend with biome surface color. Color computed on both vertex and fragment shader stages.
- **Textured curvy objects**: Apply texture to the existing sphere (globe/crystal ball) and cone (tent). Requires UV coordinates in curvy object vertex data and sampler2D in fragment shader.
- Needs: stb_image.h integration, UV coords in vertex struct, texture sampler uniform, conditional texture/color modes in fragment shader.

### 10B: Wine Glass Object **[COURSE REQ]**
- Use the "wine glass making program" to export profile points
- Generate surface of revolution from the profile curve (rotate 2D profile around Y-axis, connect strips)
- Place on a pedestal/table near the clock in Zone E (col=-16, row=15)
- Apply material properties (glass-like specular, slight transparency if possible)

### 10C: Fractal Tree Leaves **[COURSE REQ]**
- L-system or recursive branching algorithm for tree canopy structure
- Instead of random blob, branches split and grow leaves at tips
- At least 1 showcase fractal tree near spawn (col=0, row=12, Zone A)
- Demonstrates procedural generation and recursion — high academic value

### 10D: Flying Birds with Random Spline Motion **[COURSE REQ]**
- Bird model: Small body (1 hex) + 2 wing hexes that flap up/down (sine wave)
- Path: Catmull-Rom spline through 5–8 random waypoints in the sky
- When reaching end of spline, generate new random waypoints and loop
- 2–3 birds at different heights/speeds, visible circling the world
- Demonstrates: spline curves, random motion algorithms, animation

### 10E: Windows That Open/Close **[COURSE REQ]**
- Add window objects to existing structures (ruins, castle, or a new building)
- Key toggle (e.g., `P` key) to open/close windows with smooth animation
- Window = glass-like panel on a hinge, rotates open like the door

### 10F: Collision Detection **[COURSE REQ — Masud sir emphasis]**
- **Player–terrain**: Player cannot walk through solid blocks (extend current ground snap to full AABB)
- **Player–objects**: Cannot walk through fan, door, ruins walls, wine glass pedestal
- **Car–terrain**: Car cannot drive through blocks, follows terrain height properly
- **Player–player model**: Bounding box for the hex player model
- Method: AABB (axis-aligned bounding box) per entity/object. Before moving, check if new position collides with any solid block or object AABB. Resolve by pushing back.

### 10G: Gouraud vs Phong Shading Toggle **[COURSE REQ]**
- Key `H`: Toggle between Gouraud (per-vertex) and Phong (per-fragment) shading
- **Gouraud**: Compute full lighting in vertex shader, pass final color to fragment shader
- **Phong**: Current approach (lighting computed in fragment shader)
- Requires either two shader programs or a uniform flag to switch lighting computation location
- Best showcased in 4-viewport mode: e.g., Gouraud in two viewports, Phong in two

---

## Phase 11: Sky, Atmosphere & Sun/Moon **[PARTIAL COURSE REQ]**

### 11A: Sky Rendering
- **Sky gradient**: Blue at zenith → lighter at horizon (fragment shader based on Y direction)
- Colors shift with day-night cycle: orange at sunrise/sunset, dark blue at night
- Render as a large inverted dome or fullscreen background quad

### 11B: Sun & Moon **[COURSE REQ: light source motion tied to sun]**
- Sun: Bright yellow billboard quad/sphere arcing across sky during day
- Moon: Gray/white billboard during night
- **Sun position directly drives the directional light direction** (fulfills "tie dynamic lighting projections to the position of the sun")
- Smooth transition at dawn/dusk with color temperature shift

### 11C: Stars at Night
- Small white dots scattered on sky dome
- Only visible when dayFactor < 0.3 (fade in/out)
- Slowly rotate to simulate passage of time

### 11D: Clouds
- Flat white hex clusters floating at y=20–25
- Slowly drift with time (offset by `time * cloudSpeed`)
- Semi-transparent (alpha blending)

### 11E: Fog
- Distance-based fog: blend fragment color toward sky color based on camera distance
- Fog density varies by time of day (thicker at dawn/dusk, clear at noon)
- Uniform `fogColor` and `fogDensity` in fragment shader
- Hides far terrain edges, adds depth

---

## Phase 12: Enhanced World & Terrain

### 12A: Bigger, Better Trees
- **Thick trunks**: 2–3 hex columns wide, 5–7 hex tall
- **Massive canopies**: 5–6 hex radius irregular cloud of leaf blocks
- **Colors**: Random per-tree from palette [red, orange, yellow, cyan, blue, purple, bright green, dark green]
- **Forest zone (Zone B)**: Dense cluster of 15–25 large trees forming a canopy
- **Standalone trees**: 2–3 large trees scattered in plains as landmarks

### 12B: Terrain Improvements
- **Stepped terrain**: Distinct height steps like Minecraft. Flat tops with cliff edges.
  - Grass biome: Rolling hills 1–4 hex tall
  - Sand biome: Flat, height 0–1
  - Stone biome: Elevated plateaus 3–5 hex tall with cliff edges
- **Dirt layers**: Below grass top, 2–3 layers of dirt blocks visible on cliff sides
- **Larger world**: Extend grid beyond 80x80 if performance allows (100x100 or chunk-based)

### 12C: Water Enhancements
- Water blocks at height 0–1 forming ponds/lakes (3–5 clusters)
- Semi-transparent blue (alpha blending, render after opaque blocks)
- Sine-wave vertex Y offset on water top faces for gentle wave animation
- Specular highlight on water surface that follows sun position

### 12D: Ruins Enhancement
- Bigger structure: 8+ pillars, connecting beams, partial walls, broken sections
- Some blocks missing for ruined look
- Vines (green leaf blocks on sides)
- Placed in Zone D, visible from far away

---

## Phase 13: Player Physics & Survival

### 13A: Full AABB Player Physics
- AABB collision against the voxel world (all 6 directions, not just ground)
- Gravity: downward acceleration each frame, resolve by pushing up if intersecting ground
- Jumping: if grounded, set upward velocity (already done, refine)
- Swimming: detect water blocks, reduce gravity, allow upward movement on Space
- Sprinting: Hold Shift to move 1.5x faster, drains stamina. Slight FOV increase.

### 13B: Fall Damage
- Damage proportional to fall distance beyond 3-block threshold
- Track fall height using existing gravity/jump system
- Reduce health on impact, screen flash/shake feedback

### 13C: Health & Hunger System
- **Health**: 10 hearts (20 HP). Damage from falls, mob attacks, drowning, starvation. Regenerate when hunger > 17/20.
- **Hunger**: 10 drumsticks (20 hunger points). Decreases with sprinting, jumping, attacking. Eating food restores hunger. Low hunger prevents health regen.
- **HUD**: Health hearts + hunger bar displayed alongside existing health/stamina bars

### 13D: Death & Respawn
- Death screen overlay ("You Died!") when health reaches 0
- Respawn at spawn point with empty inventory
- Brief invincibility after respawn

---

## Phase 14: Inventory & Crafting

### 14A: Full Inventory System
- 36 slots (9 hotbar + 27 storage grid)
- Open with `I` key, rendered as orthographic overlay
- Click-to-swap item management between slots
- Items have stack counts (max 64 per slot)
- Show item icons (colored hex blocks) from block types

### 14B: Crafting System
- 3x3 grid-based crafting UI as orthographic overlay
- Open with crafting table interaction or a key
- Recipe lookup: specific grid patterns produce specific items
- Core recipes:
  - Logs → Planks
  - Planks → Sticks
  - Planks → Crafting Table
  - Sticks + Planks/Stone/Iron → Tools (pickaxe, axe, sword, shovel)
  - Coal + Stick → Torch
  - Stone → Furnace

### 14C: Furnace
- Place furnace block, right-click to open smelting UI
- Fuel slot (coal/logs) + input slot → output slot
- Smelting: raw iron → iron ingot, raw food → cooked food, sand → glass

### 14D: Tool Durability & Mining Speed
- Tools degrade with use, small durability bar under hotbar item
- Different blocks take different time to break (hold mouse button)
- Correct tool type speeds up mining (pickaxe for stone, axe for wood)
- Breaking animation: progressive darkening / cracking overlay on target block (4–5 stages)

### 14E: Item Drops & Pickup
- Broken blocks drop small rotating 3D hex items
- Walk over items to pick up into inventory (proximity check)
- Items bob up and down with small rotation

---

## Phase 15: Mobs & Combat

### 15A: Passive Mobs
- Hex-shaped animals: chickens, pigs, sheep (body + legs + head from hex prisms)
- AI state machine: idle → wander → flee (when attacked)
- Random walk: pick direction, walk 2–5 seconds, pause, repeat. Avoid cliff edges.
- Drop raw food/materials on death
- 2–4 passive mobs spawn in grass/forest biomes

### 15B: Hostile Mobs
- **Zombie**: Walks toward player within 16 blocks. Melee attack (3 HP damage). 20 HP. Drops rotten flesh.
- **Skeleton**: Approaches player, shoots arrows at 8–15 block range. 20 HP. Drops bones + arrows.
- Spawn rules: spawn on solid blocks with low light level, at least 24 blocks from player, at night or in dark areas
- Torches create safe zones (light level > 7 prevents spawning)

### 15C: Combat System
- Melee attack: left-click on mob with sword/tool
- Damage = weapon damage value, knockback on hit
- Attack cooldown (0.625s for sword)
- Damage flash: tint mob red for 0.3s on hit
- Death animation: entity falls over, fades, drops items

### 15D: Pathfinding
- Simple A* on the hex block grid for ground mobs
- Nodes = (col, row, height) positions. Check walkable: solid block below, air at feet level.
- Heuristic: hex distance. Limit search to 200 nodes.
- Recalculate path every 1–2 seconds within 32 blocks of player.

---

## Phase 16: World Generation Enhancements

### 16A: Cave Generation
- 3D Perlin/simplex noise to carve connected cave systems underground
- Threshold FBM noise at multiple octaves for air pockets in voxel grid
- Caves connect into networks; some open to surface
- Torches/ores spawn inside caves for exploration incentive

### 16B: Ore Veins
- Clustered ore blocks at specific depth ranges using noise-seeded generation
- Diamond only deep, gold mid-depth, iron everywhere
- Distinct colors per ore type; emissive glow for rare ores
- Larger veins (3–8 blocks) instead of scattered singles

### 16C: Underground Dungeons
- Small pre-built rooms underground (8x8 to 12x12)
- Stone brick walls, mob spawner block (emissive), chest with loot
- 2–3 dungeons per world, connected to cave system

### 16D: Villages/Settlements
- Clusters of 3–5 small hex huts on flat terrain in grass biome
- Each hut: 5x5 walls, door opening, wood roof, torch inside
- Village square with well (stone ring + water block)
- Procedural placement with spacing rules

### 16E: Biome-Specific Structures
- **Desert pyramid** in sand biome: large sandstone pyramid with hidden interior
- **Stone watchtower** in stone biome: tall narrow tower (3x3, 15 high)
- **Jungle treehouse** in forest biome: platform in treetops
- One unique structure per biome zone

### 16F: Snow Biome
- 7th biome with white terrain surface (BLOCK_SNOW type)
- Ice blocks on water surfaces (BLOCK_ICE)
- Snowfall particle effect in the biome area
- Igloos as biome-specific structure

### 16G: Ravines & Floating Islands
- **Ravines**: Vertical slices through terrain, 3–5 blocks wide, 8–12 deep, exposed ores on walls
- **Floating islands**: Small terrain chunks 15–20 blocks above surface, unique vegetation, waterfall edges

### 16H: Water/Lava Flow
- Water source blocks spread downward and to adjacent lower/same-level empty blocks
- Simple cellular automaton rule per game tick, limited range (7 blocks from source)
- Lava variant: slower spread, emissive, damages player on contact

---

## Phase 17: Visual Effects & Polish

### 17A: Particle System
- General-purpose emitter: position, velocity, lifetime, color, size
- Billboard quads facing camera
- Used for: block break fragments, torch smoke/sparks, rain, snow, dust, XP orbs
- Particle pooling for performance (pre-allocate max particles)

### 17B: Weather System
- **Rain**: Blue line particles falling + darkened sky + increased fog density
- **Snow**: Slow white particles, accumulates as snow layer (in snow biome)
- **Thunderstorm**: Rain + occasional full-screen flash (lightning) + boosted directional light
- Weather changes randomly over time or on key press

### 17C: Block Break & Place Animation
- **Breaking**: 4–5 stage cracking overlay. Progressive darkening + crack pattern blend while holding mouse.
- **Placing**: Brief scale-up animation (block grows from 0.3 to 1.0 over 0.15 seconds)
- Break particles: 8–12 small colored fragments burst outward matching block color

### 17D: Shadow Mapping
- Render scene from sun's perspective into depth framebuffer (2048x2048 shadow map)
- Sample shadow map in fragment shader to determine if fragment is in shadow
- Shadow intensity varies with day/night (no shadows at night)
- Bias to reduce shadow acne — classic CG technique, high academic value

### 17E: Bloom / Glow Effect
- Render bright fragments (torches, emissive ores, lava) to separate brightness buffer
- Gaussian blur pass on brightness buffer
- Additive blend with main scene — impactful at night

### 17F: Ambient Occlusion (Per-Vertex)
- For each block face vertex, count adjacent solid blocks in 3 neighboring directions
- Darken vertex color based on occlusion count (0 = full bright, 3 = darkest)
- Smooth interpolation across face gives soft contact shadows
- Massive visual upgrade to voxel look

### 17G: Torch Flame & Swaying Vegetation
- **Torch flame**: Small flickering billboard sprite at torch top, oscillating size/brightness/color
- **Swaying leaves/grass**: Sine offset on top vertices of leaf/grass blocks, driven by `time + worldPos`

### 17H: Screen Effects
- **Screen shake**: Brief decaying random offset to view matrix on damage/explosions
- **Underwater**: When camera below water level, blue tint overlay + sine UV distortion + increased fog

---

## Phase 18: Quality of Life & Performance

### 18A: Performance Optimization
- **Frustum culling**: Only render blocks within camera view frustum
- **Face culling**: Skip internal faces between adjacent solid blocks (~80% triangle reduction)
- **Chunk system**: Divide world into 16x16 chunks, single VBO each, rebuild only on change

### 18B: Save/Load
- Serialize blockGrid to binary file
- Save player position, inventory, health, hunger
- Load on startup if save file exists — player builds persist across sessions

### 18C: Sound (Optional)
- Background ambient music (loop with crossfade)
- Block break/place sound effects per material
- Walking footstep sounds (vary by surface)
- 3D positional audio using OpenAL (if time allows)

### 18D: UI Polish
- **Pause menu**: ESC to pause, overlay with Resume/Quit. Freezes game loop, shows cursor.
- **Debug overlay (F3)**: FPS, coordinates, biome, block type, light level, facing direction
- **Death screen**: "You Died!" overlay with respawn option
- **Minimap**: Small top-down orthographic render of terrain around player in screen corner

---

## BLOCK TYPES

| Block | Transparent | Light | Hardness | Tool | Drop |
|-------|-------------|-------|----------|------|------|
| Air | yes | 0 | — | — | — |
| Stone | no | 0 | 1.5 | pickaxe | cobblestone |
| Cobblestone | no | 0 | 2.0 | pickaxe | cobblestone |
| Dirt | no | 0 | 0.5 | shovel | dirt |
| Grass Block | no | 0 | 0.6 | shovel | dirt |
| Sand | no | 0 | 0.5 | shovel | sand |
| Oak Log / Wood | no | 0 | 2.0 | axe | oak log |
| Oak Planks | no | 0 | 2.0 | axe | oak planks |
| Leaves | yes (partial) | 0 | 0.2 | shears | — |
| Glass | yes | 0 | 0.3 | — | nothing |
| Water | yes | 0 | — | — | — |
| Lava | yes (emissive) | 15 | — | — | — |
| Coal Ore | no | 0 | 3.0 | pickaxe | coal |
| Iron Ore | no | 0 | 3.0 | pickaxe | raw iron |
| Gold Ore | no | 0 | 3.0 | pickaxe | raw gold |
| Diamond Ore | no | 0 | 3.0 | pickaxe | diamond |
| Torch | yes | 14 | 0.0 | any | torch |
| Crafting Table | no | 0 | 2.5 | axe | crafting table |
| Furnace | no | 0 | 3.5 | pickaxe | furnace |
| Snow | no | 0 | 0.5 | shovel | snowball |
| Ice | yes | 0 | 0.5 | pickaxe | — |
| Bedrock | no | 0 | ∞ | — | — |

---

## KEY POSITIONS (hex grid units)

| Object | Grid Position | Location |
|--------|--------------|----------|
| Player spawn | col=3, row=5 | South of castle entrance |
| Castle | col=-20..25, row=12..57 | Main structure near spawn |
| Car | col=-5, row=7 | Parked near castle entrance |
| Fan | col=-5, row=18 | Castle great hall |
| Door | col=2, row=27 | Castle hall→throne doorway |
| Clock | col=10, row=34 | Castle throne room |
| Sphere/pedestal | col=-10, row=48 | Castle library |
| Cone/tent | col=12, row=5 | Outside castle entrance |
| Bezier arch | col=57, row=-12 | Near ruins |
| Spline fence | col=-5, row=8 | Path to castle |
| Ruled surface | col=-25, row=30 | West of castle |
| Ruins | col=55, row=-10 | East, explorable |
| Forest | col=-60..-25, row=20..55 | West of castle |
| Water pond 1 | col=35, row=-10 | Southeast |
| Water pond 2 | col=-8, row=3 | South |
| Water pond 3 | col=-40, row=35 | Forest area |

---

## IMPLEMENTATION ORDER (remaining)

1. **Phase 10** — Course requirements completion (textures, wine glass, fractal trees, birds, windows, collision, Gouraud/Phong). **HIGHEST PRIORITY — required for grading.**
2. **Phase 11** — Sky, sun/moon, fog, clouds (completes light-source-tied-to-sun course req).
3. **Phase 12** — Enhanced world visuals (better trees, terrain, water, ruins).
4. **Phase 13** — Player physics & survival (full collision, fall damage, health/hunger, death/respawn).
5. **Phase 14** — Inventory & crafting (full inventory, crafting grid, furnace, tools, item drops).
6. **Phase 15** — Mobs & combat (passive/hostile mobs, AI, combat, pathfinding).
7. **Phase 16** — World generation (caves, ores, dungeons, villages, structures, snow biome, ravines).
8. **Phase 17** — Visual effects (particles, weather, shadows, bloom, AO, animations).
9. **Phase 18** — QoL & performance (optimization, save/load, sound, UI polish).

---

## RENDERING PIPELINE

```
Per Frame:
1. Update delta time
2. Process input (camera movement, block interaction, UI)
3. Update player physics (gravity, collision, AABB)
4. Update entities (mob AI, movement, collision)
5. Update world (chunk loading, water/lava flow, light propagation)
6. Rebuild dirty chunk meshes (limit N per frame)
7. Compute VP matrix, extract frustum planes
8. --- Begin Rendering ---
9. Clear framebuffer
10. Render sky (gradient dome, sun, moon, stars, clouds)
11. Render opaque terrain (front to back for early Z)
12. Render transparent blocks (water, glass — back to front, alpha blend)
13. Render entities (mobs, item drops, birds)
14. Render scene objects (fan, door, clock, car, wine glass, curvy objects)
15. Render particles (torch flames, block break, weather)
16. Render block highlight wireframe
17. Render HUD (crosshair, hotbar, health, hunger, stamina) — orthographic
18. Render inventory/crafting UI if open — orthographic
19. Swap buffers
```

---

## CONTROLS SUMMARY (planned additions)

| Key | Action | Phase |
|-----|--------|-------|
| H | Toggle Gouraud/Phong shading | 10 |
| P | Toggle windows open/close | 10 |
| I | Open/close inventory | 14 |
| Shift | Sprint | 13 |
| F3 | Debug overlay | 18 |
| Tab | Toggle weather | 17 |

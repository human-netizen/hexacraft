# HexaCraft — Minecraft Features to Add

Features we can add to HexaCraft, organized by category. Prioritized for a university CG course project (OpenGL 3.3, single-file C++).

---

## 1. Core Gameplay Mechanics

| Feature | Description |
|---------|-------------|
| **Crafting System** | Grid-based crafting UI (2x2 or 3x3) as an orthographic overlay. Combine raw materials to produce tools/items. |
| **Inventory System** | Full inventory grid (beyond hotbar), opened with a key. Click-to-swap item management rendered as overlay. |
| **Tool Durability** | Tools degrade with use and break. Small durability bar under each hotbar item. |
| **Mining Speed by Tool** | Different blocks take different time to break. Correct tool speeds it up. Breaking animation (progressive color/crack). |
| **Hunger/Food System** | Hunger bar alongside health. Depletes over time/sprinting. Food restores it. Low hunger prevents health regen. |
| **XP/Leveling** | XP orbs drop from breaking blocks or killing mobs. XP bar on HUD. Levels unlock perks. |
| **Simple Mobs** | Passive animals (random walk AI). Hostile mobs (chase player at night). Hex-shaped entities with state machines (idle/chase/attack). |
| **Mob Spawning Rules** | Hostile mobs spawn in darkness or at night. Torches create safe zones. |
| **Combat System** | Melee attack with cooldown, damage to mobs, knockback on hit. |
| **Item Drops** | Broken blocks drop rotating 3D pickup items. Mobs drop loot on death. |
| **Fall Damage** | Damage proportional to fall distance beyond a threshold. Track fall height from existing gravity. |
| **Respawn** | On death, respawn at spawn point. Simple death screen overlay. |
| **Sprinting** | Hold key to move faster, drains stamina. Slight FOV increase for speed feel. |

---

## 2. World Generation Features

| Feature | Description |
|---------|-------------|
| **Cave Generation** | 3D Perlin/simplex noise to carve connected cave systems underground. Threshold noise for air pockets. |
| **Ore Veins** | Clustered ore blocks at specific depth ranges using noise-seeded generation. |
| **Underground Dungeons** | Small pre-built rooms underground with spawner block and chest. |
| **Villages/Settlements** | Clusters of small hex huts on flat terrain. Each has door, roof, torch. Procedurally spaced. |
| **Biome Structures** | Desert pyramids, temples, igloos — one unique structure per biome. |
| **Ravines/Canyons** | Vertical terrain slices using elongated noise. Dramatic visual feature. |
| **Floating Islands** | Small terrain chunks above main surface in a sky zone. Same noise with Y offset. |
| **Underwater Features** | Coral, sunken ruins, treasure chests below water level. |
| **Water/Lava Flow** | Source blocks spread downward/outward using cellular automaton rules each tick. |
| **Snow Biome** | White surface, snowfall particles, ice blocks on water. 7th biome. |

---

## 3. Visual Effects

| Feature | Description |
|---------|-------------|
| **Particle System** | General-purpose emitter for: block break fragments, torch smoke, rain/snow, dust, XP orbs, explosions. Billboard quads with position/velocity/lifetime. |
| **Weather (Rain/Snow/Thunder)** | Rain = blue line particles + dark sky. Snow = slow white particles. Thunder = screen flash + boosted light. |
| **Fog** | Distance-based fragment blending toward sky color. Hides far terrain, adds atmosphere. |
| **Underwater Effect** | Blue tint + wave distortion + reduced visibility when camera submerged. |
| **Block Break Animation** | Progressive cracking overlay (4-5 stages) while holding break button. |
| **Block Place Animation** | Brief scale-up (block grows from 0 to full size over frames). |
| **Skybox / Sky Dome** | Procedural or textured sky with gradient shifting by day/night cycle. Cloud layers at high Y. |
| **Stars at Night** | White dots on sky dome, slowly rotating. Visible when dayFactor < 0.3. |
| **Sun and Moon** | Billboard quads arcing across sky, driving directional light direction. |
| **Animated Water** | Sine-wave vertex offset on water surfaces for gentle undulation. |
| **Torch Flame Visual** | Flickering billboard sprite at torch top, oscillating size/brightness. |
| **Swaying Vegetation** | Grass/flower blocks sway via sine offset on top vertices over time. |
| **Screen Shake** | Brief decaying random camera offset on damage/explosions. |
| **Shadow Mapping** | Render from sun's perspective into depth map, cast real shadows. Classic CG technique. |
| **Bloom/Glow** | Bright sources (torches, lava, ore) bleed light via post-process blur on bright fragments. |
| **Ambient Occlusion** | Darken block faces in concave corners by averaging neighbor light values. Per-vertex computation. |
| **Procedural Textures** | Generate wood grain, stone noise, grass patterns in shader using noise functions. |

---

## 4. Interactive / Redstone-Like Mechanics

| Feature | Description |
|---------|-------------|
| **Pressure Plates** | Block detects player standing on it, toggles signal. Can auto-open doors. |
| **Levers/Buttons** | Clickable toggle blocks. Lever rotates visually. Triggers doors, lights, pistons. |
| **Wiring/Signal Propagation** | Redstone-like block that propagates on/off via BFS. Signal weakens over 15 blocks. |
| **Pistons** | Extends one block when powered, pushes adjacent block. Dynamic voxel grid update. |
| **TNT/Explosions** | Activated TNT destroys blocks in sphere radius after fuse delay. Particles + grid modification. |
| **Minecart Rails** | Rail block type. Car snaps to and follows rail paths. Curves and slopes. |
| **Chests/Storage** | Clickable block opens inventory overlay for item storage. Per-chest instance. |
| **Trapdoors** | Horizontal doors that open downward on click. For rooftops and traps. |
| **Bounce Pads** | Special block launches player upward. Impulse to vertical velocity. |
| **Light-Emitting Blocks** | Placeable lanterns/glowstone as portable point light sources. |

---

## 5. Quality of Life

| Feature | Description |
|---------|-------------|
| **Minimap** | Small top-down orthographic render of terrain around player in screen corner. |
| **Debug Overlay (F3)** | FPS, coordinates, biome, block info, light level, facing direction. |
| **Screenshot Key** | Save framebuffer to PNG via stb_image_write on keypress. |
| **Pause Menu** | Overlay with Resume/Settings/Quit. Freezes game loop, shows cursor. |
| **Settings Menu** | Render distance, FOV, sensitivity, Gouraud/Phong toggle. |
| **Smooth Lighting** | Average light values at block corners for soft lighting transitions. |
| **Render Distance** | Only render blocks within configurable radius. Demonstrates culling. |
| **Frustum Culling** | Skip blocks/chunks outside camera view. Practical optimization. |
| **Chunk Meshing** | Group blocks into 16x16x16 chunks with single VBO each. Rebuild on change only. |
| **Face Culling** | Don't generate mesh faces between adjacent solid blocks. ~80% triangle reduction. |
| **Creative/Survival Toggle** | Creative = infinite blocks + flight. Survival = health + hunger + inventory. |
| **Save/Load World** | Serialize blockGrid to binary file. Persist player builds across sessions. |
| **Undo/Redo** | Stack of recent block changes. Key to undo last placement/break. |
| **Block Name Tooltip** | Show selected and targeted block type name on HUD. |

---

## Priority Recommendations (CG Course Impact)

These will impress most in a graphics course demo while being feasible:

1. **Particle System** — broadly useful, visually impactful, demonstrates instanced rendering
2. **Cave + Ore Generation** — extends existing noise terrain gen naturally
3. **Shadow Mapping** — classic CG technique, high academic/demo value
4. **Ambient Occlusion (per-vertex)** — significant visual upgrade to voxel rendering
5. **Fog + Animated Water + Skybox** — cheap atmospheric effects that transform the look
6. **Simple Mob AI** — demonstrates animation, state machines, and collision
7. **Face Culling + Chunk Meshing** — demonstrates optimization knowledge
8. **Fractal Trees** — already planned, great CG theory (L-systems)
9. **Block Break Particles + Animation** — polishes existing mechanics
10. **TNT Explosions** — crowd-pleaser demo, tests dynamic voxel modification

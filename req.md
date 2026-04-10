# Minecraft Clone — Graphics Project Plan

## 1. Project Overview

A simplified Minecraft clone built with **C++ and Modern OpenGL (3.3+ Core Profile)**. The player spawns into a procedurally generated voxel world, can move around in first person, place/break blocks, craft basic items, fight mobs, and survive day/night cycles. The focus is on demonstrating core computer graphics techniques: transformations, lighting, texture mapping, camera systems, terrain generation, and real time rendering optimizations.

**Target Scope:** Playable survival sandbox with terrain generation, block interaction, basic inventory/crafting, mobs, and day/night cycle.

---

## 2. Tech Stack

| Component | Choice | Reason |
|---|---|---|
| Language | C++17 | Performance, direct OpenGL access |
| Graphics API | OpenGL 3.3 Core | Widely supported, no fixed pipeline |
| Windowing | GLFW 3.x | Cross platform window/input handling |
| GL Loader | GLAD | Loads OpenGL function pointers |
| Math | GLM | GLSL compatible math (vec3, mat4, etc.) |
| Image Loading | stb_image | Single header, loads PNG/JPG textures |
| Audio | OpenAL Soft + stb_vorbis | 3D positional audio for ambient/mob sounds |
| Text Rendering | FreeType2 | HUD text (health, coordinates, debug) |
| Noise | FastNoiseLite | Perlin/Simplex noise for terrain gen |
| Build System | CMake | Cross platform builds |
| Persistence | SQLite3 | Save world changes (delta storage like fogleman/Craft) |

---

## 3. Architecture

```
src/
├── main.cpp                 # Entry point, game loop
├── core/
│   ├── Window.cpp/.h        # GLFW window wrapper
│   ├── Shader.cpp/.h        # Shader compilation/uniform management
│   ├── Camera.cpp/.h        # FPS camera (position, yaw, pitch, projection)
│   ├── InputManager.cpp/.h  # Keyboard/mouse state
│   └── Timer.cpp/.h         # Delta time, fixed timestep
├── world/
│   ├── Block.cpp/.h         # Block type enum, properties (solid, transparent, hardness)
│   ├── Chunk.cpp/.h         # 16x256x16 chunk, mesh generation
│   ├── ChunkManager.cpp/.h  # Load/unload chunks around player, chunk map
│   ├── WorldGen.cpp/.h      # Terrain generation (noise layers, biomes, trees, caves)
│   └── WorldSave.cpp/.h     # SQLite persistence
├── renderer/
│   ├── ChunkRenderer.cpp/.h # Batch render visible chunks
│   ├── SkyRenderer.cpp/.h   # Skybox/skydome, sun/moon, day night cycle
│   ├── UIRenderer.cpp/.h    # Crosshair, hotbar, inventory screens
│   ├── TextRenderer.cpp/.h  # FreeType based text
│   └── ParticleSystem.cpp/.h# Block break particles, hit effects
├── player/
│   ├── Player.cpp/.h        # Position, health, hunger, inventory
│   ├── Physics.cpp/.h       # AABB collision, gravity, jumping
│   ├── Raycaster.cpp/.h     # Block selection (ray vs voxel grid, DDA algorithm)
│   └── Inventory.cpp/.h     # Slots, hotbar, crafting grid
├── entity/
│   ├── Entity.cpp/.h        # Base entity (position, velocity, AABB)
│   ├── Zombie.cpp/.h        # Hostile mob, pathfinding, attack
│   ├── Skeleton.cpp/.h      # Ranged hostile mob
│   ├── Pig.cpp/.h           # Passive mob, drops food
│   └── EntityManager.cpp/.h # Spawn, update, despawn logic
├── crafting/
│   ├── Recipe.cpp/.h        # Recipe matching (shaped/shapeless)
│   └── CraftingTable.cpp/.h # 2x2 and 3x3 crafting grid logic
├── audio/
│   └── AudioEngine.cpp/.h   # OpenAL wrapper, 3D sound
└── utils/
    ├── TextureAtlas.cpp/.h   # Load and manage block texture atlas
    ├── Frustum.cpp/.h        # Frustum culling math
    └── AABB.cpp/.h           # Axis aligned bounding box
```

---

## 4. Feature Breakdown (by Priority)

### Phase 1 — Core Rendering (Week 1~2)

**Goal:** Render a single chunk of textured blocks with a movable camera.

- [ ] GLFW window creation, OpenGL 3.3 context, GLAD initialization
- [ ] Shader system: compile vertex/fragment shaders, set uniforms (MVP matrices)
- [ ] FPS Camera: mouse look (yaw/pitch from cursor delta), WASD movement, projection matrix (perspective with configurable FOV)
- [ ] Block texture atlas: load a 16x16 tile atlas PNG using stb_image, compute UV coordinates per face
- [ ] Single chunk mesh generation:
  - Iterate all blocks in a 16x256x16 volume
  - For each solid block, check 6 neighbors; only emit faces where the neighbor is air or transparent
  - Pack vertex data: position (3 floats), UV (2 floats), normal direction (1 byte), ambient occlusion level (1 byte)
  - Upload to a single VBO + VAO per chunk
- [ ] Render the chunk with basic directional lighting in the fragment shader

**Graphics techniques demonstrated:** Model/View/Projection transformations, vertex buffer objects, vertex array objects, texture mapping, face culling (GL_CULL_FACE back faces), basic Phong/Lambert diffuse lighting.

### Phase 2 — World Generation & Chunk Management (Week 2~3)

**Goal:** Infinite feeling world with biomes, trees, and caves.

- [ ] **Terrain height map** using layered Simplex noise:
  - Continentalness noise (large scale land/ocean)
  - Erosion noise (mountain vs flat)
  - Peaks and valleys noise (local variation)
  - Combine to get a height value per (x,z) column
- [ ] **Biome system** (simplified): plains, forest, desert, snow, ocean. Biome determines surface block (grass/sand/snow), tree type, tree density
- [ ] **Cave generation**: 3D Simplex noise with a threshold carves out cave tunnels. Spaghetti cave style (two noise fields, caves where both are near zero)
- [ ] **Ore placement**: Place coal, iron, gold, diamond, redstone, lapis at appropriate Y ranges using scatter noise
- [ ] **Tree generation**: Place oak/spruce/birch trees on surface based on biome and density noise
- [ ] **Structure generation**: Simple villages (few houses with doors, chests) at random positions
- [ ] **Chunk manager**:
  - Store chunks in an `unordered_map<ChunkPos, Chunk>`
  - Load/generate chunks within render distance (8~12 chunks) around the player
  - Unload chunks beyond render distance + 2 buffer
  - Mesh rebuilding on a separate thread (or at least deferred across frames)
- [ ] **Frustum culling**: Compute the 6 frustum planes from the VP matrix, test each chunk's AABB before drawing

**Graphics techniques:** Procedural content generation, level of detail concept, view frustum culling, multi threaded mesh building.

### Phase 3 — Player Interaction (Week 3~4)

**Goal:** Break/place blocks, physics, basic survival.

- [ ] **Raycasting (DDA)**: Cast a ray from the camera into the voxel grid. Step through cells using a 3D DDA algorithm. Return the hit block position and the face normal (so we know which adjacent cell to place a new block in). Max range: 5 blocks.
- [ ] **Block breaking**: Left click starts a break timer based on block hardness and tool type. Show a crack overlay texture (progressive stages). On completion, remove the block, update chunk mesh, spawn item entity.
- [ ] **Block placing**: Right click places the selected block on the face hit by the ray. Check that the placement cell doesn't intersect the player's AABB.
- [ ] **Player physics**:
  - AABB collision detection against the voxel world
  - Gravity: apply downward acceleration each frame, resolve by pushing player up if intersecting ground
  - Jumping: if grounded, set upward velocity
  - Swimming: detect water blocks, reduce gravity, allow upward movement on jump key
  - Sprinting: hold Ctrl to increase speed, drains hunger faster
- [ ] **Health system**: 10 hearts (20 HP). Damage from falls (> 3 blocks), mob attacks, drowning, starvation. Regenerate when hunger > 17/20.
- [ ] **Hunger system**: 10 drumsticks (20 hunger). Decreases with sprinting, jumping, attacking. Eating food restores hunger.
- [ ] **Block highlight**: Render a wireframe cube around the block the player is looking at.

**Graphics techniques:** Ray/voxel intersection, AABB collision, wireframe overlay rendering, animated texture overlays.

### Phase 4 — Inventory, Crafting & Items (Week 4~5)

**Goal:** Functional inventory with crafting.

- [ ] **Inventory system**: 36 slots (9 hotbar + 27 storage). Each slot holds an item type + count (max 64 stack). Scroll wheel or 1~9 keys to select hotbar slot.
- [ ] **Inventory UI**: Render a 2D overlay when pressing E. Drag and drop items between slots (mouse interaction on 2D plane). Show item icons from the texture atlas.
- [ ] **Crafting**:
  - 2x2 grid always available in inventory
  - 3x3 grid when interacting with a crafting table block
  - Recipe matching: compare grid contents against a recipe list. Support shaped recipes (exact pattern) and shapeless recipes (any arrangement).
  - Core recipes to implement:
    - Logs → Planks
    - Planks → Sticks
    - Planks → Crafting Table
    - Sticks + Planks/Stone/Iron/Diamond → Tools (pickaxe, axe, sword, shovel, hoe)
    - Stone → Furnace
    - Iron → Bucket
    - Wool + Planks → Bed
    - Iron → Armor pieces
    - Diamond → Armor pieces
    - Coal + Stick → Torch
- [ ] **Furnace**: Place a furnace block, right click to open. Fuel slot (coal/logs) + input slot → output slot. Smelting: raw iron → iron ingot, raw food → cooked food, sand → glass.
- [ ] **Item entities**: When a block is broken or an entity dies, spawn a floating item entity that bobs up and down. Player walks over it to pick up (proximity check).
- [ ] **Tool durability**: Each tool has a durability counter. Using it decrements. At zero, the tool breaks.
- [ ] **Hotbar HUD**: Always visible at the bottom of the screen, shows the 9 hotbar items with the selected one highlighted.

**Graphics techniques:** 2D orthographic rendering for UI, texture atlasing for item icons, alpha blending for UI backgrounds.

### Phase 5 — Day/Night Cycle & Lighting (Week 5~6)

**Goal:** Dynamic sky, sun/moon, and block lighting.

- [ ] **Sky rendering**:
  - Skydome or skybox with gradient colors that shift based on time of day
  - Sun and moon as textured quads that orbit the sky
  - Stars at night (random point sprites on the sky sphere)
  - Clouds: a 2D noise texture scrolled across a plane at y=128, semi transparent
- [ ] **Directional lighting**: The sun direction changes with time. Use it as the primary light direction in the chunk fragment shader. At night, dim the light and shift color toward blue/grey.
- [ ] **Block light propagation** (simplified):
  - Each block stores a light level (0~15)
  - Torches emit light level 14
  - Light spreads to neighbors, decreasing by 1 per block (BFS flood fill)
  - When a torch is placed/removed, do a local BFS to update light values
  - Pass light level to the vertex shader, multiply fragment color by `lightLevel / 15.0`
- [ ] **Skylight**: Sunlight propagates downward from the top of each column. Blocks under a roof get lower skylight. Combine skylight and block light (take max).
- [ ] **Ambient occlusion**: Per vertex AO for block faces. Check the 3 adjacent blocks at each corner of a face. If more neighbors are solid, darken that corner. Gives nice soft shadows in corners and edges.
- [ ] **Gamma correction**: Apply sRGB correction in the final fragment output for physically plausible brightness.
- [ ] **Fog**: Distance based fog that blends to the sky color at the render distance boundary. Hides chunk pop in.

**Graphics techniques:** Skybox/skydome rendering, cube map textures, dynamic directional lighting, BFS light propagation, vertex based ambient occlusion, fog, gamma correction.

### Phase 6 — Mobs & Combat (Week 6~7)

**Goal:** Hostile and passive mobs with basic AI.

- [ ] **Entity rendering**: Each mob is a collection of textured rectangular prisms (head, body, limbs). Animate by rotating limb joints over time (walk cycle). Use a model matrix per body part with parent child transformations.
- [ ] **Passive mobs**:
  - Pig: Wanders randomly. Drops raw porkchop on death. 10 HP.
  - Sheep: Wanders randomly. Drops wool (random color) on death. 10 HP.
  - Cow: Wanders randomly. Drops raw beef + leather on death. 10 HP.
  - Behavior: Pick a random direction, walk for 2~5 seconds, pause, repeat. Avoid walking off cliffs. Flee when attacked.
- [ ] **Hostile mobs** (spawn at night or in dark areas):
  - Zombie: Walks toward player if within 16 blocks. Melee attack (3 HP damage). 20 HP. Drops rotten flesh.
  - Skeleton: Walks toward player, stops at 8~15 block range, shoots arrows. 20 HP. Drops bones + arrows.
  - Creeper: Walks toward player silently. When within 3 blocks, starts a 1.5s fuse, then explodes (destroys blocks in radius, heavy damage). 20 HP.
  - Spider: Can climb walls (ignore vertical collision on block surfaces). Hostile at night, neutral during day. 16 HP.
- [ ] **Pathfinding**: Simple A* on the block grid for ground mobs. Only needs to work within ~32 blocks of the player. Recalculate path every 1~2 seconds.
- [ ] **Combat**:
  - Melee: Swing animation, damage = weapon damage value, knockback on hit
  - Player attack cooldown (0.625s for sword)
  - Damage flash: tint the mob red for 0.3s on hit
  - Death animation: entity falls over, fades, drops items
- [ ] **Spawn rules**: Hostile mobs spawn on solid blocks with light level < 7, at least 24 blocks from the player, max entity cap per chunk.
- [ ] **Armor damage reduction**: Each armor piece has a defense value. Total defense reduces incoming damage by a percentage.

**Graphics techniques:** Hierarchical transformations (parent child joint system), skeletal animation (simplified), particle effects for explosions, color tinting in fragment shader.

### Phase 7 — Audio, Polish & Extras (Week 7~8)

**Goal:** Sound, water, particles, and final polish.

- [ ] **Audio**:
  - Background music (ambient tracks, loop with crossfade)
  - Block sounds: break/place sounds per block material (wood, stone, dirt, glass)
  - Mob sounds: zombie groans, skeleton rattles, creeper hiss
  - Player sounds: footsteps (vary by surface), hurt, eating
  - 3D positional audio using OpenAL (mobs and blocks sound directional)
- [ ] **Water rendering**:
  - Water is a semi transparent block with animated UV (scroll the texture)
  - Underwater fog overlay: blue tinted fog when camera is below water surface
  - Water physics: water source blocks spread to adjacent air blocks at lower Y (simplified flood fill, not full Minecraft water physics)
- [ ] **Particles**:
  - Block break particles: small cubes using the block's texture, scattered with random velocity, fade over 0.5s
  - Torch flame particles: small orange/yellow quads rising from torch position
  - Rain: line sprites falling from the sky when weather is active
  - Damage particles: red splash on hit
- [ ] **Crosshair and HUD**:
  - Center crosshair (simple + shape)
  - Health hearts, hunger drumsticks, armor icons
  - Experience bar (if implementing XP)
  - Debug overlay (F3): coordinates, chunk position, FPS, facing direction, light level
- [ ] **Bed mechanic**: Right click bed at night to skip to dawn. Set spawn point.
- [ ] **Death screen**: "You Died!" overlay with respawn button.
- [ ] **Pause menu**: ESC to pause, resume/quit options.
- [ ] **World save/load**: SQLite database stores only modified blocks (delta from generated terrain). On chunk load, generate terrain, then apply deltas.

---

## 5. Rendering Pipeline Summary

```
Per Frame:
1. Update delta time
2. Process input (camera movement, block interaction, UI)
3. Update player physics (gravity, collision)
4. Update entities (AI, movement, collision)
5. Update chunk loading/unloading around player
6. Rebuild dirty chunk meshes (limit N per frame to avoid stutter)
7. Compute VP matrix, extract frustum planes
8. --- Begin Rendering ---
9. Clear framebuffer
10. Render sky (skybox/dome, sun, moon, stars, clouds)
11. Render opaque chunk meshes (front to back for early Z rejection)
12. Render transparent chunk meshes (water, glass — back to front, alpha blending)
13. Render entities (mobs, item drops)
14. Render particles
15. Render block highlight wireframe
16. Render HUD (crosshair, hotbar, hearts, hunger) — orthographic projection
17. Render inventory/crafting UI if open — orthographic projection
18. Swap buffers
```

---

## 6. Shader List

| Shader | Purpose |
|---|---|
| `chunk.vert / chunk.frag` | Main block rendering. Inputs: position, UV, normal, AO, light level. Applies texture atlas lookup, directional sun light, AO darkening, fog. |
| `sky.vert / sky.frag` | Skybox or gradient sky dome. Time of day uniform controls color interpolation. |
| `entity.vert / entity.frag` | Mob/item rendering. Similar to chunk but with model matrix per body part. Supports damage tint uniform. |
| `ui.vert / ui.frag` | 2D orthographic UI elements. Texture + color uniform. Alpha blending. |
| `text.vert / text.frag` | FreeType glyph rendering. Greyscale alpha texture, color uniform. |
| `particle.vert / particle.frag` | Point sprite or small quad particles. Position, color, alpha, size uniforms. |
| `wireframe.vert / wireframe.frag` | Block selection highlight. Line rendering with slight depth offset to prevent z fighting. |
| `water.vert / water.frag` | Animated water surface. Scrolling UV, semi transparent, optional simple wave vertex displacement. |
| `postprocess.vert / postprocess.frag` | (Optional) Full screen quad for underwater tint, gamma correction, vignette. |

---

## 7. Key Algorithms

### 7.1 Greedy Meshing (Optimization)
Instead of emitting 2 triangles per visible block face, merge adjacent coplanar faces of the same block type into larger quads. Dramatically reduces vertex count. Not required for Phase 1 but a strong optimization for later.

### 7.2 DDA Raycasting
Step through the 3D voxel grid along the camera's look direction. At each step, check if the block is solid. Uses Amanatides & Woo's fast voxel traversal. Returns hit position + face normal.

### 7.3 Light Propagation (BFS)
When a light source is placed, do a BFS from that position. Each neighbor gets `max(currentLight, sourceLight - 1)`. Queue neighbors whose light increased. When a light source is removed, do a reverse BFS: darken affected blocks, then re propagate from remaining nearby light sources.

### 7.4 A* Pathfinding
Grid based A* for mob navigation. Nodes are (x, y, z) block positions. Neighbors: 4 cardinal directions (check walkable: solid block below, air at feet and head level). Heuristic: Manhattan distance. Limit search to 200 nodes to avoid lag.

### 7.5 Perlin/Simplex Noise Layering
Terrain height = weighted sum of multiple octaves of 2D Simplex noise at different frequencies and amplitudes. Typical setup: 4~6 octaves, lacunarity 2.0, persistence 0.5. Separate noise fields for continentalness, erosion, and peaks give more natural terrain than a single noise function.

---

## 8. Block Types to Implement

| Block | Transparent | Light | Hardness | Tool | Drop |
|---|---|---|---|---|---|
| Air | yes | 0 | — | — | — |
| Stone | no | 0 | 1.5 | pickaxe | cobblestone |
| Cobblestone | no | 0 | 2.0 | pickaxe | cobblestone |
| Dirt | no | 0 | 0.5 | shovel | dirt |
| Grass Block | no | 0 | 0.6 | shovel | dirt |
| Sand | no | 0 | 0.5 | shovel | sand |
| Gravel | no | 0 | 0.6 | shovel | gravel (10% flint) |
| Oak Log | no | 0 | 2.0 | axe | oak log |
| Spruce Log | no | 0 | 2.0 | axe | spruce log |
| Oak Planks | no | 0 | 2.0 | axe | oak planks |
| Oak Leaves | yes (partial) | 0 | 0.2 | shears | (saplings chance) |
| Glass | yes | 0 | 0.3 | — | nothing |
| Water | yes | 0 | — | — | — |
| Lava | yes (emissive) | 15 | — | — | — |
| Coal Ore | no | 0 | 3.0 | pickaxe | coal |
| Iron Ore | no | 0 | 3.0 | pickaxe (stone+) | raw iron |
| Gold Ore | no | 0 | 3.0 | pickaxe (iron+) | raw gold |
| Diamond Ore | no | 0 | 3.0 | pickaxe (iron+) | diamond |
| Torch | yes | 14 | 0.0 | any | torch |
| Crafting Table | no | 0 | 2.5 | axe | crafting table |
| Furnace | no | 0 | 3.5 | pickaxe | furnace |
| Chest | no | 0 | 2.5 | axe | chest |
| Bed | no | 0 | 0.2 | any | bed |
| Wool | no | 0 | 0.8 | shears | wool |
| Tall Grass | yes | 0 | 0.0 | any | seeds (chance) |
| Flowers | yes | 0 | 0.0 | any | flower |
| Sugar Cane | yes | 0 | 0.0 | any | sugar cane |
| Obsidian | no | 0 | 50.0 | pickaxe (diamond) | obsidian |
| Bedrock | no | 0 | ∞ | — | — |

---

## 9. Performance Targets

| Metric | Target |
|---|---|
| FPS | 60+ at 8 chunk render distance |
| Chunk mesh build time | < 5ms per chunk |
| Memory per chunk | < 256KB (block data) + mesh VBO |
| Draw calls per frame | < 200 (one per visible chunk + entities + UI) |
| Total vertex count | < 2M visible vertices |

**Optimization techniques to apply:**
- Face culling: only emit faces between solid and non solid blocks
- Frustum culling: skip chunks outside the view frustum
- Greedy meshing: merge coplanar same type faces (Phase 2+)
- Chunk render sorting: render front to back for early Z discard
- Deferred mesh rebuilds: limit to 2~4 chunk rebuilds per frame
- Instancing for particles and item entities
- Texture atlas to avoid texture switches

---

## 10. Controls

| Key | Action |
|---|---|
| W/A/S/D | Move forward/left/backward/right |
| Mouse | Look around |
| Left Click | Break block (hold) / Attack |
| Right Click | Place block / Interact (crafting table, furnace, bed) |
| Scroll Wheel | Select hotbar slot |
| 1~9 | Select hotbar slot directly |
| Space | Jump / Swim up |
| Left Ctrl | Sprint |
| E | Open/close inventory |
| Q | Drop held item |
| F3 | Toggle debug overlay |
| F5 | Toggle third person (stretch goal) |
| ESC | Pause menu |
| F11 | Toggle fullscreen |

---

## 11. Timeline (8 Week Plan)

| Week | Phase | Deliverable |
|---|---|---|
| 1 | Phase 1 | Window, camera, single chunk rendering with textures |
| 2 | Phase 1~2 | Lighting on blocks, multi chunk world with terrain noise |
| 3 | Phase 2 | Biomes, trees, caves, ore placement, chunk load/unload |
| 4 | Phase 3 | Block break/place, player physics, health/hunger |
| 5 | Phase 4 | Inventory UI, crafting system, furnace, tools |
| 6 | Phase 5 | Day/night cycle, sky, light propagation, AO, fog |
| 7 | Phase 6 | Mobs (passive + hostile), combat, pathfinding |
| 8 | Phase 7 | Audio, water, particles, polish, world saving |

---

## 12. Reference Projects

- **fogleman/Craft** — Minimal C + OpenGL Minecraft clone (~3000 lines). Great reference for chunk rendering and sqlite world save.
- **Hopson97/MineCraft-One-Week-Challenge** — C++ + SFML, shows what's achievable quickly.
- **Isti01/glCraft** — C++ + OpenGL, has ambient occlusion, animated textures, good vertex packing.
- **LearnOpenGL.com** — Comprehensive OpenGL tutorials (camera, lighting, textures, skybox, text rendering, framebuffers).
- **0fps.net AO article** — "Ambient Occlusion for Minecraft like Worlds", the standard reference for voxel AO.

---

## 13. Minimum Viable Demo (If Time is Short)

If the full plan is too ambitious, here's what to prioritize for a strong graphics project demo:

1. Procedurally generated terrain with multiple biomes and trees
2. FPS camera with smooth movement
3. Block break and place with crosshair
4. Texture atlas based rendering with per face UVs
5. Day/night cycle with dynamic sky colors and directional lighting
6. Per vertex ambient occlusion
7. Distance fog
8. Frustum culling
9. Basic inventory hotbar (no crafting UI needed)
10. At least one mob (zombie that walks toward you)

This subset covers transformations, lighting, texturing, procedural generation, collision detection, and real time rendering optimization, which are the core graphics concepts that matter for evaluation.

# HexaCraft - Completed Items

## Phase 1: Core Foundation
- [x] Hexagonal prism geometry (8 faces)
- [x] GLFW + GLAD + shader loading
- [x] Custom myRotate() (Rodrigues' formula)
- [x] Static camera, WASD+ER flying controls
- [x] Console prints all controls

## Phase 2: Terrain & Scene
- [x] Procedural terrain (FBM noise, 4 octaves)
- [x] Block types: grass, dirt, sand, stone, diamond ore, gold ore
- [x] Trees (trunk + randomized leaf canopy)

## Phase 3: Camera System
- [x] Pitch (X), Yaw (Y), Roll (Z)
- [x] Bird's Eye View (B)
- [x] Rotate around look-at (F)
- [x] 4-viewport split (V): free cam, isometric, top-down, front

## Phase 4: Lighting
- [x] Directional light (1), Point lights (2), Spot light (3)
- [x] Emissive objects (ore glow)
- [x] Ambient (5), Diffuse (6), Specular (7) toggles
- [x] Master light (L)
- [x] Day-night cycle (T): Night/Dawn/Noon/Dusk

## Phase 5: Interactive Objects
- [x] Player character (hierarchical: head, body, arms, legs with joints)
- [x] Rotating fan (G toggle, 4 blades)
- [x] Door open/close (O toggle, smooth animation, pivot hinge)
- [x] Clock (analog face, hour markers, rotating hour+minute hands)

## Phase 6: Curvy Objects & Bezier
- [x] Sphere object (UV-tessellated, crystal ball on stone pedestal near ruins)
- [x] Cone object (tent in sand zone)
- [x] Bezier curve object (decorative arch near ruins, cubic Bezier rendered as tube)
- [x] Spline curve (winding fence in plains, Catmull-Rom spline as tube)
- [x] Ruled surface (ramp/slide near interactive zone)

## Phase 7: Block Placement & Breaking (Minecraft Core)
- [x] 3D voxel grid (80x80x16 blockGrid) with 12 block types
- [x] Terrain generation fills grid at startup (biomes, ores, water)
- [x] renderTerrain reads from grid instead of recomputing noise
- [x] Ray casting from camera to find targeted block (30 unit range)
- [x] Wireframe highlight on targeted block (white outline)
- [x] Left-click breaks blocks (sets to AIR)
- [x] Right-click places blocks on adjacent face
- [x] Hotbar with 9 block types (Scroll to cycle, Numpad 1-9 quick select)
- [x] getGroundY reads from grid for dynamic terrain

## Phase 8: HUD & Game Interface
- [x] Crosshair (+) at screen center (white, always visible)
- [x] Hotbar display (9 slots, bottom center, selected slot highlighted yellow)
- [x] Block color preview in each hotbar slot
- [x] Health bar (red, top left)
- [x] Stamina bar (green, below health)
- [x] All HUD rendered in screen-space (orthographic, depth test disabled)

## Phase 9: Player Movement & Camera Modes
- [x] WASD moves player character on ground (not just camera)
- [x] Player snaps to terrain height with gravity
- [x] Space to jump (parabolic arc)
- [x] Player faces movement direction (yaw rotation)
- [x] Walk animation (arms/legs swing) tied to actual movement, idle = still
- [x] Third-person camera follows behind player (default mode)
- [x] First-person camera at player head
- [x] Free-fly camera (original behavior)
- [x] C key cycles between Third-Person / First-Person / Free-Fly
- [x] E/R for vertical fly in player modes
- [x] Direct mouse look (cursor captured, no button hold needed)

## Phase 10: Course Requirements Completion
- [x] Textures: Simple texture (texture only, no color) on brick wall panel
- [x] Textures: Blended texture (texture × color) on sphere crystal ball
- [x] Textured curvy objects (sphere with grass tex, cone with wood tex)
- [x] Wine glass (surface of revolution) on table with semi-transparency
- [x] Fractal tree leaves (recursive branching, 3 showcase trees)
- [x] Flying birds with random spline motion (Catmull-Rom, 3 birds, wing flap)
- [x] Windows that open/close (P key, smooth animation, 3 castle windows)
- [x] Collision detection (AABB player–terrain, wall sliding, auto-step ledges)
- [x] Gouraud vs Phong shading toggle (H key, uniform in both shaders)
- [x] stb_image.h integration for texture loading (brick, grass, wood)

## Phase 11: Sky, Atmosphere & Fog
- [x] Distance-based fog in fragment shader (exponential, blends to sky color)
- [x] Fog density varies by time of day (thicker at dawn/dusk)
- [x] Sky color changes with day-night cycle (blue noon, orange dawn/dusk, dark night)
- [x] Sun rendering (emissive hex disc, corona glow, position tied to directional light)
- [x] Moon rendering (appears at night, fades with day factor)
- [x] Stars (120 procedural stars, twinkling, slow rotation, fade at dawn)
- [x] Clouds (semi-transparent hex clusters, slow drift, noise-based density)

## Phase 12: Enhanced Terrain & Biomes
- [x] Larger biomes (noise frequency 0.06 → 0.025, ~2.5× bigger)
- [x] More height variation (grass hills up to 12, mountains up to 20)
- [x] Continental noise layer for mountain regions vs flatlands
- [x] Biome edge blending (height smoothing at transitions)
- [x] Beach transitions (auto sand strip where land meets water biome)
- [x] Thick dirt layers (4 blocks under grass, visible on cliff faces)
- [x] Dirt/gravel/stone transition zone below dirt layer
- [x] Sand biome subsurface: 3 sand + 2 sandstone layers
- [x] Snow caps on mountain peaks and tall hills
- [x] New block types: Gravel, Clay, Snow
- [x] Clay deposits near water bodies
- [x] Gravel patches underground, on beaches, on mountain surfaces
- [x] Sparse pine trees on stone biome mountains
- [x] Irregular bedrock layer (1-2 blocks thick)
- [x] Bigger cave chambers (dual noise check)

## Phase 12b: Medieval Castle
- [x] Replaced modern villa with medieval castle (stone walls, battlements)
- [x] 4 corner towers (14 high) with pointed wooden spires
- [x] Front gate with arch, flanking pillars, wooden portcullis frame
- [x] Main keep (10-high walls) with peaked wooden roof + stone gable ends
- [x] Side building (right wing) with own peaked roof
- [x] Left wing (kitchen/armory) with flat roof + battlements
- [x] Wooden balcony on keep front with fence railing
- [x] Interior: great hall table, throne, stairs, torches
- [x] Courtyard: well (stone ring + water + roof), trees, stone path
- [x] Dark stone accent bands on walls, glass windows throughout
- [x] Torches on outer walls and gate entrance
- [x] Stone path from gate extending outward

## Phase 12c: Player & Camera Polish
- [x] Player model resized to Minecraft proportions (1.8 blocks tall)
- [x] Body parts: head 0.38×0.50, torso 0.45×0.65, arms/legs 0.18×0.65
- [x] Steve-style colors (teal shirt, brown hair, blue jeans, skin tone)
- [x] Arm/leg swing from shoulder/hip pivot points (proper animation)
- [x] Zombie mob resized to match player proportions
- [x] Camera eye height: 1.62 blocks (Minecraft standard)
- [x] Third-person camera: closer (3.5 dist), lower pivot at eye level
- [x] Ctrl+Scroll adjusts third-person distance (1.5–10 range)
- [x] FOV: 50° → 70° (Minecraft default)
- [x] Mouse sensitivity: 0.1 → 0.15

## Phase 15: Mobs & Combat
- [x] Sheep mob (white woolly body, dark face/legs, drops wool on death)
- [x] Passive mob flee behavior (run away when attacked, alert nearby mobs)
- [x] Item drops on mob death (pig/chicken: food, sheep: wool, zombie: dirt, skeleton: stone)
- [x] Skeleton hostile mob (gray/white bony humanoid, bow in hand)
- [x] Skeleton AI: approaches to 8-15 range, shoots arrow projectiles, retreats if too close
- [x] Arrow projectile system (gravity arc, player hit detection, ground collision)
- [x] Player attack cooldown (0.625s swords, 1.0s axes, 0.4s other)
- [x] Damage flash (red tint on hit via colorTint shader uniform, 0.3s duration)
- [x] Death animation (mob falls over sideways + fades out over 1 second)
- [x] A* pathfinding on hex grid (200 node limit, 1.5s recalc interval)
- [x] Zombies and skeletons use pathfinding to navigate around walls
- [x] 12 initial passive mobs (chickens, pigs, sheep) in grass zones
- [x] Up to 10 hostile mobs spawn at night, despawn at day

## Phase 14: Inventory & Crafting Completion
- [x] Correct block drops: getBlockDrop() table (stone→cobblestone, grass→dirt, glass→air, etc.)
- [x] Hold-to-break: left mouse hold required; block hardness table; tool speed multipliers (pickaxe/axe/shovel tiers 2×/4×/6×/8×)
- [x] Breaking animation: 5-stage progressive darkening overlay on targeted block while holding
- [x] Tool durability: InventorySlot.durability field; degrade on break/attack; tool breaks at 0; durability bar (green→yellow→red) in HUD hotbar

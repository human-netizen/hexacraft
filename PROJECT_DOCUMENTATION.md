# HexaCraft — Complete Project Documentation

**Course:** Computer Graphics  
**Engine:** OpenGL 3.3 Core Profile  
**Language:** C++17, single compilation unit  
**Window:** GLFW3 + GLAD (OpenGL loader)  
**Math:** GLM (header-only)  
**Build:** `g++ -o hexacraft main.cpp glad.c -Iopengl/include -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm`

Course requirement tags used throughout this document:
- `[CAM]` — Camera & Viewing Controls
- `[LIGHT]` — Lighting Implementation
- `[TEX]` — Textures & Surfaces
- `[SCENE]` — Scene Dynamics & Interaction
- `[MATH]` — Custom Math Function
- `[HIER]` — Complex Hierarchical Movement
- `[WINE]` — Wine Glass Tool Integration
- `[FRAC]` — Fractal Tree Leaves
- `[BIRD]` — Random Bird Motion
- `[SUNLIGHT]` — Light tied to sun position
- `[PHYS]` — Physics / Collision Detection
- `[CLOCK]` — Clock (Taj sir)

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Architecture & File Structure](#2-architecture--file-structure)
3. [Core Geometry — Hexagonal Prism](#3-core-geometry--hexagonal-prism)
4. [Shader Pipeline](#4-shader-pipeline)
5. [Voxel World & Terrain Generation](#5-voxel-world--terrain-generation)
6. [Camera System](#6-camera-system)
7. [Lighting System](#7-lighting-system)
8. [Textures](#8-textures)
9. [Curvy Objects & Mathematical Surfaces](#9-curvy-objects--mathematical-surfaces)
10. [Player Character & Hierarchical Model](#10-player-character--hierarchical-model)
11. [Scene Objects & Interactive Elements](#11-scene-objects--interactive-elements)
12. [Custom Math: myRotate()](#12-custom-math-myrotate)
13. [Block Breaking & Placement System](#13-block-breaking--placement-system)
14. [HUD & UI System](#14-hud--ui-system)
15. [Inventory & Crafting System](#15-inventory--crafting-system)
16. [Player Physics & Movement](#16-player-physics--movement)
17. [Collision Detection](#17-collision-detection)
18. [Sky, Atmosphere & Day-Night Cycle](#18-sky-atmosphere--day-night-cycle)
19. [Mobs, AI & Combat](#19-mobs-ai--combat)
20. [Fractal Trees & Birds](#20-fractal-trees--birds)
21. [Medieval Castle & World Structures](#21-medieval-castle--world-structures)
22. [Controls Reference](#22-controls-reference)

---

## 1. Project Overview

HexaCraft is a Minecraft-inspired voxel game built entirely from scratch using OpenGL 3.3 Core Profile. The key creative distinction is that **all voxels are hexagonal prisms** instead of cubes, giving the world a unique honeycombed visual identity. The game features a procedurally generated world with 6 biome zones, a playable third/first-person character, block breaking and placement, crafting, combat, mobs, and a full day-night cycle.

The project fulfills all items in the university Computer Graphics course requirements (`gfx_req.md`) including lighting models, camera controls, textures, curvy surfaces, hierarchical objects, a wine glass surface of revolution, fractal trees, bird animations, collision detection, and a clock.

---

## 2. Architecture & File Structure

The project uses a **single compilation unit** pattern — `main.cpp` includes every `.h` file directly. There are no separate `.cpp` translation units apart from `glad.c`.

```
main.cpp          — Entry point, GLFW init, game loop, printControls()
glad.c            — OpenGL function loader (GLAD generated)
globals.h         — Constants, enums, all global state, myRotate(), block types
shaders.h         — compileShader(), loadShaders(), uniform setters
geometry.h        — Vertex struct, hex prism mesh, sphere, cone, bezier, spline, ruled surface
world.h           — Terrain generation, biomes, trees, ruins, castle, renderTerrain()
player.h          — drawPlayer() hierarchical model with walk animation
objects.h         — Fan, door, clock, car, wine glass, block targeting
hud.h             — Crosshair, hotbar, health/stamina/hunger bars
input.h           — processInput(), collision physics, mouse/keyboard callbacks
vertexShader.glsl — MVP transform, Gouraud shading path, normal transform
fragmentShader.glsl — Phong shading, all lights, textures, fog, tone mapping
```

### Global State (`globals.h`)

All game state lives in `globals.h` as plain global variables. This is acceptable for a course project and avoids the overhead of a full ECS or OOP design:

- **`blockGrid[200][200][32]`** — the entire voxel world (200×200 columns, 32 height layers)
- **`columnMaxH[200][200]`** — cache of the highest non-air block per column, used for render culling
- **`blockState`** — `std::unordered_map<uint32_t, uint16_t>` storing extra per-block state (door open/closed, facing direction) for interactive blocks only; avoids wasting memory on non-interactive blocks
- Camera vectors: `camPos`, `camFront`, `camUp`, `camYaw`, `camPitch`, `camRoll`
- Player state: `playerWorldPos`, `playerYaw`, `playerVelY`, `playerHealth`, `playerHunger`, `playerStamina`
- Lighting toggles: `lightOn`, `dirLightOn`, `pointLightOn`, `spotLightOn`, `ambientOn`, `diffuseOn`, `specularOn`

---

## 3. Core Geometry — Hexagonal Prism

**File:** `geometry.h`

Every visible object in the world is rendered as one or more transformed hexagonal prisms. This is the foundational mesh.

### Vertex Structure

```cpp
struct Vertex {
    glm::vec3 pos;      // position
    glm::vec3 normal;   // surface normal for lighting
    glm::vec3 color;    // per-vertex color (face shading baked in)
    glm::vec2 texCoord; // UV coordinates for texturing
};
```

Each vertex carries 11 floats (44 bytes). The VAO is set up with 4 attribute pointers matching `layout(location = 0..3)` in the vertex shader.

### Hex Prism Geometry

`generateHexPrism()` builds a unit hexagonal prism centered at the origin:

- **6 rectangular side faces** — each is 2 triangles. The outward normal is computed per-face as the cross product of the edge direction and the up vector: `normalize(cross(edge, {0,1,0}))`. UV coordinates run 0→1 horizontally across each face and 0→1 vertically.
- **Top cap** — 6 triangles fanning from the center `{0, halfH, 0}`. UV maps the hexagon into the unit circle space.
- **Bottom cap** — 6 triangles fanning from `{0, -halfH, 0}` with reversed winding for correct back-face culling.

The hex corners are placed at angles `60° * i + 30°` so the hex is **flat-top** oriented, matching the offset-row grid layout.

### Drawing

`drawHexModel(modelMatrix, color)` sets the `model` uniform and `objectColor` uniform, then calls `glDrawArrays(GL_TRIANGLES, 0, hexVertexCount)`. All terrain, player parts, trees, mobs, and scene objects go through this single draw call.

`drawHexRotated(pos, color, yaw, axis, scale)` is a convenience wrapper that builds the model matrix from translation + rotation + scale using `myRotate()` before calling `drawHexModel`. Used for oriented objects like arrows and player limbs.

### Hex Grid Coordinate System

The world uses an **offset-row hex grid**:

```cpp
float xSpacing = HEX_RADIUS * 2.0f * 0.866f;  // sqrt(3) * r
float zSpacing = HEX_RADIUS * 1.5f;
float x = col * xSpacing + (row % 2) * (xSpacing * 0.5f);  // odd rows offset by half
float z = row * zSpacing;
```

`hexGridPos(col, row, y)` converts grid coordinates to world-space positions. The offset ensures hexagons tile without gaps. Grid indices are offset by `GRID_OFF_X = GRID_OFF_Z = 100` so negative coordinates (col=-100 to 99) map to valid array indices.

---

## 4. Shader Pipeline

**Files:** `vertexShader.glsl`, `fragmentShader.glsl`, `shaders.h`

### Compilation

`compileShader(type, source)` compiles a shader from a source string and checks for errors via `glGetShaderiv(GL_COMPILE_STATUS)`. `loadShaders(vertFile, fragFile)` reads both files, compiles them, links the program, and checks `glGetProgramiv(GL_LINK_STATUS)`.

### Vertex Shader

Inputs: `aPos`, `aNormal`, `aColor`, `aTexCoord` at locations 0–3.

The vertex shader does three things:

1. **MVP transform**: Computes `FragPos = model * aPos` in world space. Passes `Normal = transpose(inverse(model)) * aNormal` (the normal matrix, corrects normals under non-uniform scaling). Outputs `gl_Position = projection * view * FragPos`.

2. **Gouraud shading path** `[LIGHT]` `[SCENE]`: When `useGouraud = true`, the full Phong lighting equation is computed per-vertex here and written to the `gouraudColor` output. This includes ambient, directional, point lights (with attenuation), and spot light. The fragment shader then just uses this pre-computed color.

3. **Pass-through**: Passes `vertexColor`, `TexCoord`, and `gouraudColor` to the fragment shader.

### Fragment Shader `[LIGHT]` `[TEX]`

The fragment shader is the main rendering engine. It processes multiple rendering paths in order:

**Base color resolution `[TEX]`:**
```glsl
if (textureMode == 1)  objColor = texture(texture1, TexCoord).rgb;           // simple texture
if (textureMode == 2)  objColor = texture(texture1, TexCoord).rgb * objectColor; // blended
```

**Special paths (early returns):**
- `isHUD = true` — no lighting, no fog, just flat color or texture. Returns immediately.
- `isEmissive = true` — pulsing glow: `0.8 + 0.2 * sin(time * 3.0)`. Only fog is applied. Returns immediately.
- `!lightOn` — master off: show 5% of object color. Returns immediately.
- `useGouraud = true` — use `gouraudColor` from vertex shader. Apply texture, tone map, fog. Returns.

**Phong shading (default) `[LIGHT]`:**

Ambient term (scaled by `dayFactor` for night/day variation):
```glsl
vec3 ambColor = mix(vec3(0.05,0.05,0.1), vec3(0.25,0.27,0.3), dayFactor);
result += ambColor * objColor;
```

Directional light (sun/moon):
```glsl
vec3 ldir = normalize(-dirLightDir);
float diff = max(dot(norm, ldir), 0.0);
// Blinn-Phong specular:
vec3 reflDir = reflect(-ldir, norm);
float spec = pow(max(dot(viewDir, reflDir), 0.0), 32.0);
```

Point lights (up to 8, with quadratic attenuation + flicker):
```glsl
float atten = 1.0 / (1.0 + 0.09*dist + 0.032*dist*dist);
float flicker = 0.85 + 0.15 * sin(time * 8.0 + float(i) * 2.7);
atten *= flicker;
```

Spot light (single cutoff angle) `[LIGHT]`:
```glsl
float theta = dot(ldir, normalize(-spotLightDir));
if (theta > spotCutoff) { ... }
float intensity = clamp((theta - spotCutoff) / (1.0 - spotCutoff), 0.0, 1.0);
```

**Post-processing:**
- **Reinhard tone mapping**: `result = result / (result + vec3(1.0))` — prevents color blowout from multiple lights.
- **Exponential distance fog**: `fogFactor = exp(-fogDensity * dist * dist)` — blends fragment toward `fogColor`. `[SCENE]`
- **Damage tint**: `result = mix(result, colorTint, colorTintStrength)` — red flash on mob hits.

---

## 5. Voxel World & Terrain Generation

**File:** `world.h`

### Grid Layout

```cpp
int blockGrid[200][200][32];   // [col+100][row+100][height]
int columnMaxH[200][200];      // highest solid block per column
```

The grid is 200×200 columns (covering roughly −100 to +99 in both axes) and 32 layers tall. Each cell stores a `BlockType` enum value. `columnMaxH` is precomputed and updated whenever a block is broken or placed, so the renderer can skip empty columns instantly.

### Biome System

`initBlockGrid()` runs at startup and fills the entire grid. A 2D biome map is generated using FBM (fractional Brownian motion) noise at multiple octaves and frequency 0.025. Each column gets a biome based on noise value:

| Biome | Noise Range | Surface Block | Height Range |
|---|---|---|---|
| Grass Plains | 0.0–0.35 | GRASS over DIRT | 5–12 |
| Forest | 0.35–0.50 | GRASS over DIRT | 6–14 |
| Sand Desert | 0.50–0.65 | SAND over SANDSTONE | 3–6 |
| Stone Mountain | 0.65–0.80 | STONE over STONE | 8–20 |
| Water | < 0.0 | WATER | fills to level 4 |
| Snow Mountain | high altitude | SNOW over STONE | 16–25 |

### Height Generation

Height is generated using **multi-octave FBM noise**:
```
height = base + A1*noise(f1*x, f1*z)
              + A2*noise(f2*x, f2*z)
              + A3*noise(f3*x, f3*z)
              + A4*noise(f4*x, f4*z)
```
A **continental noise** layer is layered on top for mountain regions — where both the biome noise and a separate continental noise are high, height is multiplied for dramatic peaks.

Biome edge **blending** interpolates heights across a transition zone (~3 columns wide) to eliminate sharp seams.

### Underground Layers

For each column, below the surface block the generator fills:
- 4 layers of DIRT (under grass)
- STONE from there down to bedrock
- GRAVEL patches (noise-seeded)
- CLAY deposits near water columns
- ORE BLOCKS at specific depth ranges (diamond deep, gold mid, iron everywhere, coal near surface)
- **Bedrock** at layers 0–1 (random 1–2 thick)
- **Caves**: dual-noise check carves air pockets; both a "cave" noise and a "detail" noise must exceed threshold at a point for it to be hollow. This creates connected cave chambers.

### Snow Caps

```cpp
if (biome == STONE_MOUNTAIN && height > 16)
    topBlock = BLOCK_SNOW;
```

### Rendering `renderTerrain()`

Rather than regenerating geometry each frame, the renderer iterates over all columns within a frustum-culled distance band. For each column, it renders only layers 0 to `columnMaxH[col][row]`. Transparent blocks (water, glass, ice) are collected and drawn in a second pass with alpha blending enabled.

Each block is drawn with `drawHexModel(model, getBlockColor(type))`. The model matrix positions the hex at `hexGridPos(col, row, h * HEX_HEIGHT)`.

---

## 6. Camera System `[CAM]`

**File:** `input.h`

### Camera Modes

Three modes are cycled with the `C` key:

| Mode | Description |
|---|---|
| 0 — Third-Person | Camera floats behind/above player's head |
| 1 — First-Person | Camera at player's eye (1.62 units above feet) |
| 2 — Free-Fly | Detached camera, full 3D movement |

### Mouse Look

The mouse is captured with `glfwSetInputMode(GLFW_CURSOR_DISABLED)`. `mouseCallback` computes delta movement from the last frame and updates `camYaw` and `camPitch`:

```cpp
camYaw   += xoffset * mouseSensitivity;   // sensitivity = 0.15
camPitch += yoffset * mouseSensitivity;
camPitch  = clamp(camPitch, -89.0f, 89.0f);
```

`updateCameraVectors()` recomputes `camFront` from yaw+pitch using standard spherical coordinates:
```cpp
front.x = cos(radians(camYaw)) * cos(radians(camPitch));
front.y = sin(radians(camPitch));
front.z = sin(radians(camYaw)) * cos(radians(camPitch));
camFront = normalize(front);
```

Roll is applied separately in the view matrix using `myRotate` on the up vector.

### Camera Rotations `[CAM]`

- **Pitch (X key)**: `camPitch += rotSpeed` (Shift+X for opposite direction)
- **Yaw (Y key)**: `camYaw += rotSpeed` (Shift+Y for opposite)
- **Roll (Z key)**: `camRoll += rotSpeed` — modifies the `camUp` vector used in `glm::lookAt`

### Bird's Eye View `[CAM]`

**B key** toggles `birdsEye`. When active, the camera is repositioned directly above the player:
```cpp
camPos = playerWorldPos + glm::vec3(0, 25.0f, 0);
camFront = glm::vec3(0, -1, 0);  // looking straight down
camUp = glm::vec3(0, 0, -1);     // north is forward
```

### Orbit Around Look-At `[CAM]`

**F key** toggles orbit mode. Each frame `orbitAngle` increments. The camera orbits `lookAtTarget` (set to the castle entrance area):
```cpp
camPos.x = lookAtTarget.x + orbitRadius * cos(radians(orbitAngle));
camPos.z = lookAtTarget.z + orbitRadius * sin(radians(orbitAngle));
camFront = normalize(lookAtTarget - camPos);
camYaw   = degrees(atan2(camFront.z, camFront.x));
camPitch = degrees(asin(camFront.y));
```

### 4-Viewport Split `[CAM]`

**V key** toggles `fourViewport`. When active, the frame is rendered 4 times using `glViewport` to divide the 1280×720 window into quadrants. Each viewport uses a different camera configuration:

| Viewport | Position | Projection |
|---|---|---|
| Top-left (free cam) | Player-following perspective | Perspective 70° FOV |
| Top-right (isometric) | 45° elevated, angled | Orthographic |
| Bottom-left (top-down) | Directly above, looking down | Orthographic |
| Bottom-right (front) | Fixed front view | Perspective |

### Third-Person Camera

When in third-person mode (`cameraMode == 0`), the camera position is computed each frame as an offset behind and above the player eye:
```cpp
float camYawRad   = radians(camYaw);
float camPitchRad = radians(camPitch);
offset.x = -cos(camYawRad) * cos(camPitchRad) * thirdPersonDist;
offset.y = -sin(camPitchRad) * thirdPersonDist + thirdPersonHeight;
offset.z = -sin(camYawRad) * cos(camPitchRad) * thirdPersonDist;
camPos = playerWorldPos + {0, eyeHeight, 0} + offset;
```
`Ctrl + Scroll` adjusts `thirdPersonDist` between 1.5 and 10 units.

---

## 7. Lighting System `[LIGHT]`

**File:** `fragmentShader.glsl`, `vertexShader.glsl`, `shaders.h`

All lighting is implemented in GLSL with individually toggleable components.

### Light Types

**Directional Light (Key 1)** — Simulates the sun/moon. Direction vector (`dirLightDir`) is updated each frame based on the sun's position in the sky. Color shifts from warm orange at dawn/dusk to white at noon to deep blue at night. `[SUNLIGHT]`

**Point Lights (Key 2)** — Up to 8 simultaneous point lights supported via arrays in the shader:
```glsl
uniform vec3 pointLightPos[8];
uniform vec3 pointLightColor[8];
uniform int numPointLights;
```
Attenuation follows the quadratic formula: `1.0 / (1.0 + 0.09*d + 0.032*d²)`. Torches and glowstone register as point lights. Each torch gets a **flicker** effect: `0.85 + 0.15 * sin(time * 8.0 + i * 2.7)`.

**Spot Light (Key 3)** — Single cone light with cutoff angle. Position follows the player (or a fixed scene location). The soft edge uses intensity ramping: `clamp((theta - cutoff) / (1.0 - cutoff), 0, 1)`.

**Emissive Light** — Objects like diamond ore, glowstone, lanterns, and torches set `isEmissive = true`. They bypass all lighting calculation and glow with a pulsing brightness: `0.8 + 0.2 * sin(time * 3.0)`.

### Component Toggles

- **Ambient (Key 5)**: Scaled by `dayFactor` — dim blue at night, soft grey at day. No direction dependency.
- **Diffuse (Key 6)**: Lambert diffuse `max(dot(N, L), 0)` for all light types.
- **Specular (Key 7)**: Blinn-Phong reflection `pow(max(dot(V, R), 0), 32)` with shininess 32.
- **Master (Key L)**: Kills all lighting; objects render at 5% brightness.

### Gouraud vs Phong Toggle (Key H) `[LIGHT]` `[SCENE]`

The `useGouraud` uniform switches the shading model:

- **Phong (default)**: Lighting computed **per fragment** in `fragmentShader.glsl`. Smooth, high-quality shading, especially visible on curved objects.
- **Gouraud**: Lighting computed **per vertex** in `vertexShader.glsl`. The result `gouraudColor` is interpolated across the triangle. On the flat hexagonal faces this looks acceptable but on curves the low polygon count causes visible artifacts — making the difference visually obvious.

Both modes support all light types, toggles, and textures.

---

## 8. Textures `[TEX]`

**Files:** `globals.h`, `world.h`, `objects.h`

Textures are loaded at startup using **stb_image.h** (a single-header public domain library for PNG/JPG loading). Textures from the FM Default Minecraft resource pack are used.

### Texture Mode Uniform

The fragment shader reads `textureMode`:

| Value | Behavior | Requirement Fulfilled |
|---|---|---|
| `0` | No texture — use `objectColor` only | Default for all geometry |
| `1` | **Simple texture** — `objColor = texture(texture1, TexCoord).rgb` | `[TEX]` simple texture without surface color |
| `2` | **Blended texture** — `objColor = texture(texture1, TexCoord).rgb * objectColor` | `[TEX]` blended texture with surface color |

### Applied Textures

- **Stone brick texture (mode 1)** `[TEX]`: Applied to castle walls, ruins blocks. The texture image is sampled directly; vertex color is completely ignored. UV coordinates are generated procedurally based on block face orientation.
- **Grass/terrain texture (mode 2)** `[TEX]`: Applied to the sphere crystal ball pedestal. The biome surface color tints the texture (a green biome makes a green-tinted terrain texture).
- **Wood texture (mode 2)** `[TEX]`: Applied to the cone (tent) object. The wood grain texture is multiplied by a tan/brown `objectColor`.
- **Door item icon (mode 1)** in HUD: The door's face image is sampled directly for the hotbar item icon.

### UV Coordinates

UV coordinates are embedded in the `Vertex` struct and generated in `generateHexPrism()`. Side faces get `u` sweeping 0→1 across the face width and `v` sweeping 0→1 from bottom to top. Cap faces map into a unit circle (radial UV).

---

## 9. Curvy Objects & Mathematical Surfaces `[TEX]`

**File:** `geometry.h`, `objects.h`

### Sphere `[TEX]`

Generated in `initSphere()` using UV tessellation (latitude/longitude):
```cpp
for each latitude band (phi: 0..PI):
    for each longitude step (theta: 0..2*PI):
        pos = {sin(phi)*cos(theta), cos(phi), sin(phi)*sin(theta)} * radius
        normal = normalize(pos)
        uv = {theta / (2*PI), phi / PI}
```
The sphere sits as a **crystal ball** on a stone pedestal near the castle library. It uses **blended texture mode** (grass texture × teal color). `[TEX]`

### Cone `[TEX]`

`initCone()` generates a cone with configurable radius and height:
```cpp
for each ring step:
    r = baseRadius * (1 - t)    // linearly taper to apex
    for each segment:
        pos = {r*cos(angle), height*t, r*sin(angle)}
        normal = normalize({cos(angle), baseRadius/height, sin(angle)})  // slant normal
```
Used as a **tent** in the sand biome with wood texture in blended mode. `[TEX]`

### Bezier Curve (Decorative Arch) `[TEX]`

`renderBezierArch()` evaluates a cubic Bezier:
```
B(t) = (1-t)³·P0 + 3(1-t)²t·P1 + 3(1-t)t²·P2 + t³·P3
```
The curve is swept into a tube by generating rings of N vertices centered on the curve point, oriented perpendicular to the tangent vector `B'(t)`. Used for a stone **decorative arch** near the ruins.

### Spline Curve (Winding Fence) `[TEX]`

`renderSplineFence()` uses **Catmull-Rom splines** through a series of control points. Between control points P[i-1], P[i], P[i+1], P[i+2]:
```
tangent[i] = 0.5 * (P[i+1] - P[i-1])
```
The interpolation guarantees C1 continuity (tangents match at knots). Like the Bezier tube, it's swept into a cylindrical fence running through the plains. Also used for **bird flight paths**. `[BIRD]`

### Ruled Surface (Ramp/Slide) `[TEX]`

`renderRuledSurface()` interpolates linearly between two arbitrary 3D curves (a straight line at the top and a curved arc at the bottom):
```
P(s, t) = (1 - t) * curve_top(s) + t * curve_bottom(s)
```
Each row of the surface is a straight line connecting corresponding points on the two boundary curves. Normals are computed via cross product of the two partial derivatives. Used as a **curved ramp/slide** west of the castle.

### Wine Glass (Surface of Revolution) `[WINE]`

`renderWineGlass()` generates a surface of revolution by rotating a 2D profile curve around the Y-axis. The profile was generated using the course's wine glass making tool and exported as a series of `(r, y)` control points. The generation sweeps these points through 360° in `N_SEGMENTS` angular steps:

```cpp
for each profile point (r, y):
    for each angle theta in [0, 2*PI]:
        pos = {r * cos(theta), y, r * sin(theta)}
        normal = normalize({cos(theta), 0, sin(theta)})  // radial outward
```

Adjacent rings are connected with quad strips (2 triangles per quad). The glass uses semi-transparent rendering with `alpha = 0.7` and a specular-heavy material to simulate glass.

---

## 10. Player Character & Hierarchical Model `[HIER]`

**File:** `player.h`

### Design

The player is a chibi-proportioned hexagonal humanoid drawn from ~20 individual `drawHexModel()` calls. All parts share a common **base transform** (`baseM`) that positions the model at `playerWorldPos` and rotates to face `playerYaw` (the movement direction).

### Hierarchical Transform Chain `[HIER]`

```cpp
auto baseM = [&]() -> glm::mat4 {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
    m = myRotate(m, PI/2.0f - yaw, glm::vec3(0, 1, 0));  // face movement direction
    return m;
};
```

The rotation uses `PI/2 - yaw` (not `yaw - PI/2`) because the model's face is in the +Z direction. Given `yaw = atan2(moveDir.z, moveDir.x)` (standard atan2 angle from +X), the formula `PI/2 - yaw` correctly rotates the face to match the movement vector `{cos(yaw), 0, sin(yaw)}`.

### Part Hierarchy

Each part is drawn with its own child matrix derived from `baseM()`:

```
baseM (world pos + body yaw)
├── TORSO         — glm::translate(baseM, {0, 0.635, 0}) + scale
├── HEAD          — glm::translate(baseM, {0, 1.075, 0}) + scale
├── HAIR (×5)     — glm::translate(baseM, offset) + scale
├── FACE (eyes, nose, mouth ×5) — glm::translate(baseM, offset)
├── LEFT ARM      — glm::translate(baseM, {-0.44, 0.85, 0})  ← shoulder pivot
│   └── myRotate(+armSwing, X)  ← forward swing
│       ├── Upper arm
│       └── Lower arm (forearm)
├── RIGHT ARM     — glm::translate(baseM, {+0.44, 0.85, 0})  ← shoulder pivot
│   └── myRotate(-armSwing, X)  ← counter-swing
│       ├── Upper arm
│       ├── Lower arm
│       └── HELD ITEM (attached at wrist)
├── LEFT LEG      — glm::translate(baseM, {-0.14, 0.42, 0})  ← hip pivot
│   └── myRotate(+legSwing, X)
│       ├── Upper leg (pants)
│       └── Shoe
└── RIGHT LEG     — glm::translate(baseM, {+0.14, 0.42, 0})  ← hip pivot
    └── myRotate(-legSwing, X)
        ├── Upper leg (pants)
        └── Shoe
```

### Walk Animation

`walkCycle = sin(playerWalkTime * 3.0)` drives `armSwing = walkCycle * 0.4` and `legSwing = walkCycle * 0.35`. Arms and legs swing in opposition (left arm with right leg, right arm with left leg). `playerWalkTime` increments at rate `6.0 * deltaTime` while moving, and stays frozen when idle.

### Held Item in Right Hand `[HIER]`

The held item is attached at the wrist (bottom of right arm), inheriting the arm's swing transform. Different item types render differently:

| Item Type | Rendering |
|---|---|
| Sword | Thin flat blade tilted forward at −0.85 rad |
| Axe | Handle + flat head hex |
| Pickaxe | Handle + horizontal head |
| Shovel | Handle + small blade |
| Block | Small hex block in hand |
| Bow | Vertical arc shape |

### Player Body Proportions

- Total height: ~1.8 blocks (Minecraft standard)
- Eye height: 1.62 blocks above feet (Minecraft standard)
- Head: large (chibi ratio) at 0.44×0.45×0.44
- Torso: 0.48×0.43×0.36
- Arms/Legs: 0.22×0.28×0.22

---

## 11. Scene Objects & Interactive Elements `[SCENE]`

**File:** `objects.h`

### Rotating Fan (Key G) `[SCENE]`

Located in the castle great hall. Four blades arranged at 90° intervals around a central hub. Each blade's angle is driven by `fanAngle` which increments each frame when `fanOn = true`. The fan uses a hierarchical parent-child transform: the hub is translated to world position, and each blade is translated outward from the hub then rotated by `fanAngle + 90° * i`. `[HIER]`

### Door (Key O) `[SCENE]`

An oak door with a hinge pivot on one side. When `doorOpen` is toggled, `doorOpenAngle` smoothly interpolates toward 90° (open) or 0° (closed) using:
```cpp
doorOpenAngle += (target - doorOpenAngle) * 8.0f * deltaTime;
```
The door model matrix is: translate to hinge, rotate by `doorOpenAngle`, translate back. Any block with `SHAPE_DOOR` and `isInteractive = true` uses the same toggle system via `blockState`.

### Clock (Key — always visible) `[CLOCK]`

The clock consists of:
- Circular face (hex disc)
- 12 hour markers at 30° intervals
- **Hour hand** — rotates at `time * (2*PI / 3600)` (one full revolution per hour)
- **Minute hand** — rotates at `time * (2*PI / 60)` (one per minute, faster)

Both hands are thin elongated hex prisms pivoting at the clock center using a hierarchical matrix:
```cpp
glm::mat4 m = clockBase;
m = glm::translate(m, clockCenter);
m = myRotate(m, -hourAngle, {0, 0, 1});  // pivot at center
m = glm::translate(m, {0, handLength/2, 0});  // offset to draw from center outward
```
`[HIER]` `[CLOCK]`

### Car (Arrow Keys) `[SCENE]`

A drivable vehicle with 4 wheels. Arrow Up/Down accelerates/brakes the car's velocity. Arrow Left/Right steers (modifies heading). The car model is hierarchical: body → 4 wheel axle positions → each wheel (hex disc rotated 90°). Wheels rotate forward/backward proportionally to speed. `[HIER]`

### Block Targeting (Raycasting)

`updateBlockTarget()` shoots a ray from `camPos` along `camFront`. The ray marches in small steps (0.2 world units). At each step, `worldToColRow(pos.x, pos.z)` converts to grid coordinates and `getBlock(col, row, h)` checks for a solid block. The march records both the hit block and the previous (air) position for **face determination** (which face the player is looking at, used to place on the correct adjacent block).

The targeted block gets a **wireframe highlight** drawn as 12 hex edges using `GL_LINES` with the depth test offset slightly (`glPolygonOffset`) to prevent z-fighting.

---

## 12. Custom Math: myRotate() `[MATH]`

**File:** `globals.h`

The course requires implementing a custom rotation function instead of using `glm::rotate`. `myRotate` implements **Rodrigues' Rotation Formula**:

Given axis `k` (normalized) and angle θ, the rotation matrix is:

```
R = I·cos(θ) + sin(θ)·[k]× + (1-cos(θ))·(k⊗k)
```

Where `[k]×` is the skew-symmetric cross-product matrix. In column-major form (GLM convention):

```cpp
glm::mat4 myRotate(glm::mat4 m, float angleRad, glm::vec3 axis) {
    axis = normalize(axis);
    float c = cos(angleRad), s = sin(angleRad), t = 1 - c;
    float x = axis.x, y = axis.y, z = axis.z;

    glm::mat4 rot(1.0f);
    rot[0][0] = t*x*x + c;    rot[1][0] = t*x*y - s*z;  rot[2][0] = t*x*z + s*y;
    rot[0][1] = t*x*y + s*z;  rot[1][1] = t*y*y + c;    rot[2][1] = t*y*z - s*x;
    rot[0][2] = t*x*z - s*y;  rot[1][2] = t*y*z + s*x;  rot[2][2] = t*z*z + c;

    return m * rot;
}
```

GLM uses **column-major** storage where `rot[col][row]`, so `rot[0]` is the first column. The function returns `m * rot` (appending the rotation to the right of the existing transform). This is used for **every rotation in the entire project** — player limbs, camera roll, block facing, fan blades, door swing, clock hands, mob orientation, held items.

---

## 13. Block Breaking & Placement System

**File:** `input.h`

### Breaking (Right Click — Hold to Break)

Breaking is a **hold-to-break** system modeled after Minecraft. When right-click is held:

1. `isBreaking = true` is set in `mouseButtonCallback`.
2. Each frame in `processInput()`, `breakHoldTime` accumulates `deltaTime * toolSpeedMultiplier`.
3. When `breakHoldTime >= hardness`, the block is broken:
   ```cpp
   if (breakHoldTime >= getBlockHardness(bt)) breakBlock();
   ```

**Tool speed multipliers** (`getToolSpeedMultiplier`): Pickaxe vs stone = 2×–8× depending on tier. Wrong tool type = no speed bonus.

**Block hardness** (`getBlockHardness`): Leaves = 0.2, dirt = 0.5, wood = 2.0, stone = 1.5, obsidian = 50.0, bedrock = unbreakable.

**Breaking animation**: A 5-stage progressive darkening overlay is drawn on the targeted block. Stage is `floor(breakHoldTime / hardness * 5)`, driving a `colorTint` from (0,0,0) at strength 0.0 → 0.8 over the 5 stages.

**Break particles**: On block break, 8–12 small colored hex fragments are spawned with random outward velocities. These are updated with gravity each frame and rendered as tiny scaled hexes.

### Placement (Left Click)

Left-click calls `placeBlock()`. The function:
1. Checks `placeFaceDir` (determined during raycasting) to find the adjacent empty cell
2. Verifies the slot has the selected block type
3. Calls `setBlock(placeCol, placeRow, placeHeight, bt)`
4. For interactive blocks (doors, stairs), writes initial facing into `blockState` using `yawToFacing(playerYaw)`
5. Decrements the hotbar slot count by 1

### Drops & Pickup

`breakBlock()` calls `spawnItemDrop(col, row, h, getBlockDrop(bt))` which spawns a floating, bobbing, spinning item at the block's world position. `getBlockDrop` handles special cases (stone → cobblestone, grass → dirt, glass → air/nothing).

Item drops are updated each frame with gravity and ground collision. When `glm::length(playerWorldPos - drop.pos) < PICKUP_RADIUS (1.5)`, the drop is added to the player's inventory.

---

## 14. HUD & UI System

**File:** `hud.h`

The HUD is rendered in a separate pass at the end of each frame:
```cpp
glDisable(GL_DEPTH_TEST);
setMat4(shader, "projection", glm::ortho(0, W, 0, H, -1, 1));
setMat4(shader, "view", glm::mat4(1.0f));
setBool(shader, "isHUD", true);
```
This switches to screen-space orthographic projection so all HUD elements are rendered flat on top of the 3D scene.

### Crosshair

Two overlapping thin quads (+) centered at screen center (640, 360). Always white, always visible.

### Hotbar

9 slots drawn at the bottom center of the screen. The selected slot has a yellow border. Each non-empty slot shows:
- A colored hex preview of the block (using `getBlockColor(type)`)
- A stack count number
- A **durability bar** (green→yellow→red based on remaining durability / max durability)

For tool items (swords, axes, etc.), the hotbar renders the item's **flat sprite** (a textured quad using `textureMode 1`) to visually distinguish tools from blocks.

### Health Bar

Red filled rectangle, depleted from right to left. `width = (health / maxHealth) * barWidth`. Shown top-left of screen.

### Stamina Bar

Green filled rectangle below the health bar. Depletes when sprinting, regenerates when walking.

### Hunger Bar

Yellow/orange bar showing remaining hunger. Decreases over time, faster when sprinting.

### Inventory Screen

When **I** is pressed, `inventoryOpen = true` and `glfwSetInputMode(GLFW_CURSOR_NORMAL)` restores the cursor. A large overlay is drawn:
- 3×3 crafting grid (top section, also accessible from crafting table)
- 3×9 storage grid (27 slots)
- 9-slot hotbar row at the bottom

Clicking slots with left/right mouse button transfers items using drag-and-drop logic. Shift+click moves a full stack instantly.

---

## 15. Inventory & Crafting System

**File:** `globals.h`, `input.h`

### Inventory Storage

```cpp
struct InventorySlot { int type; int count; int durability; };
InventorySlot playerInventory[9 + 27];  // 9 hotbar + 27 storage
int hotbarSlot = 0;
```

### Crafting Recipes

Recipe matching uses a fixed `recipes[]` array of `{3×3 input pattern, output type, output count}` structs. On each UI redraw, the 3×3 crafting grid contents are compared against every recipe using exact pattern matching:

```cpp
for each recipe:
    if (craftingGrid[0..8] == recipe.input[0..8]):
        show recipe.output in result slot
```

Recipes include all tiers of tools (wood/stone/iron/diamond), sticks, and bow. Tool materials use the `ITEM_STICK` enum directly in recipe patterns.

### Tool Durability

When a tool breaks a block or attacks, `playerInventory[hotbarSlot].durability--`. At 0, the slot is cleared (tool destroyed). The HUD durability bar shows `durability / getMaxDurability(type)`.

| Tier | Max Durability |
|---|---|
| Wood | 59 |
| Stone | 131 |
| Iron | 250 |
| Diamond | 1561 |

---

## 16. Player Physics & Movement

**File:** `input.h`

### WASD Movement

Movement direction is computed in camera-relative space:
```cpp
glm::vec3 forward = {cos(radians(camYaw)), 0, sin(radians(camYaw))};
glm::vec3 right   = {-sin(radians(camYaw)), 0, cos(radians(camYaw))};
moveDir = W*forward - S*forward + D*right - A*right;
```

After normalizing, the direction is applied at `pSpeed = 4.5 * speedMult * deltaTime` units/frame (1.5× when sprinting). `playerYaw = atan2(moveDir.z, moveDir.x)` updates the body facing direction.

### Gravity

Each frame (when not flying):
```cpp
playerVelY -= 15.0f * deltaTime;  // gravitational acceleration
playerWorldPos.y += playerVelY * deltaTime;
```

When `playerWorldPos.y <= groundY`, the player snaps to the ground: `playerWorldPos.y = groundY`, `playerVelY = 0`, `playerOnGround = true`.

### Jumping

Space bar: `playerVelY = 6.0f; playerOnGround = false;` — gives a parabolic arc.

### Fall Damage

`fallStartY` is recorded when the player leaves the ground. On landing:
```cpp
float fallDist = fallStartY - groundY;
if (fallDist > 3.0f * HEX_HEIGHT):
    damage = (fallDist / HEX_HEIGHT) - 3.0f;  // 1 HP per block above 3
    playerHealth -= damage;
```

### Ladder Climbing

When the player's feet or body layer contains a `BLOCK_LADDER`, `playerOnLadder = true`. Gravity is suppressed. W moves up, S moves down the ladder at ±4 m/s. Releasing both keys holds position.

### Sprinting

`Shift` + movement: `pSpeed *= 1.5`. Drains stamina at 15/sec. At stamina = 0, sprinting stops.

### Hunger & Health Regen

Hunger decrements every 5 seconds (faster when sprinting). When hunger > 14/20, health regenerates at 0.5 HP/sec. When hunger = 0, health drains at 0.5 HP/sec (starvation).

---

## 17. Collision Detection `[PHYS]`

**File:** `input.h`

### isSolid()

Determines whether a block at `(col, row, h)` constitutes a physical barrier:
- Air and water are always non-solid
- `BlockProperties.isSolid` must be true
- Interactive blocks (doors, fence gates) pass if their `blockState` bit 2 = 1 (open)

### Multi-Cell Horizontal Collision

The core improvement over simple center-point collision: `canMoveTo(wx, wz, currentY)` samples **9 points** around the player's footprint:

```
Center, ±X at radius, ±Z at radius,
four diagonal points at 45° intervals
```

All with player radius `PR = 0.28` units. For each unique hex cell those points land in, both `feetH = floor(Y / HEX_HEIGHT)` and `headH = feetH + 1` are checked. If any cell is solid at either layer, movement is blocked. `[PHYS]`

The 0.28 radius is intentionally kept below the hex half-width (~0.43) so single-hex-wide corridors remain navigable.

### Auto-Step (Ledge Climbing)

`tryMoveWithStep(px, py, pz, nx, nz)`:
1. Tries `canMoveTo(nx, nz, py)` — direct horizontal move
2. If blocked, tries `canMoveTo(nx, nz, py + HEX_HEIGHT)` — one block up
3. If step succeeds, snaps `py = getGroundYAtHeight(col, row, steppedY)` — player smoothly climbs the ledge

### Wall Sliding

If the full move `(nx, nz)` fails:
```cpp
if (!tryMoveWithStep(px, py, pz, nx, pz))   // try X-only
    tryMoveWithStep(px, py, pz, px, nz);     // try Z-only
```
This allows the player to slide along walls instead of stopping dead.

---

## 18. Sky, Atmosphere & Day-Night Cycle `[SUNLIGHT]`

### Day-Night Cycle (Key T)

`dayFactor` cycles through four named states (Night=0 → Dawn=0.3 → Noon=1.0 → Dusk=0.3) on key press. Each frame, `dayFactor` smoothly interpolates toward the target using `mix(current, target, 8*deltaTime)`.

### Sky Color

The sky background `glClearColor` is computed by mixing:
```cpp
nightSky  = {0.02, 0.02, 0.08}
dawnSky   = {0.90, 0.55, 0.25}  // orange sunrise
noonSky   = {0.40, 0.65, 0.90}  // blue day
duskSky   = {0.85, 0.45, 0.20}
skyColor  = mix(nightSky, noonSky, dayFactor)
```

### Sun Rendering `[SUNLIGHT]`

The sun is a large emissive hex disc orbiting in an arc. Its position:
```cpp
float sunAngle = (1.0f - dayFactor) * PI;  // 0=horizon, PI/2=noon, PI=other horizon
sunPos = player + {cos(sunAngle)*50, sin(sunAngle)*50, 0}
```

The **directional light direction** is set each frame to point from the sun toward the origin:
```cpp
dirLightDir = normalize(sunPos - sceneCenter);
dirLightColor = mix(nightColor, noonColor, dayFactor);
```
This directly ties the lighting projection to the sun's position. `[SUNLIGHT]`

### Moon

An emissive gray hex disc on the opposite side of the sky from the sun. Fades out at dawn using `alpha = clamp(1 - dayFactor * 3, 0, 1)`.

### Stars

120 procedurally positioned stars rendered as tiny emissive hexes. Positions are seeded from `gridSeed(i*7, i*13)` so they're stable between frames. They rotate slowly (driven by `time * 0.001`) and fade in at night: `alpha = clamp(0.8 - dayFactor * 2, 0, 1)`.

### Clouds

Semi-transparent hex clusters at Y=22–26. 20 cloud formations, each 3–6 hexes wide. Positions drift with `x_offset = time * cloudSpeed`. Alpha = 0.4, rendered after opaque geometry with `GL_BLEND`.

### Distance Fog

In the fragment shader:
```glsl
float fogFactor = exp(-fogDensity * dist * dist);  // Gaussian falloff
result = mix(fogColor, result, fogFactor);
```
`fogDensity` is set from C++ each frame: higher at dawn/dusk (0.003), low at noon (0.0008), heavy at night (0.005). `fogColor` matches the sky color. This hides the hard terrain edge at the grid boundary.

---

## 19. Mobs, AI & Combat

**File:** `input.h`

### Mob Types

| Type | HP | Behavior | Drop |
|---|---|---|---|
| Chicken | 4 | Passive, flee when hit | Food |
| Pig | 10 | Passive, flee | Food |
| Sheep | 8 | Passive, flee, alerts nearby | Wool |
| Zombie | 20 | Hostile, melee attacks | Dirt (rotten flesh proxy) |
| Skeleton | 20 | Hostile, ranged arrows | Stone (bones proxy) |

### Mob Model

Each mob is drawn hierarchically similar to the player: a body hex + head hex + 4 leg hexes. Legs swing with a walk animation driven by the mob's movement speed. Skeletons carry a bow in their right hand.

### AI State Machine

```
Passive mobs:    IDLE → WANDER → FLEE (if attacked)
Hostile mobs:    IDLE → CHASE → ATTACK
```

Passive mobs pick a random walk direction every 2–5 seconds and walk for a short duration. When attacked, they flee directly away from the attacker at 1.5× normal speed and alert nearby mobs of the same type.

### A* Pathfinding (Hostile Mobs) `[PHYS]`

Zombies and skeletons use an A* search on the hex grid to navigate around walls. The search is capped at 200 nodes for performance. Nodes are `(col, row, height)` triples. Heuristic = hex distance. Walkable check: solid block below at `h-1`, air at `h` and `h+1`. Path is recalculated every 1.5 seconds if the player is within 20 blocks. Between recalculations, the mob moves toward the next waypoint on the cached path.

### Combat

**Melee attack** (left click): `attackMob()` ray-casts from `camPos` along `camFront`. Any mob within `bestDist = 4.0` units whose hitbox intersects the ray takes damage. Attack cooldown varies by weapon: 0.625s swords, 1.0s axes, 0.4s other.

**Arrow projectiles**: Skeletons fire arrows at 8–15 block range. Each arrow has position, velocity (with gravity: `vel.y -= 9.8 * dt`), lifetime, and an active flag. Arrow–player hit checks proximity < 1.0 unit.

**Damage flash**: On hit, `colorTint = {1,0,0}` and `colorTintStrength = 0.6`. This decays over 0.3s back to 0 using `colorTintStrength -= deltaTime / 0.3`.

**Death animation**: `m.alive = false` triggers a fall-over: `deathRotation` sweeps from 0 to 90° over 1 second, tipping the mob sideways. After 1 second, the mob is removed from the list and item drops are spawned.

### Spawning Rules

At night (`dayFactor < 0.3`), hostile mobs spawn at random positions 15–25 blocks from the player, on solid ground, away from torches (torch safe zone radius = 5 blocks). Max 10 hostile mobs at a time.

---

## 20. Fractal Trees & Birds `[FRAC]` `[BIRD]`

### Fractal Tree Leaves `[FRAC]`

Three showcase fractal trees near spawn use recursive branching instead of the standard noise-blob canopy. The algorithm:

```cpp
void drawFractalBranch(glm::vec3 pos, glm::vec3 dir, float length, int depth) {
    if (depth == 0 || length < 0.3) {
        drawLeafCluster(pos);  // draw leaf hex at tip
        return;
    }
    drawHexRotated(pos, woodColor, yaw, axis, {0.05, length, 0.05});  // branch
    for (int i = 0; i < 3; i++) {
        float branchAngle = PI/4 + random * PI/6;
        float azimuth = i * 2*PI/3 + random * PI/4;
        glm::vec3 newDir = rotate(dir, branchAngle, azimuth);
        drawFractalBranch(pos + dir*length, newDir, length*0.65, depth-1);
    }
}
```

Each branch splits into 3 children at ~45° spread angles, azimuthally distributed at 120° intervals with slight randomization. Recursion depth = 4. At the leaf tips, bright-colored hex clusters are drawn. The 3 showcase trees use different seed values for natural variety. `[FRAC]`

### Birds with Random Spline Motion `[BIRD]`

Three birds fly continuously on Catmull-Rom spline paths. Each bird has:
- An array of 6 random waypoints spread across the sky at Y=18–25
- A parametric time `t` that advances each frame
- When `t` reaches the last waypoint, a new set of 6 random waypoints is generated

Position is evaluated via Catmull-Rom:
```
P(t) = 0.5 * ((2·P1) + (-P0+P2)·t + (2P0-5P1+4P2-P3)·t² + (-P0+3P1-3P2+P3)·t³)
```

The bird model is minimal: a small body hex, two wing hexes that flap via `sin(time * 8 * birdSpeed) * 0.5` radians. Bird orientation is set to the current spline tangent direction. All 3 birds run at slightly different speeds and phase offsets. `[BIRD]`

---

## 21. Medieval Castle & World Structures

**File:** `world.h`

The castle is procedurally constructed in `initBlockGrid()` by calling a series of helper functions that write blocks directly into the voxel grid. This means it is part of the world and can be broken/modified just like terrain.

### Castle Layout

```
4 corner towers (14 high) + crenellations
Front gate: arch + flanking pillars + portcullis
Main keep: 10-high walls + peaked wooden roof + gable ends
Side wings: right wing, left wing (kitchen) with flat battlement roof
Balcony: wooden floor + fence railing on keep front
Interior: great hall table, throne, stairs, torches, crafting table
Courtyard: well (stone ring + water + roof), trees, stone path
```

Walls use `BLOCK_STONE` and `BLOCK_STONE_LIGHT` for alternating bands. Windows use `BLOCK_GLASS`. Torches on outer walls register in `torchPositions[]` which feeds into the point lights array.

### Other Structures

**Stone Ruins (Zone D)**: Partial pillars of cobblestone/mossy cobblestone with missing sections and a crystal ball on a pedestal. The ruins use `initRuins()` which places blocks in patterns simulating decay.

**Bezier Arch**: Placed near the ruins as a ceremonial gateway. The bezier tube geometry is rendered in `renderObjects()` each frame (not baked into the grid).

**Cone Tent (Zone B)**: Sand biome tent, rendered as a curvy object each frame.

**Well**: Stone ring + water block + small wooden roof beams. Constructed in the castle courtyard procedurally.

---

## 22. Controls Reference

```
MOVEMENT
  W / S             Walk forward / backward
  A / D             Strafe left / right
  Space             Jump
  E                 Fly upward
  Shift + W         Sprint
  Arrow Keys        Drive car

CAMERA
  Mouse             Look around (always active)
  C                 Cycle: Third-Person → First-Person → Free-Fly
  B                 Bird's Eye View
  F                 Orbit around look-at point
  V                 4-Viewport split
  X / Shift+X       Camera pitch up / down
  Y / Shift+Y       Camera yaw right / left
  Z / Shift+Z       Camera roll CW / CCW
  Ctrl + Scroll     Adjust third-person distance

BUILDING
  Right Click (hold) Break block
  Left Click         Place block
  Scroll             Cycle hotbar slot
  1–9 (Numpad)       Quick-select hotbar slot
  I                  Open / close inventory

LIGHTING
  1                 Toggle directional light (sun)
  2                 Toggle point lights (torches)
  3                 Toggle spot light
  5                 Toggle ambient
  6                 Toggle diffuse
  7                 Toggle specular
  L                 Master light on/off
  H                 Toggle Gouraud / Phong shading

WORLD
  T                 Cycle day/night (Night → Dawn → Noon → Dusk)
  G                 Toggle rotating fan
  O                 Toggle door

MISC
  P                 Toggle windows open/close
  R                 Respawn (after death)
  ESC               Quit
```

---

## Course Requirements Checklist

| Requirement | Implementation | Status |
|---|---|---|
| Bird's Eye View `[CAM]` | B key, camera directly above | ✅ |
| Camera Pitch/Yaw/Roll `[CAM]` | X/Y/Z keys + mouse | ✅ |
| Flying simulator W/S/A/D/E/R `[CAM]` | Free-fly mode | ✅ |
| Rotate around look-at `[CAM]` | F key, orbit angle | ✅ |
| 4-viewport split `[CAM]` | V key, 4 different projections | ✅ |
| Point / Directional / Spot / Emissive lights `[LIGHT]` | Full Phong in GLSL | ✅ |
| Light component toggles 1/2/3/5/6/7/L `[LIGHT]` | Uniform booleans | ✅ |
| Simple texture (texture only, no color) `[TEX]` | `textureMode=1` on brick wall | ✅ |
| Blended texture (texture × color) `[TEX]` | `textureMode=2` on sphere/cone | ✅ |
| Textured curvy objects (sphere + cone) `[TEX]` | stb_image + UV mapping | ✅ |
| Bezier curves `[TEX]` | Cubic Bezier tube arch | ✅ |
| Spline curves `[TEX]` | Catmull-Rom fence + bird paths | ✅ |
| Ruled surface `[TEX]` | Linear interpolation between curves | ✅ |
| Rotating fan `[SCENE]` | G key, hierarchical blades | ✅ |
| Doors + windows `[SCENE]` | Right-click toggle, smooth animation | ✅ |
| Dynamic equipment `[SCENE]` | Fan, clock, car, door | ✅ |
| Console controls printout `[SCENE]` | `printControls()` at startup | ✅ |
| `myRotate()` Rodrigues formula `[MATH]` | Used for all rotations | ✅ |
| Hierarchical movement `[HIER]` | Player model, arms, legs, car, clock | ✅ |
| Wine glass from tool `[WINE]` | Surface of revolution, semi-transparent | ✅ |
| Fractal tree leaves `[FRAC]` | Recursive branching, depth 4, 3 trees | ✅ |
| Random bird motion `[BIRD]` | Catmull-Rom spline, 3 birds, wing flap | ✅ |
| Light source motion tied to sun `[SUNLIGHT]` | `dirLightDir` updates from sun pos | ✅ |
| Collision detection `[PHYS]` | Multi-cell AABB, wall slide, auto-step | ✅ |
| Clock `[CLOCK]` | Analog face, rotating hour/minute hands | ✅ |
| Gouraud vs Phong shading `[LIGHT]` | H key, vertex vs fragment lighting | ✅ |

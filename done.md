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

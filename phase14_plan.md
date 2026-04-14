# Phase 14 Implementation Plan — Inventory & Crafting Completion

## Current State

| Feature | Status |
|---|---|
| Item drops spawn on block break | ✅ done — `spawnItemDrop()` exists |
| Item drop physics (gravity, bounce) | ✅ done |
| Item bobbing + spinning render | ✅ done |
| Item pickup by walking over | ✅ done |
| Block break = instant left-click | ✅ done (but wrong — should be hold) |
| Block drops correct item | ❌ drops the block itself (stone → stone, not → cobblestone) |
| Hold-to-break with mining speed | ❌ not started |
| Breaking animation (crack stages) | ❌ not started |
| Tool durability | ❌ not started |
| Durability bar in HUD | ❌ not started |

---

## Sub-Phase 14A — Correct Block Drops ❌

**Goal:** `stone → cobblestone`, `grass → dirt`, `ore → raw drop`, etc.

**Changes:**
- Add `getBlockDrop(int blockType) -> int` in `world.h` — lookup table
  - `BLOCK_STONE` → `BLOCK_COBBLESTONE`
  - `BLOCK_GRASS` → `BLOCK_DIRT`
  - `BLOCK_ORE_DIAMOND` → `BLOCK_ORE_DIAMOND` (keep, no smelting yet)
  - `BLOCK_ORE_GOLD` → `BLOCK_ORE_GOLD`
  - `BLOCK_COAL_ORE` → `BLOCK_COAL_ORE`
  - `BLOCK_GLASS` → `BLOCK_AIR` (no drop without silk touch)
  - `BLOCK_LEAF` → `BLOCK_AIR` (no drop without shears)
  - Everything else → itself
- Update `breakBlock()` in `input.h` to call `spawnItemDrop(..., getBlockDrop(bt))`
- Skip `spawnItemDrop` if drop is `BLOCK_AIR`

---

## Sub-Phase 14B — Hold-to-Break + Mining Speed ❌

**Goal:** Left-click-hold breaks blocks; harder blocks take longer; correct tool speeds it up.

**New globals in `globals.h`:**
```cpp
bool isBreaking = false;
float breakHoldTime = 0.0f;
int breakTargetCol, breakTargetRow, breakTargetH;
```

**New functions:**
- `getBlockHardness(int bt) -> float` — base seconds to break bare-handed:
  - `BLOCK_DIRT / BLOCK_SAND / BLOCK_GRAVEL / BLOCK_SNOW` → 0.5s
  - `BLOCK_GRASS` → 0.6s
  - `BLOCK_WOOD / BLOCK_PLANKS / BLOCK_LEAF` → 2.0s
  - `BLOCK_STONE / BLOCK_COBBLESTONE / BLOCK_SANDSTONE` → 7.5s
  - `BLOCK_ORE_*` → 15.0s
  - `BLOCK_OBSIDIAN` → 250.0s (diamond pick only)
  - Tools & air → 0 (instant / not breakable)
- `getToolSpeedMultiplier(int toolType, int blockType) -> float`:
  - Pickaxe on stone-type: wood=2×, stone=4×, iron=6×, diamond=8×
  - Axe on wood-type: wood=2×, stone=4×, iron=6×, diamond=8×
  - Shovel on dirt/sand/gravel/snow: wood=2×, stone=4×, iron=6×, diamond=8×
  - Wrong tool: 1× (no bonus)
  - `BLOCK_OBSIDIAN`: only diamond pick works, all others = 0 (unbreakable)

**Input changes:**
- Mouse PRESS left → `isBreaking = true`, reset `breakHoldTime`, record target
- Mouse RELEASE left → `isBreaking = false`, reset `breakHoldTime`
- Remove instant `breakBlock()` from mouse press handler
- In `processInput()` each frame: if `isBreaking && hasTarget`:
  - Target changed → reset `breakHoldTime`, update stored target
  - Compute `breakDuration = hardness / toolSpeedMultiplier`
  - `breakHoldTime += deltaTime`
  - If `breakHoldTime >= breakDuration` → `breakBlock()`, reset

---

## Sub-Phase 14C — Breaking Animation ❌

**Goal:** 5 stages of progressive darkening on the targeted block while holding break.

**Approach:**
- Compute `progress = clamp(breakHoldTime / breakDuration, 0, 1)`
- Add `drawBreakOverlay(int col, int row, int h, float progress)` in `hud.h`
- Draw semi-transparent dark hex at target, slightly scaled (1.02×)
- 5 stages: 0–20% / 20–40% / 40–60% / 60–80% / 80–100% → alpha 0.1 / 0.25 / 0.4 / 0.55 / 0.7
- Enable alpha blending, draw, restore state

---

## Sub-Phase 14D — Tool Durability ❌

**Goal:** Tools wear down with use; break when 0; HUD shows bar.

**Struct change:**
```cpp
struct InventorySlot {
    int type;
    int count;
    int durability; // -1 = infinite (blocks), 0 = broken, >0 = remaining
};
```

**`getMaxDurability(int toolType) -> int`:**
- Wood: 59, Stone: 131, Iron: 250, Diamond: 1561, non-tools: -1

**Changes:**
- `breakBlock()`: if held item is tool → `durability--`; if 0 → destroy slot
- `attackMob()`: same on hit
- `computeCraftingOutput()`: initialize `durability = getMaxDurability(type)` for tools
- HUD hotbar: for tool slots, draw thin bar at bottom of icon
  - Green (>50%) → Yellow (20–50%) → Red (<20%)
  - Width = `(durability / maxDurability) * slotWidth`

---

## Execution Order

- [x] 14A — Correct block drops
- [x] 14B — Hold-to-break + mining speed
- [x] 14C — Breaking animation
- [x] 14D — Tool durability

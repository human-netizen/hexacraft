# Inventory System Fix Plan — Minecraft Parity

## Completed (Phase 1)
- [x] Item stack count display on all slots (hotbar, storage, crafting, dragged)
- [x] Storage grid row order fixed (row 0 at top like Minecraft)
- [x] Recipe offset matching (place recipe anywhere in 3x3 grid)
- [x] Fixed duplicate/broken recipes (wood→wood, clay→stone_light, 9diamond→ice)
- [x] Tools are functional — swords deal tier-based damage, axes deal high damage
- [x] `attackMob()` reads held item for damage calculation
- [x] `breakBlock()` requires diamond pickaxe for obsidian
- [x] Escape closes inventory before quitting game
- [x] Extracted `closeInventory()` helper (deduplicated I/Esc logic)
- [x] Scroll callback uses `getBlockName()` instead of hardcoded array
- [x] Shift-click mass craft on output slot (crafts until inputs run out)

## Completed (Phase 2) — Block Texture Icons in Inventory
- [x] Loaded 23 textures from Gold resource pack (grass, sand, oak log, leaves, snow, ice, clay, etc.)
- [x] Added all new block→texture mappings in `getBlockTexture()` (world.h)
- [x] Added `needsTint()` for grayscale textures (grass, leaves, water) that need color blending
- [x] Updated `drawFlatIcon()` with tint parameter — uses textureMode=2 for tinted blocks
- [x] Updated both `drawSlotItem` and `drawMiniItem` (recipe book) renderers

### Still using colored squares (no texture in either pack):
- dirt, white/red/blue wool, carpets

## Completed (Phase 3) — Further Polish
- [x] Slot hover highlighting (semi-transparent white overlay on hovered slot)
- [x] Item name tooltip on hover (purple box with block name, shown when not dragging)
- [x] Shift-click from crafting grid back to inventory
- [x] Middle-click pick block (selects existing hotbar slot or copies to current slot)
- [x] Number keys 1-9 for hotbar selection when inventory is open (lighting toggles still work outside inventory)
- [x] Handle dragged item drop when inventory full on close (prints warning, clears item)
- [x] 2x2 personal crafting grid when opening with I (vs 3x3 from crafting table); recipe book only available with crafting table

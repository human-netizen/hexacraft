# FM Default v1.3.2 [MC1.19.3+] — Texture Reference

**Base path:** `FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/`

Use this file to quickly look up which textures exist and their exact paths for `loadTexture()` calls.

---

## Directory Structure

```
assets/minecraft/
├── blockstates/     JSON blockstate definitions
├── lang/            en_us.json, en_gb.json, en_au.json, en_ca.json, en_nz.json
├── models/
│   └── block/       JSON model files
└── textures/
    ├── block/       Block face textures (most useful)
    ├── entity/      Mob/entity skins
    ├── environment/ Sky, sun, moon, rain, snow
    ├── gui/         UI panels, inventory, icons
    ├── item/        Item sprites
    └── particle/    Particle sprites
```

---

## Block Textures (`textures/block/`)

### Terrain / Natural
| Filename | Description |
|----------|-------------|
| `stone.png` | Plain stone |
| `stone_inf.png` | Infinite-variation stone (FM exclusive) |
| `cobblestone.png` | Cobblestone |
| `cobblestone_inf.png` | Infinite-variation cobblestone |
| `mossy_cobblestone.png` | Mossy cobblestone |
| `bedrock.png` | Bedrock |
| `grass_block_side.png` | Grass block side (dirt+grass strip) |
| `grass_block_side_overlay.png` | Grass overlay for side (tintable) |
| `grass_block_snow.png` | Grass block with snow top |
| `dirt_path_side.png` | Dirt path side |
| `deepslate.png` | Deepslate side |
| `deepslate_top.png` | Deepslate top |
| `deepslate_inf.png` | Infinite-variation deepslate |
| `deepslate_inf_mirrored.png` | Mirrored deepslate variation |
| `deepslate_top_inf.png` | Infinite-variation deepslate top |
| `gravel.png` | *(not present — use color fallback)* |
| `sand.png` | *(not present — use color fallback)* |
| `mycelium_side.png` | Mycelium side |
| `mycelium_snow.png` | Mycelium with snow |
| `podzol_side.png` | Podzol side |
| `podzol_snow.png` | Podzol with snow |
| `netherrack.png` | Netherrack |
| `soul_sand.png` | *(not present)* |
| `end_stone.png` | *(not present — blockstate only)* |

### Stone Bricks
| Filename | Description |
|----------|-------------|
| `stone_bricks.png` | Standard stone bricks |
| `stone_bricks_inf.png` | Infinite-variation stone bricks |
| `cracked_stone_bricks.png` | Cracked stone bricks |
| `cracked_stone_bricks_inf.png` | Infinite-variation cracked bricks |
| `chiseled_stone_bricks.png` | Chiseled stone bricks |
| `chiseled_stone_bricks_inf.png` | Infinite-variation chiseled bricks |
| `mossy_stone_bricks.png` | Mossy stone bricks |
| `mossy_stone_bricks_inf.png` | Infinite-variation mossy bricks |

### Wood
| Filename | Description |
|----------|-------------|
| `oak_planks.png` | Oak planks |
| `birch_planks.png` | Birch planks |
| `spruce_planks.png` | Spruce planks |
| `jungle_planks.png` | Jungle planks |
| `acacia_planks.png` | Acacia planks |
| `dark_oak_planks.png` | Dark oak planks |
| `birch_log.png` | Birch log side |
| `birch_log_top.png` | Birch log top |
| `spruce_log.png` | Spruce log side |
| `spruce_log_top.png` | Spruce log top |
| `petrified_oak_slab.png` | Petrified oak slab |

### Ores
| Filename | Description | Notes |
|----------|-------------|-------|
| `coal_ore.png` | Coal ore (stone base + overlay) | |
| `coal_ore_overlay.png` | Coal ore vein overlay only | |
| `iron_ore.png` | Iron ore | |
| `iron_ore_overlay.png` | Iron ore overlay | |
| `gold_ore.png` | Gold ore | |
| `gold_ore_overlay.png` | Gold ore overlay | |
| `diamond_ore.png` | Diamond ore | |
| `diamond_ore_overlay.png` | Diamond ore overlay | |
| `lapis_ore.png` | Lapis ore | |
| `lapis_ore_overlay.png` | Lapis ore overlay | |
| `emerald_ore.png` | Emerald ore | |
| `emerald_ore_overlay.png` | Emerald ore overlay | |
| `copper_ore.png` | Copper ore | |
| `copper_ore_overlay.png` | Copper ore overlay | |
| `redstone_ore.png` | Redstone ore (lit) | |
| `redstone_ore_off.png` | Redstone ore (unlit) | |
| `redstone_ore_overlay.png` | Redstone ore overlay | |
| `redstone_ore_off_overlay.png` | Unlit redstone overlay | |
| `nether_gold_ore.png` | Nether gold ore | |
| `nether_gold_ore_overlay.png` | Nether gold overlay | |
| `nether_quartz_ore.png` | Nether quartz ore | |
| `nether_quartz_ore_overlay.png` | Nether quartz overlay | |
| `deepslate_coal_ore.png` | Deepslate coal ore | |
| `deepslate_iron_ore.png` | Deepslate iron ore | |
| `deepslate_gold_ore.png` | Deepslate gold ore | |
| `deepslate_diamond_ore.png` | Deepslate diamond ore | |
| `deepslate_lapis_ore.png` | Deepslate lapis ore | |
| `deepslate_emerald_ore.png` | Deepslate emerald ore | |
| `deepslate_copper_ore.png` | Deepslate copper ore | |
| `deepslate_redstone_ore.png` | Deepslate redstone ore (lit) | |
| `deepslate_redstone_ore_off.png` | Deepslate redstone (unlit) | |

### Glass & Transparent
| Filename | Description |
|----------|-------------|
| `glass.png` | Clear glass |
| `glass_pane_top.png` | Glass pane top face |
| `black_stained_glass.png` | Black glass |
| `blue_stained_glass.png` | Blue glass |
| `brown_stained_glass.png` | Brown glass |
| `cyan_stained_glass.png` | Cyan glass |
| `gray_stained_glass.png` | Gray glass |
| `green_stained_glass.png` | Green glass |
| `light_blue_stained_glass.png` | Light blue glass |
| `light_gray_stained_glass.png` | Light gray glass |
| `lime_stained_glass.png` | Lime glass |
| `magenta_stained_glass.png` | Magenta glass |
| `orange_stained_glass.png` | Orange glass |
| `pink_stained_glass.png` | Pink glass |
| `purple_stained_glass.png` | Purple glass |
| `red_stained_glass.png` | Red glass |
| `white_stained_glass.png` | White glass |
| `yellow_stained_glass.png` | Yellow glass |

### Interactive / Functional Blocks
| Filename | Description |
|----------|-------------|
| `iron_door_bottom.png` | Iron door lower half |
| `iron_door_top.png` | Iron door upper half |
| `iron_bars.png` | Iron bars |
| `ladder.png` | Ladder |
| `torch.png` | Torch (on wall/ground) |
| `redstone_torch.png` | Redstone torch (lit) |
| `redstone_torch_off.png` | Redstone torch (unlit) |
| `crafting_table_front.png` | Crafting table front face |
| `crafting_table_side.png` | Crafting table side face |
| `crafting_table_top.png` | Crafting table top face |
| `furnace_front.png` | Furnace front (unlit) |
| `furnace_front_on.png` | Furnace front (lit) |
| `furnace_side.png` | Furnace side |
| `furnace_top.png` | Furnace top |
| `bookshelf.png` | Bookshelf side |
| `sign.png` | Generic sign face |
| `sea_lantern.png` | Sea lantern |
| `sponge.png` | Sponge |
| `wet_sponge.png` | Wet sponge |

### Copper Variants (Waxed)
| Filename |
|----------|
| `waxed_copper_block.png` |
| `waxed_cut_copper.png` |
| `waxed_exposed_copper.png` |
| `waxed_exposed_cut_copper.png` |
| `waxed_weathered_copper.png` |
| `waxed_weathered_cut_copper.png` |
| `waxed_oxidized_copper.png` |
| `waxed_oxidized_cut_copper.png` |
| `waxed_copper_double1.png`, `waxed_copper_double2.png` |
| `waxed_copper_stairs1.png`, `waxed_copper_stairs2.png` |
| *(+ exposed/weathered/oxidized variants)* |

### Other Block Textures
| Filename | Description |
|----------|-------------|
| `beacon.png` | Beacon |
| `beacon_glass.png` | Beacon glass |
| `blackstone.png` | Blackstone side |
| `blackstone_top.png` | Blackstone top |
| `bone_block_side.png` | Bone block side |
| `bone_block_top.png` | Bone block top |
| `cactus_side.png` | Cactus side |
| `cactus_top.png` | Cactus top |
| `cactus_bottom.png` | Cactus bottom |
| `carved_pumpkin.png` | Carved pumpkin face |
| `jack_o_lantern.png` | Lit jack-o-lantern face |
| `pumpkin_side.png` | Pumpkin side |
| `pumpkin_top.png` | Pumpkin top |
| `pumpkin_bottom.png` | Pumpkin bottom |
| `melon_side.png` | Melon side |
| `melon_top.png` | Melon top |
| `crimson_nylium_side.png` | Crimson nylium side |
| `warped_nylium_side.png` | Warped nylium side |
| `dried_kelp_side.png` | Dried kelp side |
| `dried_kelp_top.png` | Dried kelp top |
| `dried_kelp_bottom.png` | Dried kelp bottom |
| `water_overlay.png` | Water surface overlay |
| `lily_pad.png` | Lily pad |
| `fire_0.png`, `fire_1.png` | Fire animation frames |
| `soul_fire_0.png`, `soul_fire_1.png` | Soul fire frames |
| `soul_torch.png` | Soul torch |
| `redstone_block.png` | Redstone block |
| `redstone_dust_line0.png`, `redstone_dust_line1.png` | Redstone dust |
| `spawner.png` | Mob spawner |
| `spawner_top.png`, `spawner_bottom.png`, `spawner_e.png` | Spawner variants |
| `enchanting_table_top.png` | Enchanting table top |
| `enchanting_table_side.png` | Enchanting table side |
| `enchanting_table_bottom.png` | Enchanting table bottom |
| `dragon_egg.png`, `dragon_egg_e.png` | Dragon egg |

---

## NOT Present (use color fallback)

These common blocks do **not** have textures in this pack:
- `sand.png`, `gravel.png` (terrain blocks)
- `dirt.png`, `grass_block_top.png`
- `oak_log.png`, `oak_log_top.png`, `oak_leaves.png`
- `snow.png` (snow block top)
- `ice.png`
- Any wool color textures (`white_wool.png`, etc.)
- Any concrete textures (only blockstates, no PNGs)
- `sandstone.png`, `sandstone_top.png`, `sandstone_bottom.png`
- `clay.png`

---

## Environment Textures (`textures/environment/`)

| Filename | Description |
|----------|-------------|
| `sun.png` | Sun billboard sprite |
| `moon_phases.png` | Moon phase sprite sheet (4×2 grid) |
| `rain.png` | Rain particle streak |
| `snow.png` | Snow particle flake |
| `end_sky.png` | End dimension sky |

---

## Entity Textures (`textures/entity/`)

### Player
| Filename | Description |
|----------|-------------|
| `steve.png` | Steve skin (64×64) |
| `alex.png` | Alex skin (64×64) |
| `elytra.png` | Elytra wings |

### Mobs
| Filename | Description |
|----------|-------------|
| `enderman/enderman.png` | Enderman skin |
| `enderman/enderman_eyes.png` | Enderman glowing eyes |
| `creeper/creeper.png` | Creeper skin |
| `creeper/creeper_armor.png` | Charged creeper overlay |
| `zombie/drowned.png` | Drowned zombie |
| `zombie/drowned_outer_layer.png` | Drowned outer layer |
| `iron_golem/iron_golem.png` | Iron golem skin |
| `iron_golem/iron_golem_crackiness_*.png` | Iron golem damage states |
| `ghast/ghast.png` | Ghast |
| `ghast/ghast_shooting.png` | Ghast shooting state |
| `endermite.png` | Endermite |
| `blaze.png` | Blaze |
| `wither/wither.png` | Wither |
| `slime/magmacube.png` | Magma cube |

### Cats
`entity/cat/`: all_black, black, jellie, red, siamese, tabby, white

### Signs
`entity/signs/`: oak, birch, spruce, jungle, acacia, dark_oak, crimson, warped, mangrove

### Shulkers (16 colors)
`entity/shulker/shulker_<color>.png` — black, blue, brown, cyan, gray, green, light_blue, light_gray, lime, magenta, orange, pink, purple, red, silver, white, yellow + default `shulker.png`

### Villagers
- `entity/villager/type/`: desert, jungle, plains, savanna, snow, swamp
- `entity/villager/profession/`: armorer, cleric, fisherman, fletcher, leatherworker, toolsmith, weaponsmith

---

## GUI Textures (`textures/gui/`)

### Containers
| Filename | Description |
|----------|-------------|
| `gui/container/inventory.png` | Player inventory UI |
| `gui/container/crafting_table.png` | Crafting table UI |
| `gui/container/furnace.png` | Furnace UI |
| `gui/container/blast_furnace.png` | Blast furnace UI |
| `gui/container/smoker.png` | Smoker UI |
| `gui/container/anvil.png` | Anvil UI |
| `gui/container/beacon.png` | Beacon UI |
| `gui/container/brewing_stand.png` | Brewing stand UI |
| `gui/container/enchanting_table.png` | Enchanting UI |
| `gui/container/generic_54.png` | Large chest UI (6 rows) |
| `gui/container/hopper.png` | Hopper UI |
| `gui/container/dispenser.png` | Dispenser UI |
| `gui/container/shulker_box.png` | Shulker box UI |
| `gui/container/horse.png` | Horse inventory UI |
| `gui/container/grindstone.png` | Grindstone UI |
| `gui/container/loom.png` | Loom UI |
| `gui/container/stonecutter.png` | Stonecutter UI |
| `gui/container/recipe_background.png` | Recipe background |
| `gui/container/stats_icons.png` | Stat icons |
| `gui/container/villager.png`, `villager2.png` | Villager trade UI |

### HUD / General
| Filename | Description |
|----------|-------------|
| `gui/icons.png` | Hearts, hunger, armor, XP bar sprites |
| `gui/bars.png` | XP/boss bars |
| `gui/widgets.png` | Hotbar, buttons, slots |
| `gui/recipe_book.png` | Recipe book panel |
| `gui/toasts.png` | Notification toasts |
| `gui/options_background.png` | Options dirt background |
| `gui/title/minecraft.png` | Minecraft logo |
| `gui/title/edition.png` | Edition text |

### Creative Inventory
- `gui/container/creative_inventory/tab_items.png`
- `gui/container/creative_inventory/tab_inventory.png`
- `gui/container/creative_inventory/tab_item_search.png`
- `gui/container/creative_inventory/tabs.png`

---

## Item Textures (`textures/item/`)

### Tools & Weapons
| Category | Files |
|----------|-------|
| Wooden | `wooden_sword/axe/pickaxe/shovel/hoe.png` |
| Stone | `stone_sword/axe/pickaxe/shovel/hoe.png` |
| Iron | `iron_sword/axe/pickaxe/shovel/hoe.png` |
| Golden | `golden_sword/axe/pickaxe/shovel/hoe.png` |
| Diamond | `diamond_sword/axe/pickaxe/shovel/hoe.png` |
| Netherite | `netherite_sword/axe/pickaxe/shovel/hoe.png` |
| Bow | `bow.png`, `bow_pulling_0/1/2.png` |
| Crossbow | `crossbow_standby/arrow/firework/pulling_0/1/2.png` |
| Trident | `trident.png` |

### Armor
| Category | Files |
|----------|-------|
| Chainmail | `chainmail_helmet/chestplate/leggings/boots.png` |
| Iron | `iron_helmet/chestplate/leggings/boots.png` (no item sprite for iron armor — use color) |
| Golden | `golden_helmet/chestplate/leggings/boots.png` |
| Diamond | `diamond_helmet/chestplate/leggings/boots.png` |
| Leather | `leather_helmet/chestplate/leggings/boots.png` + `_overlay.png` variants |
| Turtle | `turtle_helmet.png` |
| Horse | `diamond_horse_armor.png`, `golden_horse_armor.png`, `iron_horse_armor.png` |

### Doors (Item form)
| Filename | Description |
|----------|-------------|
| `oak_door.png` | Oak door item |
| `iron_door.png` | Iron door item |
| `birch_door.png`, `spruce_door.png` | Birch/Spruce door items |
| `jungle_door.png`, `dark_oak_door.png` | Jungle/Dark oak door items |
| `acacia_door.png` | Acacia door item |
| `crimson_door.png`, `warped_door.png` | Nether door items |
| `mangrove_door.png` | Mangrove door item |

### Buckets & Containers
`bucket.png`, `water_bucket.png`, `lava_bucket.png`, `milk_bucket.png`, `powder_snow_bucket.png`
Fish buckets: `cod_bucket.png`, `salmon_bucket.png`, `tropical_fish_bucket.png`, `pufferfish_bucket.png`, `axolotl_bucket.png`

### Misc Items
| Filename | Description |
|----------|-------------|
| `stick.png` | Stick |
| `bone.png` | Bone |
| `arrow.png` | Arrow |
| `blaze_rod.png` | Blaze rod |
| `book.png` | Book |
| `enchanted_book.png` | Enchanted book |
| `written_book.png`, `writable_book.png` | Written/writable book |
| `fishing_rod.png`, `fishing_rod_cast.png` | Fishing rod |
| `elytra.png`, `broken_elytra.png` | Elytra |
| `shulker_shell.png` | Shulker shell |
| `egg.png` | Egg |
| `gold_ingot.png`, `iron_nugget.png` | Metal items |
| `glass_bottle.png` | Glass bottle |
| `potion.png`, `splash_potion.png`, `lingering_potion.png` | Potions |
| `spawn_egg.png`, `spawn_egg_overlay.png` | Spawn egg |
| `minecart.png` | Minecart |
| `lead.png` | Lead |
| `filled_map.png`, `filled_map_markings.png` | Map |
| `item_frame.png`, `glow_item_frame.png` | Item frames |
| `end_crystal.png` | End crystal |
| `campfire.png`, `soul_campfire.png` | Campfires |
| `dried_kelp.png` | Dried kelp |
| `golden_carrot.png` | Golden carrot |
| `melon_slice.png`, `glistering_melon_slice.png` | Melon |
| `honey_bottle.png` | Honey bottle |
| `experience_bottle.png` | XP bottle |
| `debug_stick.png` | Debug stick |
| `knowledge_book.png` | Knowledge book |

### Clock & Compass (Animated)
- `clock_00.png` through `clock_63.png` (64 frames)
- `compass_00.png` through `compass_31.png` (32 frames)

### Boats
`oak_boat.png`, `birch_boat.png`, `spruce_boat.png`, `jungle_boat.png`, `acacia_boat.png`, `dark_oak_boat.png`, `mangrove_boat.png`
Chest boats: `item/chest_boat/<wood>.png`

### Chests
`item/chest/normal.png`, `normal_left.png`, `normal_right.png`, `trapped.png`, `ender.png`, `christmas.png`

---

## Particle Textures (`textures/particle/`)

| Filename | Description |
|----------|-------------|
| `particles.png` | Main particle sprite sheet |
| `flame.png` | Flame particle |
| `lava.png` | Lava spark |
| `bubble.png` | Water bubble |
| `heart.png` | Heart particle |
| `angry.png` | Angry particle |
| `damage.png` | Damage indicator |
| `footprint.png` | Footprint |
| `flash.png` | Flash effect |
| `nautilus.png` | Nautilus spiral |
| `soul_fire_flame.png` | Soul fire particle |
| `explosion.png` + `explosion_0` through `_15` | Explosion animation |
| `big_smoke_0` through `_11` | Large smoke animation |
| `splash_0` through `_3` | Water splash |
| `sweep_0` through `_8` + `sweep.png` | Sword sweep |

---

## Key Textures for HexaCraft (Quick Reference)

These are the most likely to be used in Phase 3–5 of the building blocks plan:

```
Block textures needed:
  textures/block/oak_planks.png          → BLOCK_OAK_PLANKS, stairs, slabs
  textures/block/stone_bricks.png        → stone slabs, stairs
  textures/block/cobblestone.png         → stone walls
  textures/block/stone_bricks_inf.png    → variation walls
  textures/block/iron_door_bottom.png    → iron door lower
  textures/block/iron_door_top.png       → iron door upper
  textures/block/ladder.png              → climbable ladder
  textures/block/iron_bars.png           → iron bar pane
  textures/block/glass.png               → glass pane
  textures/block/glass_pane_top.png      → glass pane top
  textures/block/torch.png               → placeable torch
  textures/block/crafting_table_front.png → crafting table
  textures/block/crafting_table_top.png   → crafting table
  textures/block/furnace_front.png        → furnace
  textures/block/bookshelf.png            → bookshelf
  textures/block/sea_lantern.png          → lantern block

  NOTE: oak_door texture is NOT present as a block texture.
  Use item/oak_door.png for the door panel sprite instead,
  or use a color-only fallback.

Environment textures:
  textures/environment/sun.png
  textures/environment/moon_phases.png
  textures/environment/rain.png
  textures/environment/snow.png

Entity skins:
  textures/entity/steve.png
  textures/entity/zombie/drowned.png
```

---

## loadTexture() Path Convention

All paths relative to the binary (project root):

```cpp
// Example calls
GLuint texOakPlanks    = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/oak_planks.png");
GLuint texStoneBricks  = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/stone_bricks.png");
GLuint texLadder       = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/ladder.png");
GLuint texIronDoorBot  = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/iron_door_bottom.png");
GLuint texIronDoorTop  = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/iron_door_top.png");
GLuint texIronBars     = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/iron_bars.png");
GLuint texGlass        = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/glass.png");
GLuint texTorch        = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/torch.png");
GLuint texCraftingTop  = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/crafting_table_top.png");
GLuint texBookshelf    = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/bookshelf.png");
GLuint texFurnace      = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/block/furnace_front.png");
GLuint texSun          = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/environment/sun.png");
GLuint texSteve        = loadTexture("FM Default_v1.3.2[MC1.19.3+]/assets/minecraft/textures/entity/steve.png");
```

# GOLD Resource Pack — Comprehensive README

This folder is a **Minecraft modpack resource pack** (FM Default style, pack_format 5 / MC 1.16).
It retextures both vanilla Minecraft and **205 mod namespaces** from what appears to be an
"All The Mods" style modpack. The pack also ships several RTF/DOCX format guide files and horse
armor PNGs at the root level.

---

## Root Files

| File | Purpose |
|------|---------|
| `pack.mcmeta` | MC pack descriptor — format 5, description "Rosalina - from Super Mario Galaxy" |
| `pack.png` | Pack icon (32×32) |
| `horse_armor_diamond/gold/iron.png` | Horse armor overlays |
| `Version History.txt` | Full changelog (Ver 1.x – 3.3, 2016–2018) — 3D doors, 3D rails, animated ores, etc. |
| `AHIT Soundpack [MC1.15+].rtf` | A Hat in Time sound replacements guide |
| `FM_All_the_Mods_10*.rtf` | ATM10 mod-specific texture notes |
| `FM Default_*.rtf` / `FM Vanilla_*.rtf` | Version-specific FM Default release notes |
| `FM_Modded_[MC1.18].rtf` | Modded pack notes for MC 1.18 |
| `FM Optifine Add-On_*.docx` | Optifine CTM / connected-textures add-on guides |
| `EMC_Beta*.docx` | EMC (economy/energy currency) beta docs |
| `z_Custom Options/` | Optional overrides folder |

---

## Asset Structure

```
gold/assets/<namespace>/textures/<category>/
```

All textures are PNG (some have `.mcmeta` animation sidecars).

---

## Vanilla Minecraft (`minecraft` namespace)

**668 block textures · 437 item textures · 431 GUI textures**

### Block Textures — Key Groups

#### Ores (all have `_overlay.png` variants for OptiFine ore highlighting)
```
coal_ore.png / deepslate_coal_ore.png
iron_ore.png / deepslate_iron_ore.png
gold_ore.png / deepslate_gold_ore.png
diamond_ore.png / deepslate_diamond_ore.png
emerald_ore.png / deepslate_emerald_ore.png
lapis_ore.png / deepslate_lapis_ore.png
copper_ore.png / deepslate_copper_ore.png
redstone_ore.png / redstone_ore_on.png (animated) / deepslate_redstone_ore.png
nether_gold_ore.png / nether_quartz_ore.png
```

#### Stone / Masonry
```
stone.png / stone_inf.png          cobblestone.png / cobblestone_inf.png
stone_bricks.png / stone_bricks_inf.png
mossy_stone_bricks.png / mossy_stone_bricks_inf.png
cracked_stone_bricks.png / cracked_stone_bricks_inf.png
chiseled_stone_bricks.png / chiseled_stone_bricks_inf.png
smooth_stone.png / smooth_stone_slab_side.png / stone_slab_side.png
blackstone.png / blackstone_top.png
end_stone.png / end_stone_bricks.png
bricks.png / nether_bricks.png / red_nether_bricks.png
prismarine_bricks.png
```

#### Wood (6 vanilla + Cherry, Bamboo, Mangrove, Crimson, Warped, Pale Oak)
```
oak_planks.png      oak_log.png / oak_log_top.png      stripped_oak_log.png
birch_planks.png    birch_log.png                      stripped_birch_log.png
spruce_planks.png   spruce_log.png                     stripped_spruce_log.png
jungle_planks.png   jungle_log.png                     stripped_jungle_log.png
acacia_planks.png   acacia_log.png                     stripped_acacia_log.png
dark_oak_planks.png dark_oak_log.png                   stripped_dark_oak_log.png
cherry_trapdoor.png (cherry log/planks in same pattern)
bamboo_trapdoor.png
```

#### Doors & Trapdoors (all wood types + Iron, Copper variants)
```
oak_door_bottom.png / oak_door_top.png / oak_door_side.png
birch/spruce/jungle/acacia/dark_oak/cherry/bamboo/mangrove/crimson/warped — same pattern
iron_door_bottom.png / iron_door_top.png / iron_door_side.png
copper_door_bottom/top/side — plus exposed/weathered/oxidized/waxed variants
(trapdoors: oak_trapdoor.png, iron_trapdoor.png, copper_trapdoor.png, etc.)
```

#### Glass & Stained Glass (16 colors)
```
glass.png / glass_pane_top.png / beacon_glass.png
black/blue/brown/cyan/gray/green/light_blue/light_gray/lime/
magenta/orange/pink/purple/red/white/yellow _stained_glass.png
```

#### Terrain / Environment
```
grass_block_top.png / grass_block_side.png / grass_block_side_overlay.png
dirt.png / coarse_dirt.png / sand.png / red_sand.png / gravel.png
sandstone.png / sandstone_top.png / sandstone_bottom.png
red_sandstone.png / chiseled_sandstone.png / cut_sandstone.png
ice.png / packed_ice.png / snow.png
obsidian.png / netherrack.png / soul_sand.png / bedrock.png
```

#### Lights & Special
```
glowstone.png / sea_lantern.png (animated) / redstone_lamp.png / redstone_lamp_on.png
torch.png / soul_torch.png / redstone_torch.png / soulbound_torch.png
bookshelf.png / ladder.png / anvil.png / anvil_top.png
redstone_block.png / beacon.png
```

#### Terracotta
```
cyan_glazed_terracotta.png  (other glazed terracotta in full vanilla set)
```

#### Special Jack-o-Lanterns (custom skin variants)
```
empire_carver_jack_o_lantern_*.png
haunted_jack_o_lantern_*.png
spooky_jack_o_lantern_*.png
```

---

## Mod Namespaces with Block Textures

### `chipped` — 516 block textures
**Chipped** adds alternate visual styles for vanilla doors, trapdoors, ladders, and lily pads.
Organized into subdirectories by block type, each containing many style variants.

```
textures/block/oak_door/          → barred, beach, boarded, chipped, dual_paneled, ...
textures/block/oak_trapdoor/      → many style variants
textures/block/acacia_door/ birch_door/ jungle_door/ dark_oak_door/ ...
textures/block/ladder/            → alternate ladder textures
textures/block/lily_pad/          → alternate lily pad textures
+ direct PNGs: acacia_bamboo_door_lower/upper, acacia_barn_door_lower/upper, ...
```

### `mcwdoors` — 450 block textures (Macaw's Doors)
Massive library of door styles per wood type:
```
acacia_bamboo_door_lower/upper.png
acacia_barn_door_lower/upper.png
acacia_beach_door_lower/upper.png
acacia_classic/cottage/four_panel/japanese/modern/mystic/nether/paper/swamp/waffle/whispering_door_...
(all patterns repeated for all wood types)
```

### `quark` — 114 block textures
Quark adds wood types and utility blocks:
```
azalea_bookshelf/ladder/door/trapdoor/log/planks/sign.png
blossom_bookshelf/ladder/door/trapdoor/log/planks/sign.png
birch/crimson/dark_oak/spruce/warped _bookshelf/_ladder.png
black/blue/brown/cyan/gray/green/... _framed_glass.png    (16-color framed glass)
blackstone_furnace_front/side/top.png + on/soul variants
chute_bottom/side/top.png
cut_vine.png
deepslate_furnace_front/side.png
```

### `railcraft` — 165 block textures
Train tracks for many types (abandoned, activator, booster, control, coupler, detector, etc.):
```
abandoned_track_*.png / abandoned_junction_track.png / abandoned_turnout_*.png
activator_track.png / activator_track_on.png
booster_track.png / control_track.png
coupler_track_auto_coupler.png / coupler_track_decoupler.png
detector_track.png / disembarking_track_*.png
(+ many more track variants with _on / _switched states)
```

### `biomeswevegone` — 50 block textures
Custom wood types and biome-specific blocks:
```
aspen/ baobab/ blue_enchanted/ cika/ cypress/ ebony/ fir/ florus/
green_enchanted/ holly/ ironwood/ jacaranda/ mahogany/ maple/ palm/
pine/ rainbow_eucalyptus/ redwood/ sakura/ skyris/ spirit/ white_mangrove/
willow/ witch_hazel/ zelkova/  (each has log/planks subdirs or side PNGs)
baobab_door_side.png / baobab_trapdoor_side.png
blackwood_door_side.png / blue_bioshroom_door_side.png ...
brimwood_door_bottom/top.png (animated) / brimwood_trapdoor.png (animated)
```

### `regions_unexplored` — 51 block textures
```
cradlewood_door_bottom/top.png / cradlewood_trapdoor.png
jinglestem_door_bottom/top.png / jinglestem_trapdoor.png
starlight_mangrove_door_bottom/top.png
(+ ore and other block textures for the mod's dimensions)
```

### `eternal_starlight` — 29 block textures
Fantasy ores in ice/haze/grimstone:
```
eternal_ice_atalphaite_ore.png / eternal_ice_redstone_ore.png / eternal_ice_saltpeter_ore.png
grimstone_atalphaite_ore.png / grimstone_malarite_ore.png / grimstone_redstone_ore.png
grimstone_saltpeter_ore.png
haze_ice_atalphaite_ore.png / haze_ice_redstone_ore.png / haze_ice_saltpeter_ore.png
```

### `forbidden_arcanus` — 29 block textures
Magic mod with unique door/trapdoor/ore:
```
arcane_edelwood_door_bottom/top.png / arcane_edelwood_trapdoor.png
arcane_crystal_ore/ (subdir)
aurum_door_bottom/top/side.png / aurum_trapdoor.png
deorum_door_bottom/top/side.png / deorum_trapdoor.png
edelwood_door_bottom/top/side.png / edelwood_trapdoor.png
fungyss_door_bottom/top/side.png / fungyss_trapdoor.png
```

### `allthemodium` — 16 block textures
ATM-specific dimensional materials:
```
ancient_door_bottom/top/side.png / ancient_trap_door.png / ancient_trap_door_side.png
ancient_grass.png / ancient_grass_side.png / ancient_grass_top.png
demonic_door_bottom/top/side.png
soul_door_bottom/top/side.png / soul_trap_door.png / soul_trap_door_side.png
```

### `azalea` — 16 block textures (Azalea mod, separate from Quark's azalea)
```
azalea_door_bottom/top.png / azalea_trapdoor.png / azalea_log.png /
azalea_planks.png / azalea_sign.png / azalea_log_top.png
flowering_azalea_door_bottom/top.png / flowering_azalea_log.png /
flowering_azalea_planks.png / stripped_azalea_log.png / stripped_azalea_log_top.png
```

### `bellsandwhistles` — 32 block textures
Train-themed decorative blocks (likely Supplementaries/Create adjacent):
```
(32 PNG textures for bells, whistles, and other train/industrial props)
```

### `farmersdelight` — 17 block textures
Food / kitchen mod:
```
(17 block textures — counters, cutting boards, stoves, cooking pots, etc.)
```

### `mekanism` — 10 block textures
Tech mod ores:
```
deepslate_fluorite_ore.png / deepslate_lead_ore.png / deepslate_osmium_ore.png
deepslate_tin_ore.png / deepslate_uranium_ore.png
fluorite_ore.png / lead_ore.png / osmium_ore.png / tin_ore.png / uranium_ore.png
```

### `deeperdarker` — 9 block textures
```
echo_door_bottom/top/side.png / echo_trapdoor.png / echo_trapdoor_side.png
gloomslate_redstone_ore_off/on.png
sculk_stone_redstone_ore_off/on.png
treated_door_bottom/top.png / treated_trapdoor.png
```

### `ironfurnaces` — 8 block textures
Tier furnaces (Iron, Gold, Diamond, Netherite, Obsidian):
```
obsidian_furnace_front.png / obsidian_furnace_front_on.png (animated)
obsidian_furnace_side.png / obsidian_furnace_top_smoke.png
(+ blast / smoke variants)
```

### `aether` — 6 block textures
Aether dimension:
```
ambrosium_ore.png / gravitite_ore.png / zanite_ore.png
skyroot_door_bottom/top.png / skyroot_trapdoor.png
```

### `naturesaura` — 6 block textures
```
(6 nature/aura themed block textures)
```

### `torchmaster` — 3 block textures (+ PSD source files)
```
feral_flare_lantern.png / megatorch.png / megatorch_top.png
```

### `oritech` — 16 block textures (tech mod ores)
```
deepslate_nickel_ore.png / deepslate_platinum_ore.png / deepslate_uranium_ore.png
endstone_platinum_ore.png / nickel_ore.png
resource_node_coal/copper/diamond/emerald/gold/iron/lapis/nickel/platinum/redstone/uranium.png
```

### `supplementaries` — 4 block textures
```
(4 decorative block textures — signs, etc.)
```

### `cobblegengalore` — 21 block textures
Cobblestone variants and generators.

### `immersiveengineering` — 3 block textures
### `enderio` — 4 block textures
### `bibliocraft` — 2 block textures
### `bigreactors` — 5 block textures
### `goldenhopper` — 3 block textures
### `pipez` — 5 block textures
### `copperequipment` — 2 block textures
### `everythingcopper` — 13 block textures
Copper-tier hoppers (normal, exposed, weathered, oxidized, waxed variants):
```
copper_hopper_inside/outside/top.png
exposed_copper_hopper_inside/outside/top.png
oxidized_copper_hopper_inside/outside/top.png
weathered_copper_hopper_inside/outside/top.png
```

### `mysticalagriculture` — 3 block textures
### `productivebees` — 3 block textures
### `buzzier_bees` — 3 block textures
### `stevescarts` — 4 block textures
### `utilitarian` — 2 block textures

---

## Other Asset Directories (no block textures but have other assets)

| Namespace | Has |
|-----------|-----|
| `create` | 30 GUI textures, 9 item textures |
| `ae2` | 17 item textures |
| `twilightforest` | GUI textures |
| `quark` | 4 GUI textures |
| `minecraft` | Full GUI (431) + items (437) + entity + environment + particle |
| `chipped` | Models, blockstates |
| `mcwdoors` | Models, blockstates |
| `cfm` (Corail's/Comforts Furniture Mod) | Textures but not in block/ |
| `refurbished_furniture` | Textures |
| `simplylight` | Textures |
| `pneumaticcraft` | Textures |

---

## How to Use in HexaCraft

All textures are loaded once at startup using `loadTexture()` from a relative path.
The base path from the project root is `gold/assets/<namespace>/textures/block/<filename>`.

### Step 1 — Declare GLuint variable in `globals.h`
```cpp
GLuint texOakPlanksGold;          // will hold the loaded texture ID
GLuint texObsidianFurnaceFront;
GLuint texMegatorch;
```

### Step 2 — Load in `main.cpp` (after `loadShaders()`)
```cpp
#define GOLD_BLOCK(var, ns, file) \
    var = loadTexture("gold/assets/" ns "/textures/block/" file)

// Vanilla retextures (drop-in replacements for FM Default textures)
GOLD_BLOCK(texOakPlanksGold,        "minecraft",   "oak_planks.png");
GOLD_BLOCK(texStoneBricksGold,      "minecraft",   "stone_bricks.png");
GOLD_BLOCK(texCobblestoneGold,      "minecraft",   "cobblestone.png");
GOLD_BLOCK(texGlassGold,            "minecraft",   "glass.png");
GOLD_BLOCK(texGlassPaneTopGold,     "minecraft",   "glass_pane_top.png");
GOLD_BLOCK(texLadderGold,           "minecraft",   "ladder.png");
GOLD_BLOCK(texTorchGold,            "minecraft",   "torch.png");
GOLD_BLOCK(texSeaLanternGold,       "minecraft",   "sea_lantern.png");
GOLD_BLOCK(texGlowstoneGold,        "minecraft",   "glowstone.png");
GOLD_BLOCK(texBookshelfGold,        "minecraft",   "bookshelf.png");
GOLD_BLOCK(texDiamondOreGold,       "minecraft",   "diamond_ore.png");
GOLD_BLOCK(texGoldOreGold,          "minecraft",   "gold_ore.png");
GOLD_BLOCK(texIronOreGold,          "minecraft",   "iron_ore.png");

// Door textures (bottom = lower half, top = upper half)
GOLD_BLOCK(texOakDoorBottom,        "minecraft",   "oak_door_bottom.png");
GOLD_BLOCK(texOakDoorTop,           "minecraft",   "oak_door_top.png");
GOLD_BLOCK(texIronDoorBottomGold,   "minecraft",   "iron_door_bottom.png");
GOLD_BLOCK(texIronDoorTopGold,      "minecraft",   "iron_door_top.png");

// Mod-specific blocks
GOLD_BLOCK(texMegatorch,            "torchmaster", "megatorch.png");
GOLD_BLOCK(texMegatorchTop,         "torchmaster", "megatorch_top.png");
GOLD_BLOCK(texFeralFlareLantern,    "torchmaster", "feral_flare_lantern.png");
GOLD_BLOCK(texOsmiumOre,            "mekanism",    "osmium_ore.png");
GOLD_BLOCK(texLeadOre,              "mekanism",    "lead_ore.png");
GOLD_BLOCK(texTinOre,               "mekanism",    "tin_ore.png");
GOLD_BLOCK(texUraniumOre,           "mekanism",    "uranium_ore.png");
GOLD_BLOCK(texAncientGrass,         "allthemodium","ancient_grass.png");
GOLD_BLOCK(texSoulDoorBottom,       "allthemodium","soul_door_bottom.png");
GOLD_BLOCK(texEchoDoorBottom,       "deeperdarker","echo_door_bottom.png");
GOLD_BLOCK(texAmbrosiumOre,         "aether",      "ambrosium_ore.png");
GOLD_BLOCK(texZaniteOre,            "aether",      "zanite_ore.png");
```

### Step 3 — Bind in `world.h` inside `getBlockTexture()` or `bindBlockTexture()`
```cpp
case BLOCK_OAK_PLANKS:    return texOakPlanksGold;   // swap FM Default → Gold pack
case BLOCK_TORCH:          return texMegatorch;
case BLOCK_OSMIUM_ORE:     return texOsmiumOre;
```

### Step 4 — Use in render dispatch
Textures are already applied automatically by `bindBlockTexture()` + the existing render
dispatch in `renderTerrain()`. No extra shader changes needed — `textureMode` 1 or 2 controls
whether the texture replaces or tints the block color.

---

## Animated Textures (`.mcmeta` sidecars)

Some textures have `.mcmeta` files indicating they are animated sprite sheets:
```
sea_lantern.png.mcmeta
diamond_ore.png.mcmeta
emerald_ore.png.mcmeta
gold_ore.png.mcmeta
redstone_ore_on.mcmeta
soul_campfire_log_lit.png.mcmeta
brimwood_door_bottom.png.mcmeta  (biomeswevegone)
```
OpenGL with `stbi_load` loads only the first frame. To use animations you would need
to load the sprite sheet and step through it using a timer uniform in the fragment shader.
For now, loading these as static textures gives the first frame — still looks good.

---

## `_inf` Texture Variants

Several vanilla textures have `_inf.png` variants (e.g. `cobblestone_inf.png`,
`stone_bricks_inf.png`). These are **infinitely-tiling** versions with subtle random
variation to break repetition on large surfaces — identical usage, just visually better for
terrain. Prefer the `_inf` variant for terrain blocks if available.

```cpp
GOLD_BLOCK(texCobblestoneInf, "minecraft", "cobblestone_inf.png");
GOLD_BLOCK(texStoneBricksInf, "minecraft", "stone_bricks_inf.png");
GOLD_BLOCK(texMossyCobblInf,  "minecraft", "mossy_cobblestone.png");
```

---

## `_overlay` Ore Variants

Vanilla ores ship with `_overlay.png` files (e.g. `coal_ore_overlay.png`). These are the
mineral blob drawn on top of a stone background — used by OptiFine for per-biome ore tinting.
In HexaCraft, just use the base `coal_ore.png` (full composite texture). The overlay files
can be ignored unless you want to implement multi-texture ore rendering.

---

## Summary Table — Most Useful Textures for HexaCraft

| Category | Files | Path prefix |
|----------|-------|-------------|
| Stone/brick terrain | `stone.png`, `cobblestone.png`, `stone_bricks.png`, `*_inf.png` | `minecraft/` |
| Wood planks & logs | `oak_planks.png` … `dark_oak_planks.png`, all `*_log.png` | `minecraft/` |
| Ores | `coal_ore.png` … `nether_gold_ore.png` + deepslate variants | `minecraft/` |
| Doors (vanilla) | `oak_door_bottom/top.png` … `iron_door_bottom/top.png` | `minecraft/` |
| Glass | `glass.png`, `glass_pane_top.png`, 16 `*_stained_glass.png` | `minecraft/` |
| Lights | `torch.png`, `sea_lantern.png`, `glowstone.png`, `redstone_lamp.png` | `minecraft/` |
| Mod ores | `osmium/lead/tin/uranium_ore.png` | `mekanism/` |
| Mod ores | `nickel/platinum/deepslate_nickel/platinum_ore.png` | `oritech/` |
| Mod ores | `ambrosium/gravitite/zanite_ore.png` | `aether/` |
| Mod lights | `megatorch.png`, `feral_flare_lantern.png` | `torchmaster/` |
| Fantasy blocks | `ancient_grass.png`, `soul_door_*.png`, `demonic_door_*.png` | `allthemodium/` |
| Echo blocks | `echo_door_*.png`, `echo_trapdoor.png` | `deeperdarker/` |
| Wood variety | `brimwood_door_*.png`, 25 biome wood types | `biomeswevegone/` |
| Door styles | 450+ style variants per wood type | `mcwdoors/` |
| Alt door styles | Alternate style variants (barred, boarded, beach, etc.) | `chipped/` |
| Bookshelf variety | `acacia/birch/dark_oak/... _bookshelf.png` | `quark/` |
| Framed glass | `black/blue/... _framed_glass.png` | `quark/` |
| Track blocks | `abandoned/activator/booster/detector_track*.png` | `railcraft/` |
| Star ores | `grimstone_malarite_ore.png`, `eternal_ice_atalphaite_ore.png` | `eternal_starlight/` |

#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

// =====================================================
// Constants
// =====================================================
const int WIN_W = 1280, WIN_H = 720;
const float HEX_RADIUS = 0.5f;
const float HEX_HEIGHT = 1.0f;
const float PI = 3.14159265358979f;

// Block grid dimensions
const int GRID_W = 200;   // col range: -100 to 99
const int GRID_D = 200;   // row range: -100 to 99
const int GRID_H = 32;    // height layers 0..31
const int GRID_OFF_X = 100; // offset so col=-100 maps to index 0
const int GRID_OFF_Z = 100;

// Block types
enum BlockType {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_SAND,
    BLOCK_STONE,
    BLOCK_WATER,
    BLOCK_WOOD,
    BLOCK_LEAF,
    BLOCK_ORE_DIAMOND,
    BLOCK_ORE_GOLD,
    BLOCK_GLASS,
    BLOCK_STONE_LIGHT,
    BLOCK_BEDROCK,
    BLOCK_COAL_ORE,
    BLOCK_GRAVEL,
    BLOCK_CLAY,
    BLOCK_SNOW,
    BLOCK_ICE,
    BLOCK_PLANKS,
    BLOCK_CRAFTING_TABLE,
    BLOCK_SANDSTONE,
    BLOCK_BRICKS,
    BLOCK_WOOL_WHITE,
    BLOCK_WOOL_RED,
    BLOCK_WOOL_BLUE,
    BLOCK_COBBLESTONE,
    BLOCK_MOSSY_COBBLESTONE,
    BLOCK_GLOWSTONE,
    BLOCK_POLISHED_DIORITE,
    BLOCK_POLISHED_GRANITE,
    BLOCK_POLISHED_ANDESITE,
    BLOCK_QUARTZ_BLOCK,
    BLOCK_IRON_BLOCK,
    BLOCK_DIAMOND_BLOCK,
    BLOCK_GOLD_BLOCK,
    BLOCK_CUT_SANDSTONE,
    BLOCK_MOSSY_BRICKS,
    BLOCK_OBSIDIAN,

    // --- Building & Decoration Blocks ---
    // Wool colors
    BLOCK_WOOL_GREEN, BLOCK_WOOL_YELLOW, BLOCK_WOOL_BLACK, BLOCK_WOOL_ORANGE,
    BLOCK_WOOL_PINK, BLOCK_WOOL_PURPLE, BLOCK_WOOL_CYAN, BLOCK_WOOL_BROWN,
    BLOCK_WOOL_GRAY, BLOCK_WOOL_LIGHT_GRAY, BLOCK_WOOL_MAGENTA, BLOCK_WOOL_LIME,

    // Full blocks
    BLOCK_BOOKSHELF, BLOCK_SMOOTH_STONE, BLOCK_TERRACOTTA,

    // Slabs (half-height)
    BLOCK_SLAB_STONE, BLOCK_SLAB_WOOD, BLOCK_SLAB_SANDSTONE, BLOCK_SLAB_BRICK,

    // Stairs (two-step)
    BLOCK_STAIRS_STONE, BLOCK_STAIRS_WOOD,

    // Carpets (paper-thin)
    BLOCK_CARPET_WHITE, BLOCK_CARPET_RED, BLOCK_CARPET_BLUE,

    // Thin blocks
    BLOCK_GLASS_PANE, BLOCK_IRON_BARS, BLOCK_FENCE_WOOD,

    // Interactive blocks
    BLOCK_DOOR_OAK, BLOCK_DOOR_IRON, BLOCK_TRAPDOOR_OAK, BLOCK_FENCE_GATE, BLOCK_LADDER,

    // Decorative
    BLOCK_LANTERN, BLOCK_TORCH_BLOCK, BLOCK_SIGN, BLOCK_BANNER,

    BLOCK_COUNT,

    // ---- Items (non-placeable) ----
    ITEM_STICK = BLOCK_COUNT,

    // Wood tools
    ITEM_SWORD_WOOD, ITEM_AXE_WOOD, ITEM_PICKAXE_WOOD, ITEM_SHOVEL_WOOD,
    // Stone tools
    ITEM_SWORD_STONE, ITEM_AXE_STONE, ITEM_PICKAXE_STONE, ITEM_SHOVEL_STONE,
    // Iron tools
    ITEM_SWORD_IRON, ITEM_AXE_IRON, ITEM_PICKAXE_IRON, ITEM_SHOVEL_IRON,
    // Diamond tools
    ITEM_SWORD_DIAMOND, ITEM_AXE_DIAMOND, ITEM_PICKAXE_DIAMOND, ITEM_SHOVEL_DIAMOND,
    // Ranged
    ITEM_BOW,

    // Terrain sculpt tools (plan_2 Step 3). Not craftable and not consumed —
    // these edit the world in bulk rather than a block at a time, so they are
    // given, never earned. See sculptHill/sculptPond in input.h.
    ITEM_TOOL_HILL, ITEM_TOOL_POND,

    ITEM_COUNT
};

// Underground depth: all terrain raised by this many blocks to create minable underground
const int UNDERGROUND_DEPTH = 10;

// 3D voxel grid
int blockGrid[GRID_W][GRID_D][GRID_H];
// Cache: highest non-air block per column (avoids iterating empty air in render)
int columnMaxH[GRID_W][GRID_D];

// =====================================================
// Block State (for interactive/special blocks)
// Stores extra per-block data: facing, open/closed, etc.
// Bits 0-1: facing direction (0=N, 1=E, 2=S, 3=W)
// Bit 2: open/closed (doors, trapdoors, fence gates)
// Bit 3: top/bottom (slabs, trapdoors)
// =====================================================
inline uint32_t blockStateKey(int col, int row, int h) {
    return ((uint32_t)(col + GRID_OFF_X) << 14) |
           ((uint32_t)(row + GRID_OFF_Z) << 6) |
           ((uint32_t)h);
}
std::unordered_map<uint32_t, uint16_t> blockState;

// =====================================================
// Block Shape & Properties
// =====================================================
enum BlockShape {
    SHAPE_FULL_HEX, SHAPE_SLAB, SHAPE_STAIR, SHAPE_CARPET,
    SHAPE_PANE, SHAPE_FENCE, SHAPE_DOOR, SHAPE_TRAPDOOR,
    SHAPE_SMALL_HEX, SHAPE_FLAT_PANEL
};

struct BlockProperties {
    BlockShape shape;
    bool isSolid;
    bool isInteractive;  // right-click toggles state
    bool isTransparent;  // needs alpha blending
    bool isEmissive;     // glows
    bool isClimbable;    // ladders
    bool isItem;         // non-placeable inventory item (tools, etc.)
};

BlockProperties getBlockProps(int type) {
    //                          shape            solid  interact transp emiss  climb  item
    switch (type) {
        // Slabs
        case BLOCK_SLAB_STONE:
        case BLOCK_SLAB_WOOD:
        case BLOCK_SLAB_SANDSTONE:
        case BLOCK_SLAB_BRICK:
            return {SHAPE_SLAB,      true,  false, false, false, false, false};

        // Stairs
        case BLOCK_STAIRS_STONE:
        case BLOCK_STAIRS_WOOD:
            return {SHAPE_STAIR,     true,  false, false, false, false, false};

        // Carpets
        case BLOCK_CARPET_WHITE:
        case BLOCK_CARPET_RED:
        case BLOCK_CARPET_BLUE:
            return {SHAPE_CARPET,    false, false, false, false, false, false};

        // Thin blocks
        case BLOCK_GLASS_PANE:
            return {SHAPE_PANE,      false, false, true,  false, false, false};
        case BLOCK_IRON_BARS:
            return {SHAPE_PANE,      false, false, false, false, false, false};
        case BLOCK_FENCE_WOOD:
            return {SHAPE_FENCE,     true,  false, false, false, false, false};

        // Interactive
        case BLOCK_DOOR_OAK:
        case BLOCK_DOOR_IRON:
            return {SHAPE_DOOR,      true,  true,  false, false, false, false};
        case BLOCK_TRAPDOOR_OAK:
            return {SHAPE_TRAPDOOR,  true,  true,  false, false, false, false};
        case BLOCK_FENCE_GATE:
            return {SHAPE_FENCE,     true,  true,  false, false, false, false};
        case BLOCK_LADDER:
            return {SHAPE_FLAT_PANEL,false, false, false, false, true,  false};

        // Decorative
        case BLOCK_LANTERN:
            return {SHAPE_SMALL_HEX, false, false, false, true,  false, false};
        case BLOCK_TORCH_BLOCK:
            return {SHAPE_SMALL_HEX, false, false, false, true,  false, false};
        case BLOCK_SIGN:
            return {SHAPE_FLAT_PANEL,false, false, false, false, false, false};
        case BLOCK_BANNER:
            return {SHAPE_FLAT_PANEL,false, false, false, false, false, false};

        // Existing transparent blocks
        case BLOCK_GLASS:
            return {SHAPE_FULL_HEX,  true,  false, true,  false, false, false};
        case BLOCK_WATER:
            return {SHAPE_FULL_HEX,  false, false, true,  false, false, false};
        case BLOCK_ICE:
            return {SHAPE_FULL_HEX,  true,  false, true,  false, false, false};
        case BLOCK_GLOWSTONE:
            return {SHAPE_FULL_HEX,  true,  false, false, true,  false, false};
        case BLOCK_ORE_DIAMOND:
        case BLOCK_ORE_GOLD:
            return {SHAPE_FULL_HEX,  true,  false, false, true,  false, false};

        // Tools & items (non-placeable)
        case ITEM_STICK:
        case ITEM_SWORD_WOOD:  case ITEM_AXE_WOOD:  case ITEM_PICKAXE_WOOD:  case ITEM_SHOVEL_WOOD:
        case ITEM_SWORD_STONE: case ITEM_AXE_STONE: case ITEM_PICKAXE_STONE: case ITEM_SHOVEL_STONE:
        case ITEM_SWORD_IRON:  case ITEM_AXE_IRON:  case ITEM_PICKAXE_IRON:  case ITEM_SHOVEL_IRON:
        case ITEM_SWORD_DIAMOND: case ITEM_AXE_DIAMOND: case ITEM_PICKAXE_DIAMOND: case ITEM_SHOVEL_DIAMOND:
        case ITEM_BOW:
        case ITEM_TOOL_HILL: case ITEM_TOOL_POND:
            return {SHAPE_FULL_HEX,  false, false, false, false, false, true};

        // Default: solid full hex
        default:
            return {SHAPE_FULL_HEX,  true,  false, false, false, false, false};
    }
}

// Inventory Structures
struct InventorySlot {
    int type;       // BlockType (0 = AIR)
    int count;      // max 64
    int durability; // -1 = infinite (blocks/non-tools), 0 = broken, >0 = uses remaining
};

// Max durability per tool tier
int getMaxDurability(int toolType) {
    switch (toolType) {
        case ITEM_SWORD_WOOD:    case ITEM_AXE_WOOD:
        case ITEM_PICKAXE_WOOD:  case ITEM_SHOVEL_WOOD:    return 59;
        case ITEM_SWORD_STONE:   case ITEM_AXE_STONE:
        case ITEM_PICKAXE_STONE: case ITEM_SHOVEL_STONE:   return 131;
        case ITEM_SWORD_IRON:    case ITEM_AXE_IRON:
        case ITEM_PICKAXE_IRON:  case ITEM_SHOVEL_IRON:    return 250;
        case ITEM_SWORD_DIAMOND: case ITEM_AXE_DIAMOND:
        case ITEM_PICKAXE_DIAMOND: case ITEM_SHOVEL_DIAMOND: return 1561;
        case ITEM_BOW:                                     return 384;
        case ITEM_STICK:                                   return -1;
        default:                                           return -1; // blocks/non-tools
    }
}

// 0-8: Hotbar. 9-35: Storage Grid
InventorySlot playerInventory[36];
// Crafting UI
InventorySlot craftingGrid[9];
InventorySlot craftingOutput;

// =====================================================
// Crafting Recipes
// =====================================================
struct CraftingRecipe {
    int pattern[9];   // 3x3 grid: BlockType per cell (0 = empty)
    int resultType;
    int resultCount;
};

// _ = BLOCK_AIR (empty), pattern is row-major: [0-2]=top row, [3-5]=mid, [6-8]=bottom
const CraftingRecipe recipes[] = {
    // Wood Log -> 4 Planks
    {{0,0,0, 0,BLOCK_WOOD,0, 0,0,0}, BLOCK_PLANKS, 4},

    // 2 Stone -> Stone Bricks
    {{0,0,0, 0,0,0, BLOCK_STONE,BLOCK_STONE,0}, BLOCK_STONE_LIGHT, 4},

    // 4 Sand -> 1 Glass (smelting proxy)
    {{BLOCK_SAND,BLOCK_SAND,0, BLOCK_SAND,BLOCK_SAND,0, 0,0,0}, BLOCK_GLASS, 1},

    // 3 Stone -> 3 Slabs (proxy: gravel)
    {{BLOCK_STONE,BLOCK_STONE,BLOCK_STONE, 0,0,0, 0,0,0}, BLOCK_GRAVEL, 3},

    // 2 Dirt + 1 Leaf -> 1 Grass
    {{0,BLOCK_LEAF,0, 0,BLOCK_DIRT,0, 0,BLOCK_DIRT,0}, BLOCK_GRASS, 1},

    // 4 Snow -> Ice
    {{0,0,0, BLOCK_SNOW,BLOCK_SNOW,0, BLOCK_SNOW,BLOCK_SNOW,0}, BLOCK_ICE, 1},

    // Clay -> 4 Bricks
    {{0,0,0, 0,BLOCK_CLAY,0, 0,0,0}, BLOCK_BRICKS, 4},
    // Planks x4 -> Crafting Table
    {{0,0,0, BLOCK_PLANKS,BLOCK_PLANKS,0, BLOCK_PLANKS,BLOCK_PLANKS,0}, BLOCK_CRAFTING_TABLE, 1},
    // Sand x4 -> Sandstone
    {{0,0,0, BLOCK_SAND,BLOCK_SAND,0, BLOCK_SAND,BLOCK_SAND,0}, BLOCK_SANDSTONE, 1},
    // Sandstone x4 -> Cut Sandstone
    {{0,0,0, BLOCK_SANDSTONE,BLOCK_SANDSTONE,0, BLOCK_SANDSTONE,BLOCK_SANDSTONE,0}, BLOCK_CUT_SANDSTONE, 1},
    // Stone Light x4 -> Bricks (replacing proxy)
    {{0,0,0, BLOCK_STONE_LIGHT,BLOCK_STONE_LIGHT,0, BLOCK_STONE_LIGHT,BLOCK_STONE_LIGHT,0}, BLOCK_BRICKS, 1},
    // Red Wool (String proxy = Grass + Red Dye proxy = Dirt) -> red wool
    {{BLOCK_DIRT,0,0, BLOCK_GRASS,BLOCK_GRASS,0, BLOCK_GRASS,BLOCK_GRASS,0}, BLOCK_WOOL_RED, 1},
    // Blue Wool
    {{BLOCK_WATER,0,0, BLOCK_GRASS,BLOCK_GRASS,0, BLOCK_GRASS,BLOCK_GRASS,0}, BLOCK_WOOL_BLUE, 1},
    // White Wool (4 Grass representing String proxy)
    {{0,0,0, BLOCK_GRASS,BLOCK_GRASS,0, BLOCK_GRASS,BLOCK_GRASS,0}, BLOCK_WOOL_WHITE, 1},
    // Stone + Stone... wait, Cobblestone = breaking Stone
    {{0,0,0, BLOCK_STONE,BLOCK_STONE,0, BLOCK_STONE,BLOCK_STONE,0}, BLOCK_COBBLESTONE, 4},
    // Mossy Cobblestone = Cobble + Leaf
    {{0,0,0, BLOCK_COBBLESTONE,BLOCK_LEAF,0, 0,0,0}, BLOCK_MOSSY_COBBLESTONE, 1},
    // Mossy Bricks = Stone Light + Leaf
    {{0,0,0, BLOCK_STONE_LIGHT,BLOCK_LEAF,0, 0,0,0}, BLOCK_MOSSY_BRICKS, 1},
    // Glowstone = 4 Sand + 4 Wood ? (proxy)
    {{0,0,0, BLOCK_WOOD,BLOCK_WOOD,0, BLOCK_WOOD,BLOCK_WOOD,0}, BLOCK_GLOWSTONE, 1},
    // Polished Diorite = 4 Gravel
    {{0,0,0, BLOCK_GRAVEL,BLOCK_GRAVEL,0, BLOCK_GRAVEL,BLOCK_GRAVEL,0}, BLOCK_POLISHED_DIORITE, 1},
    // Polished Granite = Diorite + Dirt
    {{0,0,0, BLOCK_POLISHED_DIORITE,BLOCK_DIRT,0, 0,0,0}, BLOCK_POLISHED_GRANITE, 1},
    // Polished Andesite = Diorite + Cobblestone
    {{0,0,0, BLOCK_POLISHED_DIORITE,BLOCK_COBBLESTONE,0, 0,0,0}, BLOCK_POLISHED_ANDESITE, 1},
    // Quartz Block = 4 Snow
    {{0,0,0, BLOCK_SNOW,BLOCK_SNOW,0, BLOCK_SNOW,BLOCK_SNOW,0}, BLOCK_QUARTZ_BLOCK, 1},
    // Iron Block = 9 Stone
    {{BLOCK_STONE,BLOCK_STONE,BLOCK_STONE, BLOCK_STONE,BLOCK_STONE,BLOCK_STONE, BLOCK_STONE,BLOCK_STONE,BLOCK_STONE}, BLOCK_IRON_BLOCK, 1},
    // Diamond Block = 9 Diamond Ore
    {{BLOCK_ORE_DIAMOND,BLOCK_ORE_DIAMOND,BLOCK_ORE_DIAMOND, BLOCK_ORE_DIAMOND,BLOCK_ORE_DIAMOND,BLOCK_ORE_DIAMOND, BLOCK_ORE_DIAMOND,BLOCK_ORE_DIAMOND,BLOCK_ORE_DIAMOND}, BLOCK_DIAMOND_BLOCK, 1},
    // Gold Block = 9 Gold Ore
    {{BLOCK_ORE_GOLD,BLOCK_ORE_GOLD,BLOCK_ORE_GOLD, BLOCK_ORE_GOLD,BLOCK_ORE_GOLD,BLOCK_ORE_GOLD, BLOCK_ORE_GOLD,BLOCK_ORE_GOLD,BLOCK_ORE_GOLD}, BLOCK_GOLD_BLOCK, 1},
    // Obsidian = Water + Bedrock
    {{0,0,0, BLOCK_WATER,BLOCK_BEDROCK,0, 0,0,0}, BLOCK_OBSIDIAN, 1},

    // --- BUILDING & DECORATION BLOCK RECIPES ---

    // Slabs (3 in a row = 6 slabs)
    {{BLOCK_STONE,BLOCK_STONE,BLOCK_STONE, 0,0,0, 0,0,0}, BLOCK_SLAB_STONE, 6},
    {{BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS, 0,0,0, 0,0,0}, BLOCK_SLAB_WOOD, 6},
    {{BLOCK_SANDSTONE,BLOCK_SANDSTONE,BLOCK_SANDSTONE, 0,0,0, 0,0,0}, BLOCK_SLAB_SANDSTONE, 6},
    {{BLOCK_BRICKS,BLOCK_BRICKS,BLOCK_BRICKS, 0,0,0, 0,0,0}, BLOCK_SLAB_BRICK, 6},

    // Stairs (staircase pattern = 4 stairs)
    {{BLOCK_STONE,0,0, BLOCK_STONE,BLOCK_STONE,0, BLOCK_STONE,BLOCK_STONE,BLOCK_STONE}, BLOCK_STAIRS_STONE, 4},
    {{BLOCK_PLANKS,0,0, BLOCK_PLANKS,BLOCK_PLANKS,0, BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS}, BLOCK_STAIRS_WOOD, 4},

    // Carpets (2 wool in a row = 3 carpets)
    {{0,0,0, BLOCK_WOOL_WHITE,BLOCK_WOOL_WHITE,0, 0,0,0}, BLOCK_CARPET_WHITE, 3},
    {{0,0,0, BLOCK_WOOL_RED,BLOCK_WOOL_RED,0, 0,0,0}, BLOCK_CARPET_RED, 3},
    {{0,0,0, BLOCK_WOOL_BLUE,BLOCK_WOOL_BLUE,0, 0,0,0}, BLOCK_CARPET_BLUE, 3},

    // Glass pane (6 glass in 2 rows = 6 panes)
    {{BLOCK_GLASS,BLOCK_GLASS,BLOCK_GLASS, BLOCK_GLASS,BLOCK_GLASS,BLOCK_GLASS, 0,0,0}, BLOCK_GLASS_PANE, 6},

    // Iron bars (6 iron blocks in 2 rows = 6 bars)
    {{BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK, BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK, 0,0,0}, BLOCK_IRON_BARS, 6},

    // Fence (planks + cobble pattern)
    {{BLOCK_PLANKS,BLOCK_COBBLESTONE,BLOCK_PLANKS, BLOCK_PLANKS,BLOCK_COBBLESTONE,BLOCK_PLANKS, 0,0,0}, BLOCK_FENCE_WOOD, 3},

    // Fence gate
    {{0,0,0, BLOCK_COBBLESTONE,BLOCK_PLANKS,BLOCK_COBBLESTONE, BLOCK_COBBLESTONE,BLOCK_PLANKS,BLOCK_COBBLESTONE}, BLOCK_FENCE_GATE, 1},

    // Oak door (6 planks 2x3)
    {{BLOCK_PLANKS,BLOCK_PLANKS,0, BLOCK_PLANKS,BLOCK_PLANKS,0, BLOCK_PLANKS,BLOCK_PLANKS,0}, BLOCK_DOOR_OAK, 1},

    // Iron door (6 iron blocks 2x3)
    {{BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK,0, BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK,0, BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK,0}, BLOCK_DOOR_IRON, 1},

    // Trapdoor (6 planks 3x2 bottom)
    {{0,0,0, BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS, BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS}, BLOCK_TRAPDOOR_OAK, 2},

    // Ladder (H-shape planks)
    {{BLOCK_PLANKS,0,BLOCK_PLANKS, BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS, BLOCK_PLANKS,0,BLOCK_PLANKS}, BLOCK_LADDER, 3},

    // Bookshelf (planks + glowstone as books proxy)
    {{BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS, BLOCK_GLOWSTONE,BLOCK_GLOWSTONE,BLOCK_GLOWSTONE, BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS}, BLOCK_BOOKSHELF, 1},

    // Lantern (glowstone center + glass)
    {{0,BLOCK_GLOWSTONE,0, BLOCK_GLOWSTONE,BLOCK_GLASS,BLOCK_GLOWSTONE, 0,BLOCK_GLOWSTONE,0}, BLOCK_LANTERN, 1},

    // Torch block (wood + stone)
    {{0,BLOCK_WOOD,0, 0,BLOCK_STONE,0, 0,0,0}, BLOCK_TORCH_BLOCK, 4},

    // Sign (planks top + wood stick bottom)
    {{BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS, BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS, 0,BLOCK_WOOD,0}, BLOCK_SIGN, 3},

    // Smooth stone (2 stone light)
    {{0,0,0, BLOCK_STONE_LIGHT,BLOCK_STONE_LIGHT,0, 0,0,0}, BLOCK_SMOOTH_STONE, 2},

    // Terracotta (4 clay)
    {{0,0,0, BLOCK_CLAY,BLOCK_CLAY,0, BLOCK_CLAY,BLOCK_CLAY,0}, BLOCK_TERRACOTTA, 4},

    // Wool color variants (white wool + dye proxy)
    {{0,0,0, BLOCK_WOOL_WHITE,BLOCK_LEAF,0, 0,0,0}, BLOCK_WOOL_GREEN, 1},
    {{0,0,0, BLOCK_WOOL_WHITE,BLOCK_SAND,0, 0,0,0}, BLOCK_WOOL_YELLOW, 1},
    {{0,0,0, BLOCK_WOOL_WHITE,BLOCK_COAL_ORE,0, 0,0,0}, BLOCK_WOOL_BLACK, 1},
    {{0,0,0, BLOCK_WOOL_WHITE,BLOCK_ORE_GOLD,0, 0,0,0}, BLOCK_WOOL_ORANGE, 1},
    {{0,0,0, BLOCK_WOOL_WHITE,BLOCK_BRICKS,0, 0,0,0}, BLOCK_WOOL_PINK, 1},
    {{0,0,0, BLOCK_WOOL_BLUE,BLOCK_WOOL_RED,0, 0,0,0}, BLOCK_WOOL_PURPLE, 1},
    {{0,0,0, BLOCK_WOOL_BLUE,BLOCK_LEAF,0, 0,0,0}, BLOCK_WOOL_CYAN, 1},
    {{0,0,0, BLOCK_WOOL_RED,BLOCK_DIRT,0, 0,0,0}, BLOCK_WOOL_BROWN, 1},
    {{0,0,0, BLOCK_WOOL_WHITE,BLOCK_STONE,0, 0,0,0}, BLOCK_WOOL_GRAY, 1},
    {{0,0,0, BLOCK_WOOL_GRAY,BLOCK_WOOL_WHITE,0, 0,0,0}, BLOCK_WOOL_LIGHT_GRAY, 1},
    {{0,0,0, BLOCK_WOOL_PINK,BLOCK_WOOL_PURPLE,0, 0,0,0}, BLOCK_WOOL_MAGENTA, 1},
    {{0,0,0, BLOCK_WOOL_GREEN,BLOCK_WOOL_WHITE,0, 0,0,0}, BLOCK_WOOL_LIME, 1},

    // Banner (wool top 2 rows + wood stick)
    {{BLOCK_WOOL_WHITE,BLOCK_WOOL_WHITE,BLOCK_WOOL_WHITE, BLOCK_WOOL_WHITE,BLOCK_WOOL_WHITE,BLOCK_WOOL_WHITE, 0,BLOCK_WOOD,0}, BLOCK_BANNER, 1},

    // ---- TOOL RECIPES ----
    // S = ITEM_STICK, material tiers: planks / stone / iron / diamond

    // Stick (2 planks vertical = 4 sticks)
    {{0,BLOCK_PLANKS,0, 0,BLOCK_PLANKS,0, 0,0,0}, ITEM_STICK, 4},

    // --- Wood tools ---
    // Sword: 2 planks + 1 stick (col)
    {{0,BLOCK_PLANKS,0, 0,BLOCK_PLANKS,0, 0,ITEM_STICK,0}, ITEM_SWORD_WOOD, 1},
    // Axe: 2x2 planks top-left + 2 sticks
    {{BLOCK_PLANKS,BLOCK_PLANKS,0, BLOCK_PLANKS,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_AXE_WOOD, 1},
    // Pickaxe: 3 planks top + 2 sticks
    {{BLOCK_PLANKS,BLOCK_PLANKS,BLOCK_PLANKS, 0,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_PICKAXE_WOOD, 1},
    // Shovel: 1 plank + 2 sticks
    {{0,BLOCK_PLANKS,0, 0,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_SHOVEL_WOOD, 1},

    // --- Stone tools ---
    {{0,BLOCK_COBBLESTONE,0, 0,BLOCK_COBBLESTONE,0, 0,ITEM_STICK,0}, ITEM_SWORD_STONE, 1},
    {{BLOCK_COBBLESTONE,BLOCK_COBBLESTONE,0, BLOCK_COBBLESTONE,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_AXE_STONE, 1},
    {{BLOCK_COBBLESTONE,BLOCK_COBBLESTONE,BLOCK_COBBLESTONE, 0,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_PICKAXE_STONE, 1},
    {{0,BLOCK_COBBLESTONE,0, 0,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_SHOVEL_STONE, 1},

    // --- Iron tools ---
    {{0,BLOCK_IRON_BLOCK,0, 0,BLOCK_IRON_BLOCK,0, 0,ITEM_STICK,0}, ITEM_SWORD_IRON, 1},
    {{BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK,0, BLOCK_IRON_BLOCK,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_AXE_IRON, 1},
    {{BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK,BLOCK_IRON_BLOCK, 0,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_PICKAXE_IRON, 1},
    {{0,BLOCK_IRON_BLOCK,0, 0,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_SHOVEL_IRON, 1},

    // --- Diamond tools ---
    {{0,BLOCK_ORE_DIAMOND,0, 0,BLOCK_ORE_DIAMOND,0, 0,ITEM_STICK,0}, ITEM_SWORD_DIAMOND, 1},
    {{BLOCK_ORE_DIAMOND,BLOCK_ORE_DIAMOND,0, BLOCK_ORE_DIAMOND,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_AXE_DIAMOND, 1},
    {{BLOCK_ORE_DIAMOND,BLOCK_ORE_DIAMOND,BLOCK_ORE_DIAMOND, 0,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_PICKAXE_DIAMOND, 1},
    {{0,BLOCK_ORE_DIAMOND,0, 0,ITEM_STICK,0, 0,ITEM_STICK,0}, ITEM_SHOVEL_DIAMOND, 1},

    // --- Bow (3 sticks + string proxy: wool) ---
    {{0,ITEM_STICK,BLOCK_WOOL_WHITE, ITEM_STICK,0,BLOCK_WOOL_WHITE, 0,ITEM_STICK,BLOCK_WOOL_WHITE}, ITEM_BOW, 1}
};
const int NUM_RECIPES = sizeof(recipes) / sizeof(recipes[0]);

// Get bounding box of non-empty cells in a 3x3 grid
void gridBounds(const int grid[9], int& minR, int& maxR, int& minC, int& maxC) {
    minR = 3; maxR = -1; minC = 3; maxC = -1;
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            if (grid[r * 3 + c] != BLOCK_AIR) {
                if (r < minR) minR = r;
                if (r > maxR) maxR = r;
                if (c < minC) minC = c;
                if (c > maxC) maxC = c;
            }
        }
    }
}

void checkCraftingRecipes() {
    // Get bounding box of what's in the crafting grid
    int gridTypes[9];
    for (int i = 0; i < 9; i++) gridTypes[i] = craftingGrid[i].type;
    int gMinR, gMaxR, gMinC, gMaxC;
    gridBounds(gridTypes, gMinR, gMaxR, gMinC, gMaxC);

    // Empty grid — no output
    if (gMaxR < 0) {
        craftingOutput.type = BLOCK_AIR;
        craftingOutput.count = 0;
        return;
    }
    int gH = gMaxR - gMinR + 1;
    int gW = gMaxC - gMinC + 1;

    for (int r = 0; r < NUM_RECIPES; r++) {
        // Get recipe bounding box
        int rMinR, rMaxR, rMinC, rMaxC;
        gridBounds((int*)recipes[r].pattern, rMinR, rMaxR, rMinC, rMaxC);
        if (rMaxR < 0) continue; // empty recipe
        int rH = rMaxR - rMinR + 1;
        int rW = rMaxC - rMinC + 1;

        // Dimensions must match
        if (rH != gH || rW != gW) continue;

        // Compare normalized patterns
        bool match = true;
        for (int dr = 0; dr < rH && match; dr++) {
            for (int dc = 0; dc < rW && match; dc++) {
                int need = recipes[r].pattern[(rMinR + dr) * 3 + (rMinC + dc)];
                int have = craftingGrid[(gMinR + dr) * 3 + (gMinC + dc)].type;
                int cnt  = craftingGrid[(gMinR + dr) * 3 + (gMinC + dc)].count;
                if (need != have || (need != BLOCK_AIR && cnt < 1)) match = false;
            }
        }
        if (match) {
            craftingOutput.type = recipes[r].resultType;
            craftingOutput.count = recipes[r].resultCount;
            craftingOutput.durability = getMaxDurability(recipes[r].resultType);
            return;
        }
    }
    // No recipe matched
    craftingOutput.type = BLOCK_AIR;
    craftingOutput.count = 0;
    craftingOutput.durability = -1;
}

void consumeCraftingInputs() {
    // Remove one item from each non-empty crafting slot
    for (int i = 0; i < 9; i++) {
        if (craftingGrid[i].type != BLOCK_AIR) {
            craftingGrid[i].count--;
            if (craftingGrid[i].count <= 0) {
                craftingGrid[i].type = BLOCK_AIR;
                craftingGrid[i].count = 0;
            }
        }
    }
    checkCraftingRecipes(); // re-check after consuming
}

// Mouse Drag State
InventorySlot draggedSlot;

bool inventoryOpen = false;
bool useCraftingTable = false; // true = 3x3 (crafting table), false = 2x2 (personal)
bool recipeBookOpen = false;
int recipePage = 0;
std::string recipeSearchText = "";

bool controlCar = false;
glm::vec3 ambPos(0.0f);
float ambYaw = 0.0f;
float ambSpeed = 0.0f;
bool ambInitialized = false;
GLuint texAmbu = 0; // ambulance side decal texture

const char* getBlockName(int type) {
    switch (type) {
        case BLOCK_AIR:              return "air";
        case BLOCK_GRASS:            return "grass block";
        case BLOCK_DIRT:             return "dirt";
        case BLOCK_SAND:             return "sand";
        case BLOCK_STONE:            return "stone";
        case BLOCK_WATER:            return "water";
        case BLOCK_WOOD:             return "oak log";
        case BLOCK_LEAF:             return "oak leaves";
        case BLOCK_ORE_DIAMOND:      return "diamond ore";
        case BLOCK_ORE_GOLD:         return "gold ore";
        case BLOCK_GLASS:            return "glass";
        case BLOCK_STONE_LIGHT:      return "stone bricks";
        case BLOCK_BEDROCK:          return "bedrock";
        case BLOCK_COAL_ORE:         return "coal ore";
        case BLOCK_GRAVEL:           return "gravel";
        case BLOCK_CLAY:             return "clay";
        case BLOCK_SNOW:             return "snow";
        case BLOCK_ICE:              return "ice";
        case BLOCK_PLANKS:           return "oak planks";
        case BLOCK_CRAFTING_TABLE:   return "crafting table";
        case BLOCK_SANDSTONE:        return "sandstone";
        case BLOCK_BRICKS:           return "bricks";
        case BLOCK_WOOL_WHITE:       return "white wool";
        case BLOCK_WOOL_RED:         return "red wool";
        case BLOCK_WOOL_BLUE:        return "blue wool";
        case BLOCK_COBBLESTONE:      return "cobblestone";
        case BLOCK_MOSSY_COBBLESTONE:return "mossy cobblestone";
        case BLOCK_GLOWSTONE:        return "glowstone";
        case BLOCK_POLISHED_DIORITE: return "polished diorite";
        case BLOCK_POLISHED_GRANITE: return "polished granite";
        case BLOCK_POLISHED_ANDESITE:return "polished andesite";
        case BLOCK_QUARTZ_BLOCK:     return "quartz block";
        case BLOCK_IRON_BLOCK:       return "iron block";
        case BLOCK_DIAMOND_BLOCK:    return "diamond block";
        case BLOCK_GOLD_BLOCK:       return "gold block";
        case BLOCK_CUT_SANDSTONE:    return "cut sandstone";
        case BLOCK_MOSSY_BRICKS:     return "mossy bricks";
        case BLOCK_OBSIDIAN:         return "obsidian";
        case BLOCK_WOOL_GREEN:       return "green wool";
        case BLOCK_WOOL_YELLOW:      return "yellow wool";
        case BLOCK_WOOL_BLACK:       return "black wool";
        case BLOCK_WOOL_ORANGE:      return "orange wool";
        case BLOCK_WOOL_PINK:        return "pink wool";
        case BLOCK_WOOL_PURPLE:      return "purple wool";
        case BLOCK_WOOL_CYAN:        return "cyan wool";
        case BLOCK_WOOL_BROWN:       return "brown wool";
        case BLOCK_WOOL_GRAY:        return "gray wool";
        case BLOCK_WOOL_LIGHT_GRAY:  return "light gray wool";
        case BLOCK_WOOL_MAGENTA:     return "magenta wool";
        case BLOCK_WOOL_LIME:        return "lime wool";
        case BLOCK_BOOKSHELF:        return "bookshelf";
        case BLOCK_SMOOTH_STONE:     return "smooth stone";
        case BLOCK_TERRACOTTA:       return "terracotta";
        case BLOCK_SLAB_STONE:       return "stone slab";
        case BLOCK_SLAB_WOOD:        return "wood slab";
        case BLOCK_SLAB_SANDSTONE:   return "sandstone slab";
        case BLOCK_SLAB_BRICK:       return "brick slab";
        case BLOCK_STAIRS_STONE:     return "stone stairs";
        case BLOCK_STAIRS_WOOD:      return "wood stairs";
        case BLOCK_CARPET_WHITE:     return "white carpet";
        case BLOCK_CARPET_RED:       return "red carpet";
        case BLOCK_CARPET_BLUE:      return "blue carpet";
        case BLOCK_GLASS_PANE:       return "glass pane";
        case BLOCK_IRON_BARS:        return "iron bars";
        case BLOCK_FENCE_WOOD:       return "wood fence";
        case BLOCK_DOOR_OAK:         return "oak door";
        case BLOCK_DOOR_IRON:        return "iron door";
        case BLOCK_TRAPDOOR_OAK:     return "trapdoor";
        case BLOCK_FENCE_GATE:       return "fence gate";
        case BLOCK_LADDER:           return "ladder";
        case BLOCK_LANTERN:          return "lantern";
        case BLOCK_TORCH_BLOCK:      return "torch";
        case BLOCK_SIGN:             return "sign";
        case BLOCK_BANNER:           return "banner";
        // Tools
        case ITEM_STICK:             return "stick";
        case ITEM_SWORD_WOOD:        return "wooden sword";
        case ITEM_AXE_WOOD:          return "wooden axe";
        case ITEM_PICKAXE_WOOD:      return "wooden pickaxe";
        case ITEM_SHOVEL_WOOD:       return "wooden shovel";
        case ITEM_SWORD_STONE:       return "stone sword";
        case ITEM_AXE_STONE:         return "stone axe";
        case ITEM_PICKAXE_STONE:     return "stone pickaxe";
        case ITEM_SHOVEL_STONE:      return "stone shovel";
        case ITEM_SWORD_IRON:        return "iron sword";
        case ITEM_AXE_IRON:          return "iron axe";
        case ITEM_PICKAXE_IRON:      return "iron pickaxe";
        case ITEM_SHOVEL_IRON:       return "iron shovel";
        case ITEM_SWORD_DIAMOND:     return "diamond sword";
        case ITEM_AXE_DIAMOND:       return "diamond axe";
        case ITEM_PICKAXE_DIAMOND:   return "diamond pickaxe";
        case ITEM_SHOVEL_DIAMOND:    return "diamond shovel";
        case ITEM_BOW:               return "bow";
        case ITEM_TOOL_HILL:         return "hill tool";
        case ITEM_TOOL_POND:         return "pond tool";
        default:                     return "unknown";
    }
}

// Tool damage values (Minecraft-accurate)
float getToolDamage(int type) {
    switch (type) {
        // Swords
        case ITEM_SWORD_WOOD:    return 4.0f;
        case ITEM_SWORD_STONE:   return 5.0f;
        case ITEM_SWORD_IRON:    return 6.0f;
        case ITEM_SWORD_DIAMOND: return 7.0f;
        // Axes (high damage, slow)
        case ITEM_AXE_WOOD:      return 7.0f;
        case ITEM_AXE_STONE:     return 9.0f;
        case ITEM_AXE_IRON:      return 9.0f;
        case ITEM_AXE_DIAMOND:   return 9.0f;
        // Pickaxes
        case ITEM_PICKAXE_WOOD:    return 2.0f;
        case ITEM_PICKAXE_STONE:   return 3.0f;
        case ITEM_PICKAXE_IRON:    return 4.0f;
        case ITEM_PICKAXE_DIAMOND: return 5.0f;
        // Shovels
        case ITEM_SHOVEL_WOOD:    return 2.5f;
        case ITEM_SHOVEL_STONE:   return 3.5f;
        case ITEM_SHOVEL_IRON:    return 4.5f;
        case ITEM_SHOVEL_DIAMOND: return 5.5f;
        default: return 1.0f; // fist / non-tool
    }
}

// Check if a tool type is a sword
bool isSword(int type) {
    return type == ITEM_SWORD_WOOD || type == ITEM_SWORD_STONE ||
           type == ITEM_SWORD_IRON || type == ITEM_SWORD_DIAMOND;
}

// Check if a tool type is an axe
bool isAxe(int type) {
    return type == ITEM_AXE_WOOD || type == ITEM_AXE_STONE ||
           type == ITEM_AXE_IRON || type == ITEM_AXE_DIAMOND;
}

// Check if a tool type is a pickaxe
bool isPickaxe(int type) {
    return type == ITEM_PICKAXE_WOOD || type == ITEM_PICKAXE_STONE ||
           type == ITEM_PICKAXE_IRON || type == ITEM_PICKAXE_DIAMOND;
}

// Check if a tool type is a shovel
bool isShovel(int type) {
    return type == ITEM_SHOVEL_WOOD || type == ITEM_SHOVEL_STONE ||
           type == ITEM_SHOVEL_IRON || type == ITEM_SHOVEL_DIAMOND;
}

// Check if a block is wood-type (axes are effective)
bool isWoodBlock(int bt) {
    return bt == BLOCK_WOOD || bt == BLOCK_PLANKS || bt == BLOCK_CRAFTING_TABLE ||
           bt == BLOCK_FENCE_WOOD || bt == BLOCK_FENCE_GATE || bt == BLOCK_DOOR_OAK ||
           bt == BLOCK_TRAPDOOR_OAK || bt == BLOCK_LADDER || bt == BLOCK_SIGN ||
           bt == BLOCK_BANNER || bt == BLOCK_BOOKSHELF;
}

// Check if a block is stone-type (pickaxes are effective)
bool isStoneBlock(int bt) {
    return bt == BLOCK_STONE || bt == BLOCK_COBBLESTONE || bt == BLOCK_STONE_LIGHT ||
           bt == BLOCK_BRICKS || bt == BLOCK_SANDSTONE || bt == BLOCK_CUT_SANDSTONE ||
           bt == BLOCK_MOSSY_COBBLESTONE || bt == BLOCK_MOSSY_BRICKS ||
           bt == BLOCK_POLISHED_DIORITE || bt == BLOCK_POLISHED_GRANITE ||
           bt == BLOCK_POLISHED_ANDESITE || bt == BLOCK_QUARTZ_BLOCK ||
           bt == BLOCK_IRON_BLOCK || bt == BLOCK_DIAMOND_BLOCK || bt == BLOCK_GOLD_BLOCK ||
           bt == BLOCK_OBSIDIAN || bt == BLOCK_ORE_DIAMOND || bt == BLOCK_ORE_GOLD ||
           bt == BLOCK_COAL_ORE || bt == BLOCK_GRAVEL || bt == BLOCK_IRON_BARS;
}

// Check if a block is soil-type (shovels are effective)
bool isSoilBlock(int bt) {
    return bt == BLOCK_DIRT || bt == BLOCK_GRASS || bt == BLOCK_SAND ||
           bt == BLOCK_GRAVEL || bt == BLOCK_CLAY || bt == BLOCK_SNOW;
}

// Can the held tool break this block? (e.g., need pickaxe for ores)
bool canBreakWith(int toolType, int blockType) {
    // Anything can be broken with anything (Minecraft lets you, just slower)
    // But certain blocks require at least a pickaxe to drop items
    return true;
}

// Add crafted items directly to player inventory (no grid needed)
bool addToInventory(int type, int count) {
    // Try to stack onto existing slot first
    for(int j=0; j<36; j++) {
        if(playerInventory[j].type == type && playerInventory[j].count + count <= 64) {
            playerInventory[j].count += count;
            return true;
        }
    }
    // Then find an empty slot
    for(int j=0; j<36; j++) {
        if(playerInventory[j].type == BLOCK_AIR) {
            playerInventory[j].type = type;
            playerInventory[j].count = count;
            return true;
        }
    }
    return false; // inventory full
}

// =====================================================
// Whole-inventory tally / spend (plan_2 Step 4)
// =====================================================
// The Build tab spends straight out of the inventory instead of arranging a
// pattern in the crafting grid, so it needs these. Slots 0-35 covers the hotbar
// as well as storage, which is deliberate: a stack held in hand is still the
// player's, and requiring them to shuffle it into storage first would be a
// pointless step.
//
// autoCraftIngredient() below has a local lambda of the same name that predates
// this; it shadows this one inside that function, which is harmless — both do
// the same thing.
int countInInventory(int type) {
    int n = 0;
    for (int i = 0; i < 36; i++)
        if (playerInventory[i].type == type) n += playerInventory[i].count;
    return n;
}

// Removes up to `count` and returns how many were actually taken. There is no
// rollback on a partial spend, so callers must check countInInventory() first —
// which is why craftStructure() tests affordability for *both* ingredients
// before removing either.
int removeFromInventory(int type, int count) {
    int taken = 0;
    for (int i = 0; i < 36 && taken < count; i++) {
        if (playerInventory[i].type != type) continue;
        int want = count - taken;
        int take = (playerInventory[i].count < want) ? playerInventory[i].count : want;
        playerInventory[i].count -= take;
        taken += take;
        if (playerInventory[i].count <= 0) {
            playerInventory[i].type  = BLOCK_AIR;
            playerInventory[i].count = 0;
        }
    }
    return taken;
}

// Auto-craft an intermediate ingredient directly into inventory (no grid).
// Returns true if the player ends up with at least `needed` of `itemType`.
bool autoCraftIngredient(int itemType, int needed) {
    auto countInInventory = [&](int t) {
        int n = 0;
        for(int i=0; i<36; i++) if(playerInventory[i].type == t) n += playerInventory[i].count;
        return n;
    };

    if(countInInventory(itemType) >= needed) return true;

    // Find a recipe that produces itemType
    for(int r=0; r<NUM_RECIPES; r++) {
        if(recipes[r].resultType != itemType) continue;

        while(countInInventory(itemType) < needed) {
            // Check if we have all ingredients for this recipe
            int req[256] = {0};
            for(int i=0; i<9; i++) {
                int t = recipes[r].pattern[i];
                if(t != BLOCK_AIR) req[t]++;
            }
            bool canCraft = true;
            for(int i=0; i<256; i++) {
                if(req[i] == 0) continue;
                if(countInInventory(i) < req[i]) { canCraft = false; break; }
            }
            if(!canCraft) break;

            // Consume ingredients
            for(int i=0; i<9; i++) {
                int t = recipes[r].pattern[i];
                if(t == BLOCK_AIR) continue;
                for(int j=0; j<36; j++) {
                    if(playerInventory[j].type == t && playerInventory[j].count > 0) {
                        playerInventory[j].count--;
                        if(playerInventory[j].count == 0) playerInventory[j].type = BLOCK_AIR;
                        break;
                    }
                }
            }
            addToInventory(recipes[r].resultType, recipes[r].resultCount);
            printf("[Craft] Auto-crafted intermediate: %s\n", getBlockName(recipes[r].resultType));
        }
        return countInInventory(itemType) >= needed;
    }
    return false;
}

void autoCraftRecipe(int recipeIndex) {
    if (recipeIndex < 0 || recipeIndex >= NUM_RECIPES) return;

    // We only auto-fill if the crafting grid is completely empty
    for(int i=0; i<9; i++) {
        if(craftingGrid[i].type != BLOCK_AIR) return;
    }

    // Count required ingredients
    int required[256] = {0};
    for(int i=0; i<9; i++) {
        int t = recipes[recipeIndex].pattern[i];
        if(t != BLOCK_AIR) required[t]++;
    }

    // Auto-craft any missing intermediate ingredients (e.g. sticks from planks)
    for(int i=1; i<256; i++) {
        if(required[i] > 0) autoCraftIngredient(i, required[i]);
    }

    // Now check if we have everything
    int available[256] = {0};
    for(int i=0; i<36; i++) {
        int t = playerInventory[i].type;
        if(t != BLOCK_AIR) available[t] += playerInventory[i].count;
    }
    for(int i=0; i<256; i++) {
        if(required[i] > available[i]) {
            printf("[Craft] Cannot auto-craft recipe %d: missing %s\n",
                   recipeIndex, getBlockName(i));
            return;
        }
    }

    // Take from inventory and place exactly 1 of each required item in grid
    for(int i=0; i<9; i++) {
        int t = recipes[recipeIndex].pattern[i];
        if(t != BLOCK_AIR) {
            craftingGrid[i].type = t;
            craftingGrid[i].count = 1;
            for(int j=0; j<36; j++) {
                if(playerInventory[j].type == t && playerInventory[j].count > 0) {
                    playerInventory[j].count--;
                    if(playerInventory[j].count == 0) playerInventory[j].type = BLOCK_AIR;
                    break;
                }
            }
        }
    }
    checkCraftingRecipes();
    printf("[Craft] Auto-filled recipe %d\n", recipeIndex);
}

int hotbarSlot = 0; // 0-8
const int HOTBAR_SIZE = 9;

// Block targeting (raycasting)
bool hasTarget = false;
int targetCol, targetRow, targetHeight;
int placeCol, placeRow, placeHeight; // adjacent cell for placement

// Grab / carry (src/input.h) — K lifts the targeted block clean out of the grid
// and carries it in front of the camera. Deliberately NOT the inventory path:
// breaking a block launders it through a hotbar slot, so a mossy cobblestone
// comes back as generic cobblestone and a door forgets which way it faced.
// Carrying preserves both, because the type and the state bits travel with it.
// The origin cell is kept so the block can go back where it came from if the
// player dies still holding it — otherwise dying would delete it outright.
int      heldBlockType  = BLOCK_AIR;   // BLOCK_AIR means carrying nothing
uint16_t heldBlockState = 0;   // facing / open bits, restored on set-down
int      heldOriginCol = 0, heldOriginRow = 0, heldOriginH = 0;

// Block breaking state (hold-to-break)
bool isBreaking = false;
float breakHoldTime = 0.0f;
int breakTargetCol = 0, breakTargetRow = 0, breakTargetH = 0;

// Break particles — chips flying out while mining
struct BreakParticle {
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec3 color;
    float lifetime;     // age so far
    float maxLifetime;  // total duration
    float spin;         // current rotation (radians)
    float spinSpeed;    // radians/sec
};
std::vector<BreakParticle> breakParticles;
const int MAX_BREAK_PARTICLES = 120;
float breakParticleTimer = 0.0f; // throttle emission

// Item drops
struct ItemDrop {
    glm::vec3 pos;
    glm::vec3 vel;      // for initial bounce
    BlockType type;
    float lifetime;      // despawn after 60s
    float bobPhase;      // for bobbing animation
};
std::vector<ItemDrop> itemDrops;
const float ITEM_PICKUP_RADIUS = 1.5f;
const float ITEM_DESPAWN_TIME = 60.0f;


// =====================================================
// Mobs
// =====================================================
enum MobType { MOB_CHICKEN = 0, MOB_PIG, MOB_SHEEP, MOB_ZOMBIE, MOB_SKELETON };
enum MobState { MOB_IDLE, MOB_WANDER, MOB_CHASE, MOB_ATTACK, MOB_FLEE };

struct Mob {
    glm::vec3 pos;
    float yaw;           // facing direction
    MobType type;
    MobState state;
    float health;
    float maxHealth;
    float stateTimer;    // time in current state
    float attackCooldown; // for hostile mobs
    float walkTime;      // for walk animation
    bool alive;
    float hitFlash;      // red tint timer (0.3s on hit, counts down)
    float deathTimer;    // death animation timer (falls over + fades, -1 = alive)
    // Pathfinding
    std::vector<glm::vec3> path; // A* computed waypoints
    int pathIndex;               // current waypoint index
    float pathTimer;             // time since last path recalc
};
std::vector<Mob> mobs;
const int MAX_MOBS = 40;
float mobSpawnTimer = 0.0f;

// Arrow projectiles (shot by skeletons)
struct Arrow {
    glm::vec3 pos;
    glm::vec3 vel;
    float lifetime;
    bool active;
};
std::vector<Arrow> arrows;

// Player attack cooldown
float playerAttackCooldown = 0.0f;

// =====================================================
// Camera state
// =====================================================
glm::vec3 camPos(4.0f, 10.0f, 16.0f);
glm::vec3 camFront(0.0f, -0.4f, -1.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);
float camSpeed = 5.0f;
float camPitch = -15.0f, camYaw = -90.0f, camRoll = 0.0f;
float mouseSensitivity = 0.15f;
bool firstMouse = true;
double lastMouseX = 640.0, lastMouseY = 360.0;
bool mouseCaptured = false; // right-click to toggle

// =====================================================
// Player state — actual game character
// =====================================================
glm::vec3 playerWorldPos(0.0f, 5.0f, 0.0f); // world position — will snap to ground on first frame
float playerYaw = 0.0f;       // horizontal facing direction (radians)
float playerVelY = 0.0f;      // vertical velocity (for jump/gravity)
bool playerOnGround = true;
bool playerOnLadder = false;
bool playerWalking = false;
float playerWalkTime = 0.0f;  // for walk animation cycle
bool playerSprinting = false;

// Health, stamina, hunger
float playerHealth = 20.0f;   // max 20 (10 hearts)
float playerMaxHealth = 20.0f;
float playerStamina = 100.0f; // max 100
float playerMaxStamina = 100.0f;
float playerHunger = 20.0f;   // max 20 (10 drumsticks)
float playerMaxHunger = 20.0f;
float hungerTimer = 0.0f;     // accumulates time for hunger depletion
bool playerDead = false;
float fallStartY = 0.0f;      // Y when player left ground (for fall damage)
bool trackingFall = false;

// Camera mode: 0=third-person, 1=first-person, 2=free-fly
int cameraMode = 0;
float thirdPersonDist = 3.5f;  // distance behind player (Minecraft-like)
float thirdPersonHeight = 0.8f; // height offset above eye level

// Timing
float deltaTime = 0.0f, lastFrame = 0.0f;

// Weather (src/weather.h) — declared here because input.h owns the N key and is
// included before weather.h.
bool rainOn = false;          // N toggle

// View modes
bool fourViewport = false;    // V toggle
bool birdsEye = false;        // B toggle
float orbitAngle = 0.0f;      // F key orbit around look-at point
glm::vec3 lookAtTarget(3.0f, 3.0f, 8.0f); // castle entrance area

// Lighting state
bool lightOn = true;
bool dirLightOn = true;
bool pointLightOn = true;
bool spotLightOn = true;
bool ambientOn = true;
bool diffuseOn = true;
bool specularOn = true;

// Day-night: 0=night, 1=dawn, 2=noon, 3=dusk
int dayMode = 2;
float dayFactor = 1.0f;

// Terrain render radius in world units. Trees, torches and fog all key off this:
// fog density is derived from it in main.cpp so that geometry is fully fogged out
// by the time it reaches the cutoff, instead of vanishing mid-air. Raising it
// costs frame time roughly with the square — see the 19D notes in plan.md.
const float RENDER_DIST = 50.0f;
const float RENDER_DIST_SQ = RENDER_DIST * RENDER_DIST;

// The fog density actually uploaded this frame. renderSky() needs it so it can
// switch fog off for the sun/moon/stars and put it back afterwards: those sit at
// skyDist = 150, far beyond the fog cutoff, so distance fog erases them outright.
float currentFogDensity = 0.0f;

// Interactive objects
bool fanOn = false;          // G toggle
float fanAngle = 0.0f;
bool doorOpen = false;       // O toggle
float doorAngle = 0.0f;     // animates 0 to 90
bool windowOpen = false;     // P toggle (when not dead)
float windowAngle = 0.0f;   // animates 0 to 90

// MineCar state (arrow keys to drive)
glm::vec3 carPos(-4.0f, 0.0f, 5.0f); // parked near castle entrance
float carYaw = 0.0f;        // heading in radians
float carSpeed = 0.0f;
float wheelSpin = 0.0f;
// Front-wheel angle, radians, + = left. Steering is STATE, not an instantaneous
// input: it winds toward full lock while a key is held and self-centres when
// released. That is what lets the front wheels be drawn turned, and it is the
// input to the bicycle model in processInput.
float carSteer = 0.0f;
// Distance between the axles, taken from drawCar's own wheel positions
// (wx = +/-0.6 along the car's forward axis). Derived rather than copied from
// gfx_b3's 1.4: the steering geometry has to match the model actually drawn, or
// the wheels point somewhere the car does not go.
const float CAR_WHEELBASE  = 1.2f;
const float CAR_MAX_STEER  = 0.61f;   // 35 degrees, as theirs
const float CAR_STEER_RATE = 1.57f;   // 90 deg/s to full lock

// Flying birds
struct Bird {
    glm::vec3 pos;
    glm::vec3 waypoints[6]; // spline control points
    int currentWP;           // current waypoint index
    float t;                 // parameter along current spline segment
    float wingPhase;         // wing flap phase
    float speed;
    glm::vec3 color;
    float yaw;               // heading, derived from motion along the spline
};
std::vector<Bird> birds;

// Gouraud/Phong toggle
bool useGouraud = false;     // H toggle

// Shader
GLuint shaderProgram;

// Textures — local
GLuint texBrick = 0, texGrass = 0, texWood = 0;
// Textures — FM Default pack
GLuint texStoneFM = 0, texCobblestone = 0, texMossyCobble = 0;
GLuint texStoneBricks = 0, texMossyStoneBricks = 0, texCrackedStoneBricks = 0;
GLuint texOakPlanks = 0, texBookshelf = 0;
GLuint texIronBars = 0, texGlassFM = 0, texGlassPaneTop = 0;
GLuint texLadder = 0;
GLuint texIronDoorBot = 0, texIronDoorTop = 0;
GLuint texItemOakDoor = 0, texItemIronDoor = 0;  // item sprites for inventory icons

// Tool item sprites (FM Default pack)
GLuint texItemStick = 0;
GLuint texItemSwordWood = 0, texItemAxeWood = 0, texItemPickaxeWood = 0, texItemShovelWood = 0;
GLuint texItemSwordStone = 0, texItemAxeStone = 0, texItemPickaxeStone = 0, texItemShovelStone = 0;
GLuint texItemSwordIron = 0, texItemAxeIron = 0, texItemPickaxeIron = 0, texItemShovelIron = 0;
GLuint texItemSwordDiamond = 0, texItemAxeDiamond = 0, texItemPickaxeDiamond = 0, texItemShovelDiamond = 0;
GLuint texItemBow = 0;
GLuint texTorchBlock = 0, texSeaLantern = 0;
GLuint texCraftingTop = 0, texCraftingFront = 0;
GLuint texBedrock = 0, texCoalOre = 0, texDiamondOre = 0, texGoldOre = 0;

// Textures — Gold resource pack (blocks missing from FM Default)
GLuint texGrassTop = 0, texSandGold = 0, texOakLog = 0, texOakLeaves = 0;
GLuint texSnow = 0, texIceGold = 0, texClayGold = 0, texGravelGold = 0;
GLuint texGlowstoneGold = 0, texDiamondBlock = 0, texGoldBlock = 0, texIronBlockTex = 0;
GLuint texObsidian = 0, texSandstoneGold = 0, texCutSandstone = 0, texQuartzTop = 0;
GLuint texAndesite = 0, texDiorite = 0, texGranite = 0;
GLuint texPolAndesite = 0, texPolDiorite = 0, texPolGranite = 0;
GLuint texOakTrapdoor = 0, texOakDoorBot = 0, texWaterStill = 0;

// Textures — imported from the gfx_b3 project (see docs/plan_2.md Step 0).
// These are photographs, not 16x16 pixel art, so they load with GL_LINEAR
// magnification. Only the six that a planned step consumes are loaded;
// sun.jpg and moon.jpg are staged in assets/srcs/ but not wired to anything
// yet — the celestial bodies are still the procedural discs in skybox.h.
GLuint texCarBody = 0, texCarWindow = 0;  // step 2 — car paint and tinted glass
GLuint texRoad = 0, texRoof = 0;          // step 4 — crafted house roof, roads
GLuint texWorldMap = 0;                   // step 7 — desk globe
GLuint texHillIcon = 0;                   // step 3 — HILL tool hotbar icon

// =====================================================
// Craft-and-place structures (plan_2 Step 4)
// =====================================================
// gfx_b3's craft table produces a *structure standing in the world*, not an item
// in a slot — that is the one thing its crafting does that hexacraft's does not.
// These vectors are the whole persistence model, matching gfx_b3: session-only,
// redrawn from scratch every frame.
//
// Declared here rather than in input.h (where they are drawn) because
// gatherTorchLights() in world.h has to see the lit ones, and world.h is
// included first.
struct PlacedStructure {
    glm::vec3 pos;   // ground contact point, already snapped to terrain
    float yaw;       // radians; the structure's front faces the player who built it
};
std::vector<PlacedStructure> craftedHouses;
std::vector<glm::vec3>       craftedTrees;       // radially symmetric — no yaw
std::vector<glm::vec3>       craftedLights;      // ditto
std::vector<PlacedStructure> craftedFireplaces;

// Build tab on the crafting screen. Mutually exclusive with the recipe book:
// both panels want the same strip of screen left of the inventory.
bool buildTabOpen = false;

// Cost is always exactly two ingredients, so this is a flat struct rather than
// the 3x3 pattern the block recipes use — a build recipe has no shape.
struct BuildRecipe {
    const char* name;
    int t1, c1;      // first ingredient  (block type, count)
    int t2, c2;      // second ingredient
};
// gfx_b3's costs, mapped onto hexacraft's real block types: its MUD is
// BLOCK_DIRT, and its WOOD is BLOCK_WOOD (the enum has no BLOCK_OAK_LOG, which
// is what plan_2 wrote).
const BuildRecipe buildRecipes[] = {
    { "HOUSE",     BLOCK_DIRT,  5, BLOCK_STONE, 3 },
    { "TREE",      BLOCK_WOOD,  2, BLOCK_GRASS, 1 },
    { "TORCH",     BLOCK_WOOD,  1, BLOCK_STONE, 1 },
    { "FIREPLACE", BLOCK_STONE, 2, BLOCK_WOOD,  1 },
};
const int NUM_BUILD_RECIPES = 4;

// Hex mesh
GLuint hexVAO, hexVBO;
int hexVertexCount = 0;

// Phase 19B: draw trees from pre-baked VBOs (one draw call each) instead of
// re-running the fractal every frame. Toggle with F7 to A/B the old path.
bool useBakedTrees = true;

// =====================================================
// Custom myRotate (Rodrigues' rotation formula)
// Replaces glm::rotate
// =====================================================
glm::mat4 myRotate(glm::mat4 m, float angleRad, glm::vec3 axis) {
    axis = glm::normalize(axis);
    float c = cosf(angleRad);
    float s = sinf(angleRad);
    float t = 1.0f - c;
    float x = axis.x, y = axis.y, z = axis.z;

    glm::mat4 rot(1.0f);
    rot[0][0] = t * x * x + c;
    rot[0][1] = t * x * y + s * z;
    rot[0][2] = t * x * z - s * y;

    rot[1][0] = t * x * y - s * z;
    rot[1][1] = t * y * y + c;
    rot[1][2] = t * y * z + s * x;

    rot[2][0] = t * x * z + s * y;
    rot[2][1] = t * y * z - s * x;
    rot[2][2] = t * z * z + c;

    return m * rot;
}

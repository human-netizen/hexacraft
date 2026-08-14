#pragma once
// =====================================================
// Hexagonal grid positioning
// Flat-top hex: offset every other row
// =====================================================
glm::vec3 hexGridPos(int col, int row, float y) {
    float xSpacing = HEX_RADIUS * 2.0f * 0.866f; // sqrt(3)/2 * diameter
    float zSpacing = HEX_RADIUS * 1.5f;
    float x = col * xSpacing + (row % 2) * (xSpacing * 0.5f);
    float z = row * zSpacing;
    return glm::vec3(x, y, z);
}

// =====================================================
// Procedural Noise (value noise with smoothing)
// =====================================================
float hashNoise(int x, int y) {
    unsigned int n = (unsigned int)x * 374761393u + (unsigned int)y * 668265263u;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731u + 789221u) + 1376312589u) & 0x7fffffffu) / 1073741824.0f;
}

float smoothNoise(float x, float y) {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    // Smoothstep
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);

    float v00 = hashNoise(ix, iy);
    float v10 = hashNoise(ix + 1, iy);
    float v01 = hashNoise(ix, iy + 1);
    float v11 = hashNoise(ix + 1, iy + 1);

    float i0 = v00 + fx * (v10 - v00);
    float i1 = v01 + fx * (v11 - v01);
    return i0 + fy * (i1 - i0);
}

// Fractal brownian motion (octave noise)
float fbmNoise(float x, float y, int octaves = 4) {
    float val = 0.0f, amp = 1.0f, freq = 1.0f, maxVal = 0.0f;
    for (int i = 0; i < octaves; i++) {
        val += smoothNoise(x * freq, y * freq) * amp;
        maxVal += amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }
    return val / maxVal;
}

// Deterministic pseudo-random from grid coords
unsigned int gridSeed(int col, int row) {
    unsigned int s = ((unsigned int)col * 73856093u) ^ ((unsigned int)row * 19349663u);
    s = (s ^ (s >> 16)) * 0x45d9f3bu;
    return s;
}

// =====================================================
// Block colors
// =====================================================
const glm::vec3 COL_GRASS(0.3f, 0.7f, 0.15f);
const glm::vec3 COL_DIRT(0.55f, 0.35f, 0.17f);
const glm::vec3 COL_STONE(0.5f, 0.5f, 0.5f);
const glm::vec3 COL_STONE_LIGHT(0.85f, 0.85f, 0.85f); // White concrete
const glm::vec3 COL_SAND(0.76f, 0.7f, 0.5f);
const glm::vec3 COL_WATER(0.15f, 0.4f, 0.7f);
const glm::vec3 COL_WATER_DEEP(0.1f, 0.3f, 0.6f);
const glm::vec3 COL_ORE_DIAMOND(0.2f, 0.8f, 0.85f);
const glm::vec3 COL_ORE_GOLD(0.9f, 0.75f, 0.2f);
const glm::vec3 COL_WOOD(0.45f, 0.3f, 0.1f);
const glm::vec3 COL_WOOD_DARK(0.35f, 0.22f, 0.08f);

// Tree leaf color palette (colorful like reference images)
const glm::vec3 TREE_COLORS[] = {
    glm::vec3(0.7f, 0.15f, 0.1f),   // red
    glm::vec3(0.85f, 0.45f, 0.1f),  // orange
    glm::vec3(0.9f, 0.8f, 0.15f),   // yellow
    glm::vec3(0.15f, 0.7f, 0.7f),   // cyan
    glm::vec3(0.2f, 0.35f, 0.8f),   // blue
    glm::vec3(0.55f, 0.2f, 0.65f),  // purple
    glm::vec3(0.2f, 0.65f, 0.15f),  // green
    glm::vec3(0.6f, 0.1f, 0.3f),    // magenta
};
const int NUM_TREE_COLORS = 8;

// =====================================================
// Grid access helpers
// =====================================================
bool gridInBounds(int col, int row, int h) {
    int gi = col + GRID_OFF_X;
    int gj = row + GRID_OFF_Z;
    return gi >= 0 && gi < GRID_W && gj >= 0 && gj < GRID_D && h >= 0 && h < GRID_H;
}
int getBlock(int col, int row, int h) {
    if (!gridInBounds(col, row, h)) return BLOCK_AIR;
    return blockGrid[col + GRID_OFF_X][row + GRID_OFF_Z][h];
}
// =====================================================
// Light-emitting blocks
// =====================================================
// Glowstone, lanterns and torch blocks were drawn with isEmissive — they glowed
// on their own faces but contributed nothing to the point-light array, so a room
// walled in glowstone lit *nothing*, including itself, and read as flat.
//
// Kept as a map rather than a vector because setBlock has to be able to remove
// one in O(1) when the player mines it. Keyed exactly like blockState, so the
// same (col,row,h) packing is reused.
// Same type the per-frame light list uses, so a registry entry can be pushed
// into it directly.
struct PointLightSrc { glm::vec3 pos; glm::vec3 color; };
std::unordered_map<uint32_t, PointLightSrc> lightBlocks;

// Colour already scaled by how strongly the block emits, so consumers do not
// need a separate intensity term. Returns false for everything that is not a
// light source, which is nearly every block.
inline bool blockLightColor(int type, glm::vec3& outColor) {
    switch (type) {
        case BLOCK_GLOWSTONE:   outColor = glm::vec3(1.00f, 0.88f, 0.55f) * 1.15f; return true;
        case BLOCK_LANTERN:     outColor = glm::vec3(0.75f, 0.90f, 1.00f) * 0.90f; return true;
        case BLOCK_TORCH_BLOCK: outColor = glm::vec3(1.00f, 0.60f, 0.20f) * 1.00f; return true;
        default: return false;
    }
}

void setBlock(int col, int row, int h, int type) {
    if (!gridInBounds(col, row, h)) return;
    int gi = col + GRID_OFF_X, gj = row + GRID_OFF_Z;

    // Keep the light registry in step with the grid. Read the old type first —
    // replacing a glowstone with stone has to remove the light, not just fail to
    // add one.
    int oldType = blockGrid[gi][gj][h];
    if (oldType != type) {
        glm::vec3 lc;
        uint32_t lkey = blockStateKey(col, row, h);
        if (blockLightColor(oldType, lc)) lightBlocks.erase(lkey);
        if (blockLightColor(type, lc)) {
            glm::vec3 p = hexGridPos(col, row, 0.0f);
            // Matches the draw loop's `pos + vec3(0, h * HEX_HEIGHT, 0)`, so the
            // light sits inside the block that is emitting it.
            lightBlocks[lkey] = { glm::vec3(p.x, h * HEX_HEIGHT, p.z), lc };
        }
    }

    blockGrid[gi][gj][h] = type;
    // Update columnMaxH cache
    if (type != BLOCK_AIR) {
        if (h > columnMaxH[gi][gj]) columnMaxH[gi][gj] = h;
    } else {
        // Clear any block state when removing a block
        uint32_t key = blockStateKey(col, row, h);
        blockState.erase(key);
        if (h >= columnMaxH[gi][gj]) {
            // Broke the top block — scan down for new max
            int newMax = 0;
            for (int hh = h; hh >= 0; hh--) {
                if (blockGrid[gi][gj][hh] != BLOCK_AIR) { newMax = hh; break; }
            }
            columnMaxH[gi][gj] = newMax;
        }
    }
}

uint16_t getBlockState(int col, int row, int h) {
    if (!gridInBounds(col, row, h)) return 0;
    uint32_t key = blockStateKey(col, row, h);
    auto it = blockState.find(key);
    return (it != blockState.end()) ? it->second : 0;
}

void setBlockState(int col, int row, int h, uint16_t state) {
    if (!gridInBounds(col, row, h)) return;
    uint32_t key = blockStateKey(col, row, h);
    if (state == 0)
        blockState.erase(key);
    else
        blockState[key] = state;
}

glm::vec3 getBlockColor(int type) {
    switch (type) {
        case BLOCK_GRASS: return COL_GRASS;
        case BLOCK_DIRT: return COL_DIRT;
        case BLOCK_SAND: return COL_SAND;
        case BLOCK_STONE: return COL_STONE;
        case BLOCK_STONE_LIGHT: return COL_STONE_LIGHT;
        case BLOCK_WATER: return COL_WATER;
        case BLOCK_WOOD: return COL_WOOD_DARK;
        case BLOCK_LEAF: return glm::vec3(0.2f, 0.65f, 0.15f);
        case BLOCK_ORE_DIAMOND: return COL_ORE_DIAMOND;
        case BLOCK_ORE_GOLD: return COL_ORE_GOLD;
        case BLOCK_GLASS: return glm::vec3(0.8f, 0.9f, 0.95f);
        case BLOCK_BEDROCK: return glm::vec3(0.15f, 0.15f, 0.15f);
        case BLOCK_COAL_ORE: return glm::vec3(0.12f, 0.12f, 0.12f);
        case BLOCK_GRAVEL: return glm::vec3(0.55f, 0.52f, 0.48f);
        case BLOCK_CLAY: return glm::vec3(0.62f, 0.58f, 0.55f);
        case BLOCK_SNOW: return glm::vec3(0.95f, 0.97f, 1.0f);
        case BLOCK_ICE: return glm::vec3(0.6f, 0.8f, 1.0f);
        case BLOCK_PLANKS: return glm::vec3(0.6f, 0.45f, 0.25f);
        case BLOCK_CRAFTING_TABLE: return glm::vec3(0.5f, 0.35f, 0.2f);
        case BLOCK_SANDSTONE: return glm::vec3(0.85f, 0.8f, 0.6f);
        case BLOCK_BRICKS: return glm::vec3(0.6f, 0.3f, 0.25f);
        case BLOCK_WOOL_WHITE: return glm::vec3(0.9f, 0.9f, 0.9f);
        case BLOCK_WOOL_RED: return glm::vec3(0.7f, 0.2f, 0.2f);
        case BLOCK_WOOL_BLUE: return glm::vec3(0.2f, 0.3f, 0.7f);
        case BLOCK_COBBLESTONE: return glm::vec3(0.45f, 0.45f, 0.45f);
        case BLOCK_MOSSY_COBBLESTONE: return glm::vec3(0.4f, 0.5f, 0.4f);
        case BLOCK_GLOWSTONE: return glm::vec3(0.9f, 0.8f, 0.3f);
        case BLOCK_POLISHED_DIORITE: return glm::vec3(0.8f, 0.8f, 0.8f);
        case BLOCK_POLISHED_GRANITE: return glm::vec3(0.6f, 0.4f, 0.35f);
        case BLOCK_POLISHED_ANDESITE: return glm::vec3(0.5f, 0.5f, 0.5f);
        case BLOCK_QUARTZ_BLOCK: return glm::vec3(0.9f, 0.88f, 0.85f);
        case BLOCK_IRON_BLOCK: return glm::vec3(0.85f, 0.85f, 0.85f);
        case BLOCK_DIAMOND_BLOCK: return glm::vec3(0.3f, 0.8f, 0.8f);
        case BLOCK_GOLD_BLOCK: return glm::vec3(0.95f, 0.85f, 0.2f);
        case BLOCK_CUT_SANDSTONE: return glm::vec3(0.82f, 0.77f, 0.55f);
        case BLOCK_MOSSY_BRICKS: return glm::vec3(0.45f, 0.4f, 0.3f);
        case BLOCK_OBSIDIAN: return glm::vec3(0.1f, 0.05f, 0.15f);

        // --- Building & Decoration ---
        // Wool colors
        case BLOCK_WOOL_GREEN: return glm::vec3(0.2f, 0.5f, 0.15f);
        case BLOCK_WOOL_YELLOW: return glm::vec3(0.9f, 0.85f, 0.2f);
        case BLOCK_WOOL_BLACK: return glm::vec3(0.1f, 0.1f, 0.1f);
        case BLOCK_WOOL_ORANGE: return glm::vec3(0.85f, 0.5f, 0.1f);
        case BLOCK_WOOL_PINK: return glm::vec3(0.85f, 0.55f, 0.65f);
        case BLOCK_WOOL_PURPLE: return glm::vec3(0.45f, 0.2f, 0.6f);
        case BLOCK_WOOL_CYAN: return glm::vec3(0.15f, 0.6f, 0.6f);
        case BLOCK_WOOL_BROWN: return glm::vec3(0.4f, 0.25f, 0.12f);
        case BLOCK_WOOL_GRAY: return glm::vec3(0.35f, 0.35f, 0.35f);
        case BLOCK_WOOL_LIGHT_GRAY: return glm::vec3(0.6f, 0.6f, 0.6f);
        case BLOCK_WOOL_MAGENTA: return glm::vec3(0.7f, 0.2f, 0.55f);
        case BLOCK_WOOL_LIME: return glm::vec3(0.5f, 0.8f, 0.1f);

        // Full blocks
        case BLOCK_BOOKSHELF: return glm::vec3(0.55f, 0.4f, 0.2f);
        case BLOCK_SMOOTH_STONE: return glm::vec3(0.6f, 0.6f, 0.6f);
        case BLOCK_TERRACOTTA: return glm::vec3(0.6f, 0.4f, 0.3f);

        // Slabs (same color as their base material)
        case BLOCK_SLAB_STONE: return glm::vec3(0.5f, 0.5f, 0.5f);
        case BLOCK_SLAB_WOOD: return glm::vec3(0.6f, 0.45f, 0.25f);
        case BLOCK_SLAB_SANDSTONE: return glm::vec3(0.85f, 0.8f, 0.6f);
        case BLOCK_SLAB_BRICK: return glm::vec3(0.6f, 0.3f, 0.25f);

        // Stairs
        case BLOCK_STAIRS_STONE: return glm::vec3(0.5f, 0.5f, 0.5f);
        case BLOCK_STAIRS_WOOD: return glm::vec3(0.6f, 0.45f, 0.25f);

        // Carpets
        case BLOCK_CARPET_WHITE: return glm::vec3(0.9f, 0.9f, 0.9f);
        case BLOCK_CARPET_RED: return glm::vec3(0.7f, 0.2f, 0.2f);
        case BLOCK_CARPET_BLUE: return glm::vec3(0.2f, 0.3f, 0.7f);

        // Thin blocks
        case BLOCK_GLASS_PANE: return glm::vec3(0.8f, 0.9f, 0.95f);
        case BLOCK_IRON_BARS: return glm::vec3(0.6f, 0.6f, 0.6f);
        case BLOCK_FENCE_WOOD: return glm::vec3(0.55f, 0.4f, 0.2f);

        // Interactive
        case BLOCK_DOOR_OAK: return glm::vec3(0.55f, 0.4f, 0.2f);
        case BLOCK_DOOR_IRON: return glm::vec3(0.75f, 0.75f, 0.75f);
        case BLOCK_TRAPDOOR_OAK: return glm::vec3(0.5f, 0.38f, 0.18f);
        case BLOCK_FENCE_GATE: return glm::vec3(0.55f, 0.4f, 0.2f);
        case BLOCK_LADDER: return glm::vec3(0.6f, 0.45f, 0.2f);

        // Decorative
        case BLOCK_LANTERN: return glm::vec3(0.9f, 0.7f, 0.2f);
        case BLOCK_TORCH_BLOCK: return glm::vec3(1.0f, 0.6f, 0.1f);
        case BLOCK_SIGN: return glm::vec3(0.6f, 0.45f, 0.25f);
        case BLOCK_BANNER: return glm::vec3(0.9f, 0.9f, 0.9f);

        // Tools
        case ITEM_STICK:           return glm::vec3(0.6f, 0.4f, 0.15f);
        case ITEM_SWORD_WOOD:      case ITEM_AXE_WOOD:
        case ITEM_PICKAXE_WOOD:    case ITEM_SHOVEL_WOOD:
            return glm::vec3(0.65f, 0.45f, 0.2f);
        case ITEM_SWORD_STONE:     case ITEM_AXE_STONE:
        case ITEM_PICKAXE_STONE:   case ITEM_SHOVEL_STONE:
            return glm::vec3(0.55f, 0.55f, 0.55f);
        case ITEM_SWORD_IRON:      case ITEM_AXE_IRON:
        case ITEM_PICKAXE_IRON:    case ITEM_SHOVEL_IRON:
            return glm::vec3(0.8f, 0.8f, 0.85f);
        case ITEM_SWORD_DIAMOND:   case ITEM_AXE_DIAMOND:
        case ITEM_PICKAXE_DIAMOND: case ITEM_SHOVEL_DIAMOND:
            return glm::vec3(0.3f, 0.85f, 0.9f);
        case ITEM_BOW:             return glm::vec3(0.55f, 0.38f, 0.15f);
        // Sculpt tools are drawn as their own imported icons, so these tints
        // stay white — the icon texture is multiplied by this colour.
        case ITEM_TOOL_HILL:       return glm::vec3(1.0f, 1.0f, 1.0f);
        case ITEM_TOOL_POND:       return glm::vec3(0.45f, 0.65f, 0.95f);

        default: return glm::vec3(1, 0, 1); // magenta = error
    }
}

// =====================================================
// Block drop table — what item a block drops when broken
// =====================================================
int getBlockDrop(int type) {
    switch (type) {
        // Grass → dirt (top block breaks to dirt)
        case BLOCK_GRASS:           return BLOCK_DIRT;
        // Stone → cobblestone
        case BLOCK_STONE:           return BLOCK_COBBLESTONE;
        // Sandstone → sandstone (keeps itself)
        case BLOCK_SANDSTONE:       return BLOCK_SANDSTONE;
        // Glass and ice drop nothing (no silk touch)
        case BLOCK_GLASS:           return BLOCK_AIR;
        case BLOCK_GLASS_PANE:      return BLOCK_AIR;
        case BLOCK_ICE:             return BLOCK_AIR;
        // Leaves drop nothing (no shears)
        case BLOCK_LEAF:            return BLOCK_AIR;
        // Bedrock is unbreakable — no drop
        case BLOCK_BEDROCK:         return BLOCK_AIR;
        // Water / air — no drop
        case BLOCK_WATER:           return BLOCK_AIR;
        case BLOCK_AIR:             return BLOCK_AIR;
        // Everything else drops itself
        default:                    return type;
    }
}

// =====================================================
// Block hardness — base seconds to break bare-handed
// =====================================================
float getBlockHardness(int type) {
    switch (type) {
        case BLOCK_AIR:             return 0.0f;
        case BLOCK_WATER:           return 0.5f;

        // Soft — dirt/sand/gravel/snow
        case BLOCK_DIRT:
        case BLOCK_SAND:
        case BLOCK_GRAVEL:
        case BLOCK_SNOW:
        case BLOCK_CLAY:            return 0.4f;

        case BLOCK_GRASS:           return 0.5f;

        // Wood / leaf
        case BLOCK_WOOD:
        case BLOCK_PLANKS:
        case BLOCK_LEAF:
        case BLOCK_CRAFTING_TABLE:
        case BLOCK_BOOKSHELF:
        case BLOCK_FENCE_WOOD:
        case BLOCK_FENCE_GATE:
        case BLOCK_DOOR_OAK:
        case BLOCK_TRAPDOOR_OAK:
        case BLOCK_SLAB_WOOD:
        case BLOCK_STAIRS_WOOD:
        case BLOCK_LADDER:
        case BLOCK_SIGN:
        case BLOCK_BANNER:          return 0.8f;

        // Wool / carpet
        case BLOCK_WOOL_WHITE: case BLOCK_WOOL_RED: case BLOCK_WOOL_BLUE:
        case BLOCK_WOOL_GREEN: case BLOCK_WOOL_YELLOW: case BLOCK_WOOL_BLACK:
        case BLOCK_WOOL_ORANGE: case BLOCK_WOOL_PINK: case BLOCK_WOOL_PURPLE:
        case BLOCK_WOOL_CYAN: case BLOCK_WOOL_BROWN: case BLOCK_WOOL_GRAY:
        case BLOCK_WOOL_LIGHT_GRAY: case BLOCK_WOOL_MAGENTA: case BLOCK_WOOL_LIME:
        case BLOCK_CARPET_WHITE: case BLOCK_CARPET_RED: case BLOCK_CARPET_BLUE:
                                    return 0.4f;

        // Glass / ice — fragile
        case BLOCK_GLASS:
        case BLOCK_GLASS_PANE:
        case BLOCK_ICE:             return 0.2f;

        // Stone-type
        case BLOCK_STONE:
        case BLOCK_COBBLESTONE:
        case BLOCK_MOSSY_COBBLESTONE:
        case BLOCK_STONE_LIGHT:
        case BLOCK_BRICKS:
        case BLOCK_MOSSY_BRICKS:
        case BLOCK_SANDSTONE:
        case BLOCK_CUT_SANDSTONE:
        case BLOCK_SMOOTH_STONE:
        case BLOCK_POLISHED_DIORITE:
        case BLOCK_POLISHED_GRANITE:
        case BLOCK_POLISHED_ANDESITE:
        case BLOCK_QUARTZ_BLOCK:
        case BLOCK_TERRACOTTA:
        case BLOCK_SLAB_STONE:
        case BLOCK_SLAB_SANDSTONE:
        case BLOCK_SLAB_BRICK:
        case BLOCK_STAIRS_STONE:
        case BLOCK_IRON_BARS:
        case BLOCK_DOOR_IRON:       return 2.5f;

        // Ores
        case BLOCK_COAL_ORE:
        case BLOCK_ORE_GOLD:
        case BLOCK_ORE_DIAMOND:     return 4.0f;

        // Heavy metal blocks
        case BLOCK_IRON_BLOCK:
        case BLOCK_GOLD_BLOCK:
        case BLOCK_DIAMOND_BLOCK:   return 6.0f;

        // Glowstone / torches / lanterns
        case BLOCK_GLOWSTONE:
        case BLOCK_LANTERN:
        case BLOCK_TORCH_BLOCK:     return 0.3f;

        // Obsidian — hard even with diamond pick
        case BLOCK_OBSIDIAN:        return 10.0f;

        // Bedrock — very hard but decomposable
        case BLOCK_BEDROCK:         return 8.0f;

        default:                    return 1.5f;
    }
}

// Returns true if this block type is in the stone/ore/metal category
bool isStoneType(int bt) {
    switch (bt) {
        case BLOCK_STONE: case BLOCK_COBBLESTONE: case BLOCK_MOSSY_COBBLESTONE:
        case BLOCK_STONE_LIGHT: case BLOCK_BRICKS: case BLOCK_MOSSY_BRICKS:
        case BLOCK_SANDSTONE: case BLOCK_CUT_SANDSTONE: case BLOCK_SMOOTH_STONE:
        case BLOCK_POLISHED_DIORITE: case BLOCK_POLISHED_GRANITE: case BLOCK_POLISHED_ANDESITE:
        case BLOCK_QUARTZ_BLOCK: case BLOCK_TERRACOTTA: case BLOCK_IRON_BLOCK:
        case BLOCK_GOLD_BLOCK: case BLOCK_DIAMOND_BLOCK: case BLOCK_GLOWSTONE:
        case BLOCK_SLAB_STONE: case BLOCK_SLAB_SANDSTONE: case BLOCK_SLAB_BRICK:
        case BLOCK_STAIRS_STONE: case BLOCK_COAL_ORE: case BLOCK_ORE_GOLD:
        case BLOCK_ORE_DIAMOND: case BLOCK_IRON_BARS: case BLOCK_DOOR_IRON:
        case BLOCK_OBSIDIAN:        return true;
        default:                    return false;
    }
}

bool isWoodType(int bt) {
    switch (bt) {
        case BLOCK_WOOD: case BLOCK_PLANKS: case BLOCK_LEAF:
        case BLOCK_CRAFTING_TABLE: case BLOCK_BOOKSHELF: case BLOCK_FENCE_WOOD:
        case BLOCK_FENCE_GATE: case BLOCK_DOOR_OAK: case BLOCK_TRAPDOOR_OAK:
        case BLOCK_SLAB_WOOD: case BLOCK_STAIRS_WOOD: case BLOCK_LADDER:
        case BLOCK_SIGN: case BLOCK_BANNER:     return true;
        default:                                return false;
    }
}

bool isDirtType(int bt) {
    switch (bt) {
        case BLOCK_DIRT: case BLOCK_GRASS: case BLOCK_SAND: case BLOCK_GRAVEL:
        case BLOCK_SNOW: case BLOCK_CLAY:  return true;
        default:                           return false;
    }
}

// Speed multiplier: how much faster the held tool breaks the target block
float getToolSpeedMultiplier(int toolType, int blockType) {
    // Obsidian: only diamond pickaxe works meaningfully
    if (blockType == BLOCK_OBSIDIAN) {
        switch (toolType) {
            case ITEM_PICKAXE_DIAMOND: return 8.0f;
            case ITEM_PICKAXE_IRON:    return 3.0f;
            default:                   return 0.5f; // painfully slow but not 0
        }
    }

    bool stone = isStoneType(blockType);
    bool wood  = isWoodType(blockType);
    bool dirt  = isDirtType(blockType);

    // --- Right-tool tiers give big bonus ---
    // Pickaxes best on stone
    if (stone) {
        switch (toolType) {
            case ITEM_PICKAXE_WOOD:    return 3.0f;
            case ITEM_PICKAXE_STONE:   return 5.0f;
            case ITEM_PICKAXE_IRON:    return 7.0f;
            case ITEM_PICKAXE_DIAMOND: return 9.0f;
            default: break;
        }
    }
    // Axes best on wood
    if (wood) {
        switch (toolType) {
            case ITEM_AXE_WOOD:    return 3.0f;
            case ITEM_AXE_STONE:   return 5.0f;
            case ITEM_AXE_IRON:    return 7.0f;
            case ITEM_AXE_DIAMOND: return 9.0f;
            default: break;
        }
    }
    // Shovels best on dirt/sand/gravel/snow/clay
    if (dirt) {
        switch (toolType) {
            case ITEM_SHOVEL_WOOD:    return 3.0f;
            case ITEM_SHOVEL_STONE:   return 5.0f;
            case ITEM_SHOVEL_IRON:    return 7.0f;
            case ITEM_SHOVEL_DIAMOND: return 9.0f;
            default: break;
        }
    }

    // --- Any tool (wrong type) gives 1.5× — always noticeable vs bare hand ---
    // The sculpt tools are excluded alongside the stick and bow: they are not
    // mining implements, and letting them speed up breaking would make them a
    // strictly-better pickaxe that never wears out.
    bool anyTool = (toolType >= ITEM_STICK && toolType < ITEM_COUNT);
    if (anyTool && toolType != ITEM_STICK && toolType != ITEM_BOW
        && toolType != ITEM_TOOL_HILL && toolType != ITEM_TOOL_POND) return 1.5f;

    return 1.0f; // bare hand / stick
}

// Compute effective break duration in seconds for current player + target block
float getBreakDuration(int toolType, int blockType) {
    float hardness = getBlockHardness(blockType);
    if (hardness <= 0.0f) return 0.0f;   // instant (air etc.)
    if (hardness < 0.0f) return -1.0f;   // unbreakable
    float speed = getToolSpeedMultiplier(toolType, blockType);
    if (speed <= 0.0f) return -1.0f;     // can't break
    return hardness / speed;
}

// =====================================================
// Biome system: 0=sand, 1=grass, 2=stone, 3=water
// =====================================================
int getBiome(int col, int row) {
    float bx = col * 0.025f + 100.0f;
    float by = row * 0.025f + 100.0f;
    float n = fbmNoise(bx, by, 3);

    float temp = fbmNoise(col * 0.015f - 50.0f, row * 0.015f + 200.0f, 2);
    if (temp < -0.3f && n >= -0.05f) return 4; // snow biome
    if (n < -0.35f) return 3; // water
    if (n < -0.05f) return 0; // sand
    if (n < 0.45f)  return 1; // grass
    return 2;                  // stone
}

int getTerrainHeightBiome(int col, int row, int biome) {
    float nx = col * 0.04f;
    float ny = row * 0.04f;
    float n = fbmNoise(nx, ny, 5);
    float detail = fbmNoise(col * 0.12f, row * 0.12f, 2) * 0.25f;
    // Large-scale continental noise for mountains
    float continental = fbmNoise(col * 0.012f + 50.0f, row * 0.012f + 50.0f, 3);

    switch (biome) {
        case 0: { // sand: flat desert with gentle dunes
            int h = (int)floorf((n + detail + 1.0f) * 1.5f);
            if (h < 0) h = 0;
            if (h > 3) h = 3;
            return h + UNDERGROUND_DEPTH;
        }
        case 1: { // grass: rolling hills, some tall
            float hillFactor = (continental > 0.2f) ? 2.0f : 1.0f;
            int h = (int)floorf((n + detail + 1.0f) * 3.5f * hillFactor);
            if (h < 1) h = 1;
            if (h > 12) h = 12;
            return h + UNDERGROUND_DEPTH;
        }
        case 2: { // stone: mountains with dramatic peaks
            float mountainBoost = (continental > 0.0f) ? continental * 6.0f : 0.0f;
            int h = (int)floorf((n + detail + 1.0f) * 4.0f + mountainBoost) + 4;
            if (h < 4) h = 4;
            if (h > 20) h = 20;
            return h + UNDERGROUND_DEPTH;
        }
        case 3: return UNDERGROUND_DEPTH - 1; // water at underground depth level
    }
    return UNDERGROUND_DEPTH;
}

// =====================================================
// Trees — Fractal recursive branching (visible fork pattern)
// =====================================================

// Recursive fractal branch that draws clearly visible Y-forks.
// Each branch is a thin chain of hexes; at each recursion level it
// splits into 2–3 children with wider spread angles so the classic
// fractal tree shape is unmistakable.
//
// depth     = recursion levels remaining (0 = terminal twig)
// maxDepth  = the initial depth (used to scale thickness)
void drawFractalBranch(glm::vec3 pos, glm::vec3 dir, float length,
                       float thickness, int depth, int maxDepth,
                       glm::vec3 leafColor, unsigned int seed) {

    // --- Draw this branch as a chain of thin hex "logs" ---
    // More segments = smoother branch line
    float segLen = HEX_HEIGHT * 0.45f;
    int segments = std::max(2, (int)(length / segLen));
    glm::vec3 step = dir * (length / (float)segments);

    // At depth 0 and 1, draw branches with leaf color (twigs)
    bool isTwig = (depth <= 1);

    for (int i = 0; i < segments; i++) {
        glm::vec3 p = pos + step * (float)i;
        float v = 0.85f + 0.15f * hashNoise((int)(p.x * 10 + seed), (int)(p.y * 7));

        glm::vec3 branchColor;
        if (isTwig) {
            // Twig: blend wood -> leaf color toward the tip
            float t = (float)i / (float)segments;
            branchColor = COL_WOOD_DARK * (1.0f - t) + leafColor * t;
            branchColor *= v;
        } else {
            branchColor = COL_WOOD_DARK * v;
        }

        drawHex(p, branchColor, glm::vec3(thickness, 0.7f, thickness));
    }

    glm::vec3 tip = pos + dir * length;

    // --- Base case: terminal twig — just a few tiny leaf hexes ---
    if (depth <= 0) {
        seed = seed * 1103515245 + 12345;
        // 2-4 tiny leaf hexes clustered at the tip
        int numLeaves = 2 + (int)((seed >> 16) % 3);
        for (int i = 0; i < numLeaves; i++) {
            seed = seed * 1103515245 + 12345;
            float ox = ((float)((seed >> 4) % 60) - 30.0f) * 0.012f;
            float oy = ((float)((seed >> 8) % 40) - 10.0f) * 0.012f;
            float oz = ((float)((seed >> 12) % 60) - 30.0f) * 0.012f;
            float bright = 0.8f + 0.4f * ((seed >> 16) % 100) / 100.0f;
            drawHex(tip + glm::vec3(ox, oy, oz), leafColor * bright,
                    glm::vec3(0.25f, 0.25f, 0.25f));
        }
        return;
    }

    // --- Recursive case: fork into 2-3 children ---
    seed = seed * 1103515245 + 12345;
    int numChildren = 2 + (int)((seed >> 16) % 2);  // 2 or 3

    // Even azimuth spacing with jitter
    float baseAzimuth = (float)((seed >> 8) % 360) * PI / 180.0f;

    for (int c = 0; c < numChildren; c++) {
        seed = seed * 1103515245 + 12345;

        // Azimuth: evenly spaced around branch axis + jitter
        float azimuth = baseAzimuth + c * (2.0f * PI / numChildren)
                        + ((float)((seed >> 12) % 50) - 25.0f) * PI / 180.0f;

        // Elevation from vertical: WIDE spread so forks are clearly visible
        // Higher depth (early forks) = more upward; lower depth = wider spread
        float spreadAngle = 0.6f + 0.4f * (1.0f - (float)depth / (float)maxDepth);
        // spreadAngle goes from ~0.6 rad (early) to ~1.0 rad (late) from vertical
        float elevJitter = ((float)((seed >> 4) % 30) - 15.0f) * PI / 180.0f;
        float elevation = (PI / 2.0f) - spreadAngle + elevJitter;
        // elevation = angle from horizontal plane

        // Build child direction
        glm::vec3 childDir;
        childDir.x = cosf(elevation) * cosf(azimuth);
        childDir.y = sinf(elevation);
        childDir.z = cosf(elevation) * sinf(azimuth);

        // Light blend with parent for continuity
        float parentBlend = 0.15f;
        childDir = glm::normalize(childDir + dir * parentBlend);

        // Children: length shrinks by ~0.7, thickness by ~0.65
        float childLength    = length * (0.6f + 0.1f * ((seed >> 20) % 100) / 100.0f);
        float childThickness = thickness * (0.6f + 0.1f * ((seed >> 24) % 100) / 100.0f);

        drawFractalBranch(tip, childDir, childLength, childThickness,
                          depth - 1, maxDepth, leafColor, seed);
    }
}

// Normal tree — 3-depth fractal
void drawTree(glm::vec3 base, glm::vec3 leafColor) {
    unsigned int seed = gridSeed((int)(base.x * 10), (int)(base.z * 10));

    // --- Trunk ---
    int trunkH = 5;
    for (int i = 0; i < trunkH; i++) {
        glm::vec3 p = base + glm::vec3(0, i * HEX_HEIGHT, 0);
        float v = 0.85f + 0.15f * hashNoise((int)(base.x * 10), i);
        drawHex(p, COL_WOOD_DARK * v, glm::vec3(0.5f, 1.0f, 0.5f));
        // Root flare
        if (i < 2) {
            drawHex(p + glm::vec3(0.25f, 0, 0), COL_WOOD_DARK * v, glm::vec3(0.25f, 0.6f, 0.25f));
            drawHex(p + glm::vec3(-0.2f, 0, 0.2f), COL_WOOD_DARK * v, glm::vec3(0.25f, 0.6f, 0.25f));
            drawHex(p + glm::vec3(0.0f, 0, -0.2f), COL_WOOD_DARK * v, glm::vec3(0.2f, 0.5f, 0.2f));
        }
    }

    // --- Fractal crown ---
    glm::vec3 trunkTop = base + glm::vec3(0, trunkH * HEX_HEIGHT, 0);
    int   fracDepth = 3;
    float branchLen = 2.5f;
    float branchThk = 0.35f;

    // Central leader
    drawFractalBranch(trunkTop, glm::vec3(0.0f, 1.0f, 0.0f), branchLen * 0.9f,
                      branchThk * 0.9f, fracDepth, fracDepth, leafColor, seed * 7 + 1);

    // 2-3 lateral main branches with wide spread
    seed = seed * 1103515245 + 12345;
    int laterals = 2 + (int)((seed >> 16) % 2);
    float az0 = (float)((seed >> 8) % 360) * PI / 180.0f;
    for (int b = 0; b < laterals; b++) {
        float az = az0 + b * (2.0f * PI / laterals);
        glm::vec3 bdir = glm::normalize(glm::vec3(cosf(az) * 0.7f, 0.65f, sinf(az) * 0.7f));
        seed = seed * 1103515245 + 12345;
        drawFractalBranch(trunkTop, bdir, branchLen, branchThk,
                          fracDepth, fracDepth, leafColor, seed);
    }
}

// Large tree — 5-depth fractal for dense branching
void drawLargeTree(glm::vec3 base) {
    unsigned int seed = gridSeed((int)(base.x * 7), (int)(base.z * 7));
    glm::vec3 green(0.15f, 0.55f, 0.12f);

    // --- Thick trunk ---
    int trunkH = 7;
    for (int i = 0; i < trunkH; i++) {
        glm::vec3 p = base + glm::vec3(0, i * HEX_HEIGHT, 0);
        float v = 0.85f + 0.15f * hashNoise((int)(base.x * 10), i);
        drawHex(p, COL_WOOD_DARK * v, glm::vec3(0.7f, 1.0f, 0.7f));
        if (i < 3) {
            drawHex(p + glm::vec3(0.35f, 0, 0.15f), COL_WOOD_DARK * v, glm::vec3(0.35f, 0.7f, 0.35f));
            drawHex(p + glm::vec3(-0.3f, 0, -0.2f), COL_WOOD_DARK * v, glm::vec3(0.35f, 0.7f, 0.35f));
            drawHex(p + glm::vec3(-0.05f, 0, 0.35f), COL_WOOD_DARK * v, glm::vec3(0.3f, 0.6f, 0.3f));
        }
    }

    // --- Fractal crown — 5 levels deep ---
    glm::vec3 trunkTop = base + glm::vec3(0, trunkH * HEX_HEIGHT, 0);
    int   fracDepth = 5;
    float branchLen = 3.2f;
    float branchThk = 0.45f;

    // Central leader
    drawFractalBranch(trunkTop, glm::vec3(0.0f, 1.0f, 0.0f), branchLen * 1.0f,
                      branchThk * 0.9f, fracDepth, fracDepth, green, seed * 13 + 3);

    // 3–4 lateral main branches
    seed = seed * 1103515245 + 12345;
    int laterals = 3 + (int)((seed >> 16) % 2);
    float az0 = (float)((seed >> 8) % 360) * PI / 180.0f;
    for (int b = 0; b < laterals; b++) {
        float az = az0 + b * (2.0f * PI / laterals);
        glm::vec3 bdir = glm::normalize(glm::vec3(cosf(az) * 0.75f, 0.6f, sinf(az) * 0.75f));
        seed = seed * 1103515245 + 12345;
        drawFractalBranch(trunkTop, bdir, branchLen, branchThk,
                          fracDepth, fracDepth, green, seed);
    }
}

// =====================================================
// Baked trees (Phase 19B)
// The draw* functions above rebuild a 3–5 level fractal every frame, which cost
// ~65,000 draw calls/frame — 78% of the frame. The build* functions below emit
// the identical geometry into a vertex list once, so each tree becomes one
// glDrawArrays.
//
// Colours are not baked as RGB. Every colour the fractal produced has the form
// mix(COL_WOOD_DARK, leafColor, blend) * shade, so the vertex colour attribute
// carries (blend, shade, 0) and the shader reconstitutes it with the leaf colour
// of whichever tree is being drawn (colorMode = 1). That decouples shape from
// colour, so one baked mesh serves every tree that shares its shape.
//
// Geometry is built in tree-local space (base at the origin) so the mesh is
// position-independent and the draw only needs a translation.
// =====================================================

// Number of distinct baked shapes. Trees pick one by seed, so the forest still
// looks varied, but 55 trees cost this many meshes instead of 55.
const int NUM_TREE_VARIANTS       = 16;
const int NUM_LARGE_TREE_VARIANTS = 8;

void buildFractalBranch(std::vector<Vertex>& out,
                        glm::vec3 pos, glm::vec3 dir, float length,
                        float thickness, int depth, int maxDepth,
                        unsigned int seed) {

    float segLen = HEX_HEIGHT * 0.45f;
    int segments = std::max(2, (int)(length / segLen));
    glm::vec3 step = dir * (length / (float)segments);

    bool isTwig = (depth <= 1);

    for (int i = 0; i < segments; i++) {
        glm::vec3 p = pos + step * (float)i;
        float v = 0.85f + 0.15f * hashNoise((int)(p.x * 10 + seed), (int)(p.y * 7));

        // blend = 0 pure wood, 1 pure leaf; shade = the brightness multiplier
        float blend = isTwig ? (float)i / (float)segments : 0.0f;
        bakeHex(out, p, glm::vec3(blend, v, 0.0f), glm::vec3(thickness, 0.7f, thickness));
    }

    glm::vec3 tip = pos + dir * length;

    if (depth <= 0) {
        seed = seed * 1103515245 + 12345;
        int numLeaves = 2 + (int)((seed >> 16) % 3);
        for (int i = 0; i < numLeaves; i++) {
            seed = seed * 1103515245 + 12345;
            float ox = ((float)((seed >> 4) % 60) - 30.0f) * 0.012f;
            float oy = ((float)((seed >> 8) % 40) - 10.0f) * 0.012f;
            float oz = ((float)((seed >> 12) % 60) - 30.0f) * 0.012f;
            float bright = 0.8f + 0.4f * ((seed >> 16) % 100) / 100.0f;
            bakeHex(out, tip + glm::vec3(ox, oy, oz), glm::vec3(1.0f, bright, 0.0f),
                    glm::vec3(0.25f, 0.25f, 0.25f));
        }
        return;
    }

    seed = seed * 1103515245 + 12345;
    int numChildren = 2 + (int)((seed >> 16) % 2);

    float baseAzimuth = (float)((seed >> 8) % 360) * PI / 180.0f;

    for (int c = 0; c < numChildren; c++) {
        seed = seed * 1103515245 + 12345;

        float azimuth = baseAzimuth + c * (2.0f * PI / numChildren)
                        + ((float)((seed >> 12) % 50) - 25.0f) * PI / 180.0f;

        float spreadAngle = 0.6f + 0.4f * (1.0f - (float)depth / (float)maxDepth);
        float elevJitter = ((float)((seed >> 4) % 30) - 15.0f) * PI / 180.0f;
        float elevation = (PI / 2.0f) - spreadAngle + elevJitter;

        glm::vec3 childDir;
        childDir.x = cosf(elevation) * cosf(azimuth);
        childDir.y = sinf(elevation);
        childDir.z = cosf(elevation) * sinf(azimuth);

        float parentBlend = 0.15f;
        childDir = glm::normalize(childDir + dir * parentBlend);

        float childLength    = length * (0.6f + 0.1f * ((seed >> 20) % 100) / 100.0f);
        float childThickness = thickness * (0.6f + 0.1f * ((seed >> 24) % 100) / 100.0f);

        buildFractalBranch(out, tip, childDir, childLength, childThickness,
                           depth - 1, maxDepth, seed);
    }
}

// Mirror of drawTree(), emitting into `out` with the base at the origin.
void buildTree(std::vector<Vertex>& out, unsigned int seed) {
    int trunkH = 5;
    for (int i = 0; i < trunkH; i++) {
        glm::vec3 p = glm::vec3(0, i * HEX_HEIGHT, 0);
        float v = 0.85f + 0.15f * hashNoise((int)seed, i);
        bakeHex(out, p, glm::vec3(0.0f, v, 0.0f), glm::vec3(0.5f, 1.0f, 0.5f));
        if (i < 2) {
            bakeHex(out, p + glm::vec3(0.25f, 0, 0),     glm::vec3(0.0f, v, 0.0f), glm::vec3(0.25f, 0.6f, 0.25f));
            bakeHex(out, p + glm::vec3(-0.2f, 0, 0.2f),  glm::vec3(0.0f, v, 0.0f), glm::vec3(0.25f, 0.6f, 0.25f));
            bakeHex(out, p + glm::vec3(0.0f, 0, -0.2f),  glm::vec3(0.0f, v, 0.0f), glm::vec3(0.2f, 0.5f, 0.2f));
        }
    }

    glm::vec3 trunkTop = glm::vec3(0, trunkH * HEX_HEIGHT, 0);
    int   fracDepth = 3;
    float branchLen = 2.5f;
    float branchThk = 0.35f;

    buildFractalBranch(out, trunkTop, glm::vec3(0.0f, 1.0f, 0.0f), branchLen * 0.9f,
                       branchThk * 0.9f, fracDepth, fracDepth, seed * 7 + 1);

    seed = seed * 1103515245 + 12345;
    int laterals = 2 + (int)((seed >> 16) % 2);
    float az0 = (float)((seed >> 8) % 360) * PI / 180.0f;
    for (int b = 0; b < laterals; b++) {
        float az = az0 + b * (2.0f * PI / laterals);
        glm::vec3 bdir = glm::normalize(glm::vec3(cosf(az) * 0.7f, 0.65f, sinf(az) * 0.7f));
        seed = seed * 1103515245 + 12345;
        buildFractalBranch(out, trunkTop, bdir, branchLen, branchThk,
                           fracDepth, fracDepth, seed);
    }
}

// Mirror of drawLargeTree(), emitting into `out` with the base at the origin.
void buildLargeTree(std::vector<Vertex>& out, unsigned int seed) {
    int trunkH = 7;
    for (int i = 0; i < trunkH; i++) {
        glm::vec3 p = glm::vec3(0, i * HEX_HEIGHT, 0);
        float v = 0.85f + 0.15f * hashNoise((int)seed, i);
        bakeHex(out, p, glm::vec3(0.0f, v, 0.0f), glm::vec3(0.7f, 1.0f, 0.7f));
        if (i < 3) {
            bakeHex(out, p + glm::vec3(0.35f, 0, 0.15f),  glm::vec3(0.0f, v, 0.0f), glm::vec3(0.35f, 0.7f, 0.35f));
            bakeHex(out, p + glm::vec3(-0.3f, 0, -0.2f),  glm::vec3(0.0f, v, 0.0f), glm::vec3(0.35f, 0.7f, 0.35f));
            bakeHex(out, p + glm::vec3(-0.05f, 0, 0.35f), glm::vec3(0.0f, v, 0.0f), glm::vec3(0.3f, 0.6f, 0.3f));
        }
    }

    glm::vec3 trunkTop = glm::vec3(0, trunkH * HEX_HEIGHT, 0);
    int   fracDepth = 5;
    float branchLen = 3.2f;
    float branchThk = 0.45f;

    buildFractalBranch(out, trunkTop, glm::vec3(0.0f, 1.0f, 0.0f), branchLen * 1.0f,
                       branchThk * 0.9f, fracDepth, fracDepth, seed * 13 + 3);

    seed = seed * 1103515245 + 12345;
    int laterals = 3 + (int)((seed >> 16) % 2);
    float az0 = (float)((seed >> 8) % 360) * PI / 180.0f;
    for (int b = 0; b < laterals; b++) {
        float az = az0 + b * (2.0f * PI / laterals);
        glm::vec3 bdir = glm::normalize(glm::vec3(cosf(az) * 0.75f, 0.6f, sinf(az) * 0.75f));
        seed = seed * 1103515245 + 12345;
        buildFractalBranch(out, trunkTop, bdir, branchLen, branchThk,
                           fracDepth, fracDepth, seed);
    }
}


// =====================================================
// Stone Ruins (archway structure)
// =====================================================
void drawRuins(glm::vec3 base) {
    glm::vec3 sc = COL_STONE_LIGHT;
    glm::vec3 sd = COL_STONE;
    glm::vec3 vine(0.15f, 0.5f, 0.1f);
    glm::vec3 vineDark(0.1f, 0.35f, 0.08f);

    // 12 pillars in a larger rectangle
    float px[] = {-6.0f, -2.0f, 2.0f, 6.0f, -6.0f, -2.0f, 2.0f, 6.0f, -6.0f, 6.0f, -6.0f, 6.0f};
    float pz[] = {-4.0f, -4.0f, -4.0f, -4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 0.0f, 0.0f, -2.0f, 2.0f};
    int pillarH[] = {10, 8, 9, 10, 7, 9, 8, 6, 8, 7, 9, 5};
    for (int p = 0; p < 12; p++) {
        for (int h = 0; h < pillarH[p]; h++) {
            glm::vec3 pos = base + glm::vec3(px[p], h * HEX_HEIGHT, pz[p]);
            drawHex(pos, (h % 2 == 0) ? sc : sd, glm::vec3(0.7f, 1.0f, 0.7f));
        }
        // Vine growth on many pillars
        if (p % 2 == 0) {
            for (int h = 0; h < pillarH[p] - 1; h += 2) {
                glm::vec3 vp = base + glm::vec3(px[p] + 0.4f, h * HEX_HEIGHT, pz[p] + 0.3f);
                drawHex(vp, (h % 4 == 0) ? vine : vineDark, glm::vec3(0.25f, 0.4f, 0.25f));
            }
            // Extra vine tendrils
            if (p % 4 == 0) {
                drawHex(base + glm::vec3(px[p] - 0.3f, 1.0f, pz[p]), vine, glm::vec3(0.2f, 0.3f, 0.2f));
                drawHex(base + glm::vec3(px[p], 2.5f, pz[p] - 0.35f), vineDark, glm::vec3(0.2f, 0.35f, 0.2f));
            }
        }
    }

    // Connecting beams on top (front and back rows)
    for (float x = -6.0f; x <= 6.0f; x += 0.8f) {
        unsigned int bs = gridSeed((int)(x * 10), 99);
        if (bs % 4 == 0) continue;
        drawHex(base + glm::vec3(x, 10.0f * HEX_HEIGHT, -4.0f), sc, glm::vec3(0.5f, 0.4f, 0.5f));
        if (bs % 3 != 0)
            drawHex(base + glm::vec3(x, 9.0f * HEX_HEIGHT, 4.0f), sd, glm::vec3(0.5f, 0.4f, 0.5f));
    }
    // Side beams
    for (float z = -4.0f; z <= 4.0f; z += 0.8f) {
        unsigned int bs = gridSeed(77, (int)(z * 10));
        if (bs % 3 == 0) continue;
        drawHex(base + glm::vec3(-6.0f, 8.0f * HEX_HEIGHT, z), sd, glm::vec3(0.5f, 0.4f, 0.5f));
        if (bs % 5 != 0)
            drawHex(base + glm::vec3(6.0f, 7.0f * HEX_HEIGHT, z), sc, glm::vec3(0.5f, 0.4f, 0.5f));
    }
    // Cross beams (partial roof)
    for (float x = -4.0f; x <= 4.0f; x += 1.2f) {
        unsigned int bs = gridSeed((int)(x * 10), 150);
        if (bs % 5 < 2) continue;
        drawHex(base + glm::vec3(x, 9.5f * HEX_HEIGHT, 0.0f), sc, glm::vec3(0.5f, 0.3f, 0.5f));
    }

    // Front archway (gap in middle for entrance)
    for (float x = -6.0f; x <= 6.0f; x += 0.8f) {
        if (fabsf(x) < 1.5f) continue;
        for (int h = 0; h < 5; h++) {
            unsigned int ws = gridSeed((int)(x * 10), h + 200);
            if (ws % 7 == 0) continue;
            drawHex(base + glm::vec3(x, h * HEX_HEIGHT, -4.0f), sd);
        }
    }
    // Arch keystone
    drawHex(base + glm::vec3(0, 5.0f * HEX_HEIGHT, -4.0f), sc, glm::vec3(1.6f, 0.5f, 0.5f));

    // Partial walls on both sides (taller)
    for (int h = 0; h < 5; h++) {
        for (float z = -4.0f; z <= 4.0f; z += 0.8f) {
            unsigned int ws = gridSeed((int)(z * 10) + 50, h);
            if (ws % 4 != 0)
                drawHex(base + glm::vec3(-6.0f, h * HEX_HEIGHT, z), sd);
            ws = gridSeed((int)(z * 10) + 80, h);
            if (ws % 3 != 0)
                drawHex(base + glm::vec3(6.0f, h * HEX_HEIGHT, z), sd);
        }
    }

    // Back wall (partial, taller)
    for (float x = -6.0f; x <= 6.0f; x += 0.8f) {
        for (int h = 0; h < 4; h++) {
            unsigned int ws = gridSeed((int)(x * 10) + 120, h);
            if (ws % 5 == 0) continue;
            drawHex(base + glm::vec3(x, h * HEX_HEIGHT, 4.0f), sd);
        }
    }

    // Interior altar with emissive ore
    drawHex(base + glm::vec3(0, 0, 0), sd, glm::vec3(1.2f, 0.3f, 1.2f));
    drawHex(base + glm::vec3(0, 0.35f, 0), sc, glm::vec3(0.8f, 0.2f, 0.8f));
    // Emissive diamond on altar (inline since drawHexEmissive defined later)
    setBool(shaderProgram, "isEmissive", true);
    setVec3(shaderProgram, "emissiveColor", COL_ORE_DIAMOND);
    drawHex(base + glm::vec3(0, 0.7f, 0), COL_ORE_DIAMOND, glm::vec3(0.35f, 0.35f, 0.35f));
    setBool(shaderProgram, "isEmissive", false);

    // Vine clusters on walls (more extensive)
    drawHex(base + glm::vec3(-5.5f, 1.0f, -4.0f), vine, glm::vec3(0.3f, 0.5f, 0.3f));
    drawHex(base + glm::vec3(-5.5f, 2.0f, -4.0f), vineDark, glm::vec3(0.25f, 0.4f, 0.25f));
    drawHex(base + glm::vec3(-5.5f, 3.0f, -3.5f), vine, glm::vec3(0.2f, 0.35f, 0.2f));
    drawHex(base + glm::vec3(5.5f, 0.5f, 4.0f), vine, glm::vec3(0.35f, 0.6f, 0.35f));
    drawHex(base + glm::vec3(5.5f, 1.5f, 4.0f), vineDark, glm::vec3(0.3f, 0.5f, 0.3f));
    drawHex(base + glm::vec3(5.5f, 2.5f, 3.5f), vine, glm::vec3(0.25f, 0.45f, 0.25f));
    drawHex(base + glm::vec3(-2.0f, 0.5f, 4.0f), vine, glm::vec3(0.3f, 0.5f, 0.3f));
    drawHex(base + glm::vec3(3.0f, 1.0f, -4.0f), vineDark, glm::vec3(0.25f, 0.4f, 0.25f));

    // Scattered rubble (more extensive)
    drawHex(base + glm::vec3(7.0f, 0, -1.0f), sd, glm::vec3(0.5f, 0.5f, 0.5f));
    drawHex(base + glm::vec3(7.5f, 0, 0.5f), sc, glm::vec3(0.4f, 0.4f, 0.4f));
    drawHex(base + glm::vec3(-7.0f, 0, 1.5f), sc, glm::vec3(0.4f, 0.4f, 0.4f));
    drawHex(base + glm::vec3(-7.5f, 0, -0.5f), sd, glm::vec3(0.45f, 0.45f, 0.45f));
    drawHex(base + glm::vec3(3.5f, 0, 5.5f), sd, glm::vec3(0.6f, 0.3f, 0.6f));
    drawHex(base + glm::vec3(-3.0f, 0, -5.5f), sc, glm::vec3(0.5f, 0.3f, 0.5f));
    drawHex(base + glm::vec3(0, 0, 6.0f), sd, glm::vec3(0.45f, 0.45f, 0.45f));
    drawHex(base + glm::vec3(-1.5f, 0, 5.5f), sc, glm::vec3(0.4f, 0.35f, 0.4f));
    // Fallen pillars (two of them)
    for (float x = 1.0f; x <= 4.0f; x += 0.8f) {
        drawHex(base + glm::vec3(x, 0.2f, 5.0f), sc, glm::vec3(0.6f, 0.6f, 0.6f));
    }
    for (float z = -2.0f; z <= 1.0f; z += 0.8f) {
        drawHex(base + glm::vec3(-7.5f, 0.2f, z), sd, glm::vec3(0.55f, 0.55f, 0.55f));
    }
}

// =====================================================
// Ambulance — Voxel model from hex blocks
// =====================================================
void drawAmbulance(glm::vec3 base, float angle = 0.0f) {
    glm::vec3 white(0.95f, 0.95f, 0.95f);
    glm::vec3 red(0.85f, 0.15f, 0.15f);
    glm::vec3 darkRed(0.6f, 0.08f, 0.08f);
    glm::vec3 gray(0.5f, 0.5f, 0.5f);
    glm::vec3 darkgray(0.2f, 0.2f, 0.2f);
    glm::vec3 black(0.08f, 0.08f, 0.08f);
    glm::vec3 chrome(0.7f, 0.72f, 0.75f);
    glm::vec3 glass(0.35f, 0.55f, 0.75f);
    glm::vec3 yellow(0.95f, 0.85f, 0.15f);

    float vSize = HEX_RADIUS * 0.32f; // Increased voxel scale to make it BIG
    
    // Voxel mapping function
    auto drawV = [&](int x, int y, int z, glm::vec3 col, bool emissive = false) {
        if (emissive) {
            setBool(shaderProgram, "isEmissive", true);
            setVec3(shaderProgram, "emissiveColor", col);
        }
        glm::mat4 M = glm::translate(glm::mat4(1.0f), base);
        M = glm::rotate(M, angle, glm::vec3(0, 1, 0));
        M = glm::translate(M, glm::vec3(x * vSize, y * vSize, z * vSize));
        M = glm::scale(M, glm::vec3(vSize / HEX_RADIUS * 0.62f, vSize / HEX_HEIGHT * 1.15f, vSize / HEX_RADIUS * 0.62f));
        setMat4(shaderProgram, "model", M);
        setVec3(shaderProgram, "objectColor", col);
        glBindVertexArray(hexVAO);
        glDrawArrays(GL_TRIANGLES, 0, hexVertexCount);
        if (emissive) {
            setBool(shaderProgram, "isEmissive", false);
        }
    };

    // Voxel bounds: 13x19x33 grid
    for (int y = 0; y <= 18; y++) {
        for (int x = -6; x <= 6; x++) {
            for (int z = 0; z <= 32; z++) {
                glm::vec3 c = black;
                bool draw = false;
                bool emissive = false;

                // --- Wheels ---
                bool inFrontWheel = (z >= 4 && z <= 8 && y <= 4 && (abs(x) == 6 || abs(x) == 5));
                bool inRearWheel = (z >= 22 && z <= 26 && y <= 4 && (abs(x) == 6 || abs(x) == 5));
                if (inFrontWheel || inRearWheel) {
                    if ((y == 0 || y == 4) && (z==4 || z==8 || z==22 || z==26)) continue; 
                    draw = true; c = black;
                    if ((y >= 1 && y <= 3) && (z >= 5 && z <= 7) && abs(x) == 6) c = chrome; // Hubcap
                }
                
                // --- Chassis ---
                else if (y == 2 && z >= 2 && z <= 30 && abs(x) <= 5) {
                    draw = true; c = darkgray;
                }

                // --- Front Bumper ---
                else if (y >= 2 && y <= 3 && z == 1 && abs(x) <= 6) {
                    draw = true; c = chrome;
                    if (y == 2 && abs(x) <= 2) { c = white; } // License plate
                }

                // --- Hood ---
                else if (y >= 3 && y <= 6 && z >= 2 && z <= 9 && abs(x) <= 5) {
                    draw = true; c = red;
                    if (z == 2 && y >= 3 && y <= 5 && abs(x) <= 2) c = darkgray; // Grille
                    if (z == 2 && y >= 3 && y <= 4 && abs(x) >= 3 && abs(x) <= 4) {
                        c = yellow; emissive = true; // Headlights
                    }
                }

                // --- Cab ---
                else if (y >= 3 && y <= 11 && z > 9 && z <= 13 && abs(x) <= 6) {
                    if (y <= 6) {
                        draw = true; c = red; 
                        if (y == 5 || y == 6) c = white; // Stripe wrap-around
                    }
                    else if (y >= 7) {
                        int slopeZ = 9 + (int)((y - 7) / 1.5f); 
                        if (z >= slopeZ) {
                            draw = true;
                            if (z == slopeZ && abs(x) <= 5) c = glass;
                            else if (abs(x) == 6) {
                                if (z > slopeZ && z <= 12) c = glass;
                                else c = white;
                            }
                            else c = white; 
                        }
                    }
                }
                
                // --- Mirrors ---
                else if (y >= 6 && y <= 8 && z == 12 && abs(x) == 7) {
                    draw = true; c = black;
                }

                // --- Rear Box ---
                else if (y >= 3 && y <= 16 && z > 13 && z <= 31 && abs(x) <= 6) {
                    draw = true;
                    c = red;
                    
                    int stripeY = 5;
                    if (z >= 26) stripeY = 5 + (z - 26); // Angle up diagonal
                    
                    if (y == stripeY || y == stripeY + 1) {
                        c = white;
                        if (z >= 18 && z <= 24 && abs(x) == 6 && (z % 2 == 0)) {
                            c = yellow; // MEDICAL letters
                        }
                    }
                    
                    // Side Window
                    if (y >= 9 && y <= 11 && z >= 15 && z <= 17 && abs(x) == 6) {
                        c = glass;
                    }
                    
                    // Warning lights 
                    if (y == 16 && abs(x) == 6) {
                        if (z % 3 == 0) { c = yellow; emissive = true; }
                        else if (z % 3 == 1) { c = red; emissive = true; }
                    }
                    if (y >= 4 && y <= 5 && z == 31 && abs(x) >= 4) {
                        c = darkRed; emissive = true; // Taillights
                    }
                }
                
                // --- Box Roof ---
                else if (y >= 17 && y <= 18 && z >= 12 && z <= 31 && abs(x) <= 6) {
                    draw = true; c = white;
                    // Front overhang
                    if (y == 17 && z == 12) {
                        if (abs(x) % 2 == 0) { c = red; emissive = true; }
                        else { c = yellow; emissive = true; }
                    }
                }

                // --- Optimization: Hollow Core ---
                // Only hollow out the inside of the rear box, keeping the cab and hood fully solid
                if (draw && abs(x) <= 4 && z >= 15 && z <= 30 && y >= 4 && y <= 15) {
                    draw = false; 
                }

                if (draw) {
                    drawV(x, y, z, c, emissive);
                }
            }
        }
    }

    // --- Side decal: ambu.png on both sides of the rear box ---
    if (texAmbu) {
        // Enable alpha blending for PNG transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texAmbu);
        setInt(shaderProgram, "texture1", 0);
        setInt(shaderProgram, "textureMode", 1); // texture-only mode

        // Panel dimensions in voxel units
        float panelW = 16.0f * vSize;  // width along ambulance length
        float panelH = 4.0f * vSize;   // height
        float panelZ = 22.0f * vSize;  // center Z (middle of rear box)
        float panelY = 10.0f * vSize;  // center Y (mid-height)
        float panelX = 6.4f * vSize;   // slightly outside body to avoid z-fighting

        // Right side (+X face)
        {
            glm::mat4 M = glm::translate(glm::mat4(1.0f), base);
            M = glm::rotate(M, angle, glm::vec3(0, 1, 0));
            M = glm::translate(M, glm::vec3(panelX, panelY, panelZ));
            M = myRotate(M, -PI / 2.0f, glm::vec3(0, 1, 0)); // face +X
            M = glm::scale(M, glm::vec3(panelW, panelH, 1.0f));
            setMat4(shaderProgram, "model", M);
            setVec3(shaderProgram, "objectColor", glm::vec3(1.0f));
            glBindVertexArray(panelVAO);
            glDrawArrays(GL_TRIANGLES, 0, panelVertCount);
        }

        // Left side (-X face)
        {
            glm::mat4 M = glm::translate(glm::mat4(1.0f), base);
            M = glm::rotate(M, angle, glm::vec3(0, 1, 0));
            M = glm::translate(M, glm::vec3(-panelX, panelY, panelZ));
            M = myRotate(M, PI / 2.0f, glm::vec3(0, 1, 0)); // face -X
            M = glm::scale(M, glm::vec3(panelW, panelH, 1.0f));
            setMat4(shaderProgram, "model", M);
            setVec3(shaderProgram, "objectColor", glm::vec3(1.0f));
            glBindVertexArray(panelVAO);
            glDrawArrays(GL_TRIANGLES, 0, panelVertCount);
        }

        // Reset state
        setInt(shaderProgram, "textureMode", 0);
        glDisable(GL_BLEND);
    }
}

// =====================================================
// Water hex drawing (transparent + wave animation)
// =====================================================
void drawHexWater(glm::vec3 pos, glm::vec3 color, float time) {
    float wave = sinf(pos.x * 2.0f + time * 1.5f) * 0.05f
               + sinf(pos.z * 2.5f + time * 1.2f) * 0.04f;
    glm::vec3 p = pos + glm::vec3(0, wave, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setFloat(shaderProgram, "alpha", 0.6f);
    drawHex(p, color);
    setFloat(shaderProgram, "alpha", 1.0f);
    glDisable(GL_BLEND);
}

// --- Deferred transparent water pass (19E-2) -------------------------------
// The terrain loop queues water blocks here instead of drawing them inline, and
// flushWaterPass() draws the lot once all the opaque geometry is down: sorted
// back-to-front, with ONE blend-state change for the whole batch instead of a
// glEnable/glDisable pair per block.
//
// Depth writes stay ON deliberately. With correct back-to-front ordering that
// still blends water over water properly, and it keeps water occluding whatever
// is drawn after it. Turning writes off would let later opaque geometry paint
// straight over the surface.
struct WaterDraw { glm::vec3 pos; glm::vec3 color; float distSq; };
std::vector<WaterDraw> waterDraws;

void flushWaterPass(float time) {
    if (waterDraws.empty()) return;

    std::sort(waterDraws.begin(), waterDraws.end(),
              [](const WaterDraw& a, const WaterDraw& b) { return a.distSq > b.distSq; });

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setFloat(shaderProgram, "alpha", 0.6f);

    // water_still.png is a vertical strip of WATER_FRAMES stacked 16x16 frames.
    // It used to be sampled whole, squashing all 32 frames onto every face.
    // Narrow the UV rect to one frame and walk it down the strip over time —
    // that is the pack's own animation, played at the speed it was drawn for.
    const int   WATER_FRAMES = 32;
    const float WATER_FPS    = 8.0f;
    int   frame = ((int)(time * WATER_FPS)) % WATER_FRAMES;
    float fh    = 1.0f / (float)WATER_FRAMES;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texWaterStill);
    setInt(shaderProgram, "tex", 0);
    setInt(shaderProgram, "textureMode", 2);   // texture * water tint
    setUVRect(shaderProgram, 1.0f, fh, 0.0f, frame * fh);

    for (const WaterDraw& w : waterDraws) {
        float wave = sinf(w.pos.x * 2.0f + time * 1.5f) * 0.05f
                   + sinf(w.pos.z * 2.5f + time * 1.2f) * 0.04f;
        drawHex(w.pos + glm::vec3(0, wave, 0), w.color);
    }

    resetUVRect(shaderProgram);
    setInt(shaderProgram, "textureMode", 0);
    setFloat(shaderProgram, "alpha", 1.0f);
    glDisable(GL_BLEND);
    waterDraws.clear();
}

// =====================================================
// Emissive hex drawing
// =====================================================
void drawHexEmissive(glm::vec3 pos, glm::vec3 emitColor, glm::vec3 scale = glm::vec3(1.0f)) {
    setBool(shaderProgram, "isEmissive", true);
    setVec3(shaderProgram, "emissiveColor", emitColor);
    drawHex(pos, emitColor, scale);
    setBool(shaderProgram, "isEmissive", false);
}

// =====================================================
// Torch positions (collected during terrain build)
// =====================================================
// PointLightSrc carries a colour, not just a position. Every point light used to
// be uploaded as the same hardcoded orange in main.cpp, so a blue-white lantern
// and an orange torch lit the room identically.
std::vector<PointLightSrc> torchPositions;
const glm::vec3 COL_TORCH_FLAME(1.0f, 0.6f, 0.1f);
// A hearth burns redder than a torch — more ember, less flame.
const glm::vec3 COL_FIRE_HEARTH(1.0f, 0.42f, 0.10f);

// Geometry only — does NOT register the point light. Use when the caller has
// already pushed the light position (so off-screen torches keep lighting the scene).
void drawTorchMesh(glm::vec3 base) {
    drawHex(base + glm::vec3(0, 0.3f, 0), COL_WOOD, glm::vec3(0.15f, 0.6f, 0.15f));
    drawHexEmissive(base + glm::vec3(0, 0.7f, 0), COL_TORCH_FLAME, glm::vec3(0.2f, 0.25f, 0.2f));
}

void drawTorch(glm::vec3 base) {
    drawTorchMesh(base);
    torchPositions.push_back({ base + glm::vec3(0, 0.8f, 0), COL_TORCH_FLAME });
}

// =====================================================
// Check if position is in forest zone (Zone B)
// =====================================================
bool isForestZone(int col, int row) {
    return (col >= -60 && col <= -25 && row >= 20 && row <= 55);
}

// Check if position is a water pond center — returns 0=no, 1=water, 2=beach
int isWaterPond(int col, int row) {
    // Pond definitions: {centerCol, centerRow, waterRadius, beachRadius}
    struct PondDef { int cx, cy, wr, br; };
    PondDef ponds[] = {
        {35, -10, 15, 18},   // Pond 1: large lake southeast
        {-8, 3, 8, 11},     // Pond 2: south of castle
        {-40, 35, 10, 13},  // Pond 3: forest pond
        {-30, -15, 12, 15}, // Pond 4: desert oasis
        {30, -25, 10, 13},  // Pond 5: mountain lake
    };
    for (auto& p : ponds) {
        int d = (col - p.cx) * (col - p.cx) + (row - p.cy) * (row - p.cy);
        if (d <= p.wr) return 1; // water
        if (d <= p.br) return 2; // beach (sand border)
    }
    return 0;
}

// =====================================================
// Initialize block grid from procedural generation
// =====================================================
const int TERRAIN_MIN = -90, TERRAIN_MAX = 90;

// Store tree/torch locations for rendering non-grid objects
// `variant` selects which baked mesh (19B) this tree draws — assigned from the
// placement seed so the forest stays varied without a mesh per tree.
// `groundH` is the index of the block the tree stands ON, recorded at world-gen.
// The draw loop used to rescan the whole 32-block column top-down every frame to
// work this out, which is both wasted work and wrong: it finds the topmost
// non-air block, so anything built or floating above the tree lifts it into the
// sky. -1 means "not resolved yet"; resolveGroundHeights() fills those in.
struct TreeInfo { int col, row; glm::vec3 leafColor; bool large; int variant = 0; int groundH = -1; };
std::vector<TreeInfo> treeLocations;

// --- Baked mesh pool -------------------------------------------------------
struct TreeMesh { GLuint vao = 0, vbo = 0; int count = 0; };
static TreeMesh g_treeMeshes[NUM_TREE_VARIANTS];
static TreeMesh g_largeTreeMeshes[NUM_LARGE_TREE_VARIANTS];
static bool g_treeMeshesReady = false;

static void uploadTreeMesh(TreeMesh& m, const std::vector<Vertex>& verts) {
    m.count = (int)verts.size();
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
    glBindVertexArray(0);
}

// Bake the tree variants once. Call after initBlockGrid(), with a GL context.
// Only variants the world actually placed get baked — a large-tree mesh is ~5 MB,
// so baking all of them regardless would waste most of the VRAM.
void initTreeMeshes() {
    bool usedSmall[NUM_TREE_VARIANTS]       = {false};
    bool usedLarge[NUM_LARGE_TREE_VARIANTS] = {false};
    int smallTrees = 0, largeTrees = 0;

    for (const TreeInfo& ti : treeLocations) {
        if (ti.large) { usedLarge[ti.variant % NUM_LARGE_TREE_VARIANTS] = true; largeTrees++; }
        else          { usedSmall[ti.variant % NUM_TREE_VARIANTS]       = true; smallTrees++; }
    }

    size_t totalVerts = 0;
    int bakedSmall = 0, bakedLarge = 0;
    std::vector<Vertex> verts;

    for (int i = 0; i < NUM_TREE_VARIANTS; i++) {
        if (!usedSmall[i]) continue;
        verts.clear();
        buildTree(verts, gridSeed(i * 977 + 13, i * 613 + 7));
        uploadTreeMesh(g_treeMeshes[i], verts);
        totalVerts += verts.size();
        bakedSmall++;
    }
    for (int i = 0; i < NUM_LARGE_TREE_VARIANTS; i++) {
        if (!usedLarge[i]) continue;
        verts.clear();
        buildLargeTree(verts, gridSeed(i * 1543 + 29, i * 811 + 17));
        uploadTreeMesh(g_largeTreeMeshes[i], verts);
        totalVerts += verts.size();
        bakedLarge++;
    }

    g_treeMeshesReady = true;
    printf("[Trees] %d trees (%d small, %d large) -> %d small + %d large baked meshes\n",
           smallTrees + largeTrees, smallTrees, largeTrees, bakedSmall, bakedLarge);
    printf("[Trees] %zu verts, %.1f MB VRAM (was ~%d draw calls/frame, now 1 per tree)\n",
           totalVerts, totalVerts * sizeof(Vertex) / (1024.0 * 1024.0),
           smallTrees * 380 + largeTrees * 1651);
}

void destroyTreeMeshes() {
    for (int i = 0; i < NUM_TREE_VARIANTS; i++) {
        glDeleteVertexArrays(1, &g_treeMeshes[i].vao);
        glDeleteBuffers(1, &g_treeMeshes[i].vbo);
    }
    for (int i = 0; i < NUM_LARGE_TREE_VARIANTS; i++) {
        glDeleteVertexArrays(1, &g_largeTreeMeshes[i].vao);
        glDeleteBuffers(1, &g_largeTreeMeshes[i].vbo);
    }
}
// `height` is the index of the block the torch stands ON — same meaning as
// TreeInfo::groundH. It was already being recorded and then ignored by the draw
// loop, which rescanned the column instead. See resolveGroundHeights().
struct TorchInfo { int col, row, height; };
std::vector<TorchInfo> torchLocations;

// =========================================================
// MEDIEVAL CASTLE (Main Spawn Base) — matching reference images
// Stone walls, battlements, corner towers with spires,
// main keep with peaked wooden roof, front gate with arch
// =========================================================
void buildMedievalCastle(int c0, int r0) {
    int W = 40, D = 36;
    int c1 = c0 + W - 1;
    int r1 = r0 + D - 1;
    int G = UNDERGROUND_DEPTH; // ground level

    auto fillRect = [&](int cx0, int cx1, int rx0, int rx1, int h0, int h1, int type) {
        for (int c = cx0; c <= cx1; c++)
            for (int r = rx0; r <= rx1; r++)
                for (int h = h0; h <= h1; h++)
                    setBlock(c, r, h, type);
    };

    // 1. Clear plot + foundation
    for (int c = c0 - 2; c <= c1 + 2; c++) {
        for (int r = r0 - 2; r <= r1 + 2; r++) {
            for (int h = G + 1; h < GRID_H; h++) setBlock(c, r, h, BLOCK_AIR);
            setBlock(c, r, 0, BLOCK_BEDROCK);
            for (int h = 1; h <= G; h++) setBlock(c, r, h, BLOCK_STONE);
        }
    }
    // Grass courtyard floor
    for (int c = c0 + 1; c < c1; c++)
        for (int r = r0 + 1; r < r1; r++)
            setBlock(c, r, G, BLOCK_GRASS);
    // Stone path in courtyard
    for (int r = r1 - 8; r <= r1; r++) {
        setBlock(c0 + W/2 - 1, r, G, BLOCK_STONE);
        setBlock(c0 + W/2, r, G, BLOCK_STONE);
        setBlock(c0 + W/2 + 1, r, G, BLOCK_STONE_LIGHT);
    }

    // Clear trees/torches in area
    for (int i = (int)treeLocations.size() - 1; i >= 0; i--) {
        if (treeLocations[i].col >= c0 - 2 && treeLocations[i].col <= c1 + 2 &&
            treeLocations[i].row >= r0 - 2 && treeLocations[i].row <= r1 + 2) {
            treeLocations.erase(treeLocations.begin() + i);
        }
    }
    for (int i = (int)torchLocations.size() - 1; i >= 0; i--) {
        if (torchLocations[i].col >= c0 - 2 && torchLocations[i].col <= c1 + 2 &&
            torchLocations[i].row >= r0 - 2 && torchLocations[i].row <= r1 + 2) {
            torchLocations.erase(torchLocations.begin() + i);
        }
    }

    // ============================================
    // 2. OUTER WALLS (stone, 7 blocks high)
    // ============================================
    int wallH = 7;
    // Back wall (north)
    fillRect(c0, c1, r0, r0, G+1, G+wallH, BLOCK_STONE);
    // Front wall (south) — with gate gap
    for (int c = c0; c <= c1; c++) {
        if (c >= c0 + W/2 - 2 && c <= c0 + W/2 + 1) continue; // gate opening
        fillRect(c, c, r1, r1, G+1, G+wallH, BLOCK_STONE);
    }
    // Left wall (west)
    fillRect(c0, c0, r0, r1, G+1, G+wallH, BLOCK_STONE);
    // Right wall (east)
    fillRect(c1, c1, r0, r1, G+1, G+wallH, BLOCK_STONE);

    // Battlements (crenellations) on all walls — every other block is 1 higher
    for (int c = c0; c <= c1; c++) {
        if (c % 2 == 0) {
            if (!(c >= c0 + W/2 - 2 && c <= c0 + W/2 + 1)) // skip gate
                setBlock(c, r1, G+wallH+1, BLOCK_STONE);
            setBlock(c, r0, G+wallH+1, BLOCK_STONE);
        }
    }
    for (int r = r0; r <= r1; r++) {
        if (r % 2 == 0) {
            setBlock(c0, r, G+wallH+1, BLOCK_STONE);
            setBlock(c1, r, G+wallH+1, BLOCK_STONE);
        }
    }

    // Wall walkway (wood plank path on top of walls, inside edge)
    fillRect(c0+1, c1-1, r0+1, r0+1, G+wallH, G+wallH, BLOCK_WOOD);
    fillRect(c0+1, c1-1, r1-1, r1-1, G+wallH, G+wallH, BLOCK_WOOD);
    fillRect(c0+1, c0+1, r0+1, r1-1, G+wallH, G+wallH, BLOCK_WOOD);
    fillRect(c1-1, c1-1, r0+1, r1-1, G+wallH, G+wallH, BLOCK_WOOD);

    // Stone accent stripe at mid-height on walls (like cobblestone wall detail)
    for (int c = c0; c <= c1; c++) {
        setBlock(c, r0, G+4, BLOCK_STONE_LIGHT);
        if (!(c >= c0 + W/2 - 2 && c <= c0 + W/2 + 1))
            setBlock(c, r1, G+4, BLOCK_STONE_LIGHT);
    }
    for (int r = r0; r <= r1; r++) {
        setBlock(c0, r, G+4, BLOCK_STONE_LIGHT);
        setBlock(c1, r, G+4, BLOCK_STONE_LIGHT);
    }

    // ============================================
    // 3. CORNER TOWERS (4 towers, 14 high + spire)
    // ============================================
    int twrH = 14;
    int twrSize = 3; // 3x3 footprint
    int towers[4][2] = {
        {c0, r0},           // NW
        {c1-2, r0},         // NE
        {c0, r1-2},         // SW
        {c1-2, r1-2}        // SE
    };
    for (int t = 0; t < 4; t++) {
        int tc = towers[t][0], tr = towers[t][1];
        // Tower body (stone walls, hollow inside)
        fillRect(tc, tc+2, tr, tr+2, G+1, G+twrH, BLOCK_STONE);
        fillRect(tc+1, tc+1, tr+1, tr+1, G+1, G+twrH-1, BLOCK_AIR); // hollow core

        // Darker stone band at intervals
        for (int h = G+3; h <= G+twrH; h += 4) {
            fillRect(tc, tc+2, tr, tr+2, h, h, BLOCK_COAL_ORE);
            setBlock(tc+1, tr+1, h, BLOCK_AIR); // keep hollow
        }

        // Tower battlements
        setBlock(tc, tr, G+twrH+1, BLOCK_STONE);
        setBlock(tc+2, tr, G+twrH+1, BLOCK_STONE);
        setBlock(tc, tr+2, G+twrH+1, BLOCK_STONE);
        setBlock(tc+2, tr+2, G+twrH+1, BLOCK_STONE);

        // Pointed spire (wooden, pyramid shape)
        // Layer 1: 3x3 wood base
        fillRect(tc, tc+2, tr, tr+2, G+twrH+2, G+twrH+2, BLOCK_WOOD);
        // Layer 2: cross shape
        setBlock(tc+1, tr, G+twrH+3, BLOCK_WOOD);
        setBlock(tc+1, tr+2, G+twrH+3, BLOCK_WOOD);
        setBlock(tc, tr+1, G+twrH+3, BLOCK_WOOD);
        setBlock(tc+2, tr+1, G+twrH+3, BLOCK_WOOD);
        setBlock(tc+1, tr+1, G+twrH+3, BLOCK_WOOD);
        // Layer 3: single peak
        setBlock(tc+1, tr+1, G+twrH+4, BLOCK_WOOD);
        setBlock(tc+1, tr+1, G+twrH+5, BLOCK_WOOD);

        // Arrow slit windows on towers
        for (int h = G+3; h <= G+twrH-2; h += 3) {
            setBlock(tc+1, tr, h, BLOCK_GLASS);     // front
            setBlock(tc+1, tr+2, h, BLOCK_GLASS);   // back
            setBlock(tc, tr+1, h, BLOCK_GLASS);     // left
            setBlock(tc+2, tr+1, h, BLOCK_GLASS);   // right
        }
    }

    // ============================================
    // 4. FRONT GATE with arch + flanking gate towers
    // ============================================
    int gateL = c0 + W/2 - 3; // left gate tower
    int gateR = c0 + W/2 + 2; // right gate tower
    int gateH = 10;

    // Left gate pillar
    fillRect(gateL, gateL+1, r1-1, r1, G+1, G+gateH, BLOCK_STONE);
    // Right gate pillar
    fillRect(gateR, gateR+1, r1-1, r1, G+1, G+gateH, BLOCK_STONE);

    // Dark stone accent on gate pillars
    fillRect(gateL, gateL+1, r1, r1, G+3, G+3, BLOCK_COAL_ORE);
    fillRect(gateL, gateL+1, r1, r1, G+6, G+6, BLOCK_COAL_ORE);
    fillRect(gateR, gateR+1, r1, r1, G+3, G+3, BLOCK_COAL_ORE);
    fillRect(gateR, gateR+1, r1, r1, G+6, G+6, BLOCK_COAL_ORE);

    // Arch over gate (stone bridge connecting the two pillars)
    fillRect(gateL, gateR+1, r1, r1, G+wallH, G+wallH, BLOCK_STONE);
    fillRect(gateL+1, gateR, r1, r1, G+wallH+1, G+wallH+1, BLOCK_STONE_LIGHT); // arch top detail

    // Gate pillar caps (small wooden spire on each)
    setBlock(gateL, r1, G+gateH+1, BLOCK_WOOD);
    setBlock(gateL+1, r1, G+gateH+1, BLOCK_WOOD);
    setBlock(gateR, r1, G+gateH+1, BLOCK_WOOD);
    setBlock(gateR+1, r1, G+gateH+1, BLOCK_WOOD);
    setBlock(gateL, r1, G+gateH+2, BLOCK_WOOD);
    setBlock(gateR+1, r1, G+gateH+2, BLOCK_WOOD);

    // Wooden portcullis frame (dark wood around gate opening)
    int gOpen0 = c0 + W/2 - 2, gOpen1 = c0 + W/2 + 1;
    fillRect(gOpen0, gOpen1, r1, r1, G+wallH-1, G+wallH-1, BLOCK_WOOD); // top beam
    setBlock(gOpen0, r1, G+1, BLOCK_WOOD); // left post base
    setBlock(gOpen1, r1, G+1, BLOCK_WOOD); // right post base

    // ============================================
    // 5. MAIN KEEP (large central building)
    // ============================================
    int keepC0 = c0 + 6, keepC1 = c1 - 6;   // keep cols
    int keepR0 = r0 + 4, keepR1 = r0 + 18;   // keep rows (back portion)
    int keepWallH = 10;
    int keepW = keepC1 - keepC0 + 1;

    // Keep walls (stone)
    fillRect(keepC0, keepC1, keepR0, keepR0, G+1, G+keepWallH, BLOCK_STONE); // back
    fillRect(keepC0, keepC1, keepR1, keepR1, G+1, G+keepWallH, BLOCK_STONE); // front
    fillRect(keepC0, keepC0, keepR0, keepR1, G+1, G+keepWallH, BLOCK_STONE); // left
    fillRect(keepC1, keepC1, keepR0, keepR1, G+1, G+keepWallH, BLOCK_STONE); // right
    // Floor
    fillRect(keepC0+1, keepC1-1, keepR0+1, keepR1-1, G, G, BLOCK_WOOD);
    // Second floor
    fillRect(keepC0+1, keepC1-1, keepR0+1, keepR1-1, G+5, G+5, BLOCK_WOOD);

    // Keep interior hollow
    fillRect(keepC0+1, keepC1-1, keepR0+1, keepR1-1, G+1, G+keepWallH-1, BLOCK_AIR);

    // Dark stone accent bands on keep walls
    fillRect(keepC0, keepC1, keepR0, keepR0, G+3, G+3, BLOCK_COAL_ORE);
    fillRect(keepC0, keepC1, keepR1, keepR1, G+3, G+3, BLOCK_COAL_ORE);
    fillRect(keepC0, keepC0, keepR0, keepR1, G+3, G+3, BLOCK_COAL_ORE);
    fillRect(keepC1, keepC1, keepR0, keepR1, G+3, G+3, BLOCK_COAL_ORE);
    fillRect(keepC0, keepC1, keepR0, keepR0, G+7, G+7, BLOCK_COAL_ORE);
    fillRect(keepC0, keepC1, keepR1, keepR1, G+7, G+7, BLOCK_COAL_ORE);
    fillRect(keepC0, keepC0, keepR0, keepR1, G+7, G+7, BLOCK_COAL_ORE);
    fillRect(keepC1, keepC1, keepR0, keepR1, G+7, G+7, BLOCK_COAL_ORE);

    // Windows on keep (glass panes at regular intervals)
    for (int c = keepC0 + 2; c <= keepC1 - 2; c += 3) {
        // Front and back wall windows
        setBlock(c, keepR0, G+5, BLOCK_GLASS);
        setBlock(c, keepR0, G+6, BLOCK_GLASS);
        setBlock(c, keepR0, G+8, BLOCK_GLASS);
        setBlock(c, keepR0, G+9, BLOCK_GLASS);
        setBlock(c, keepR1, G+5, BLOCK_GLASS);
        setBlock(c, keepR1, G+6, BLOCK_GLASS);
        setBlock(c, keepR1, G+8, BLOCK_GLASS);
        setBlock(c, keepR1, G+9, BLOCK_GLASS);
    }
    for (int r = keepR0 + 2; r <= keepR1 - 2; r += 3) {
        // Side wall windows
        setBlock(keepC0, r, G+5, BLOCK_GLASS);
        setBlock(keepC0, r, G+6, BLOCK_GLASS);
        setBlock(keepC0, r, G+8, BLOCK_GLASS);
        setBlock(keepC0, r, G+9, BLOCK_GLASS);
        setBlock(keepC1, r, G+5, BLOCK_GLASS);
        setBlock(keepC1, r, G+6, BLOCK_GLASS);
        setBlock(keepC1, r, G+8, BLOCK_GLASS);
        setBlock(keepC1, r, G+9, BLOCK_GLASS);
    }

    // Keep entrance (front center door)
    int keepMid = keepC0 + keepW / 2;
    setBlock(keepMid, keepR1, G+1, BLOCK_AIR);
    setBlock(keepMid, keepR1, G+2, BLOCK_AIR);
    setBlock(keepMid, keepR1, G+3, BLOCK_AIR);
    setBlock(keepMid-1, keepR1, G+1, BLOCK_AIR);
    setBlock(keepMid-1, keepR1, G+2, BLOCK_AIR);
    setBlock(keepMid-1, keepR1, G+3, BLOCK_AIR);
    // Wood door frame
    setBlock(keepMid-2, keepR1, G+1, BLOCK_WOOD);
    setBlock(keepMid-2, keepR1, G+2, BLOCK_WOOD);
    setBlock(keepMid-2, keepR1, G+3, BLOCK_WOOD);
    setBlock(keepMid+1, keepR1, G+1, BLOCK_WOOD);
    setBlock(keepMid+1, keepR1, G+2, BLOCK_WOOD);
    setBlock(keepMid+1, keepR1, G+3, BLOCK_WOOD);
    setBlock(keepMid-1, keepR1, G+4, BLOCK_WOOD); // arch
    setBlock(keepMid, keepR1, G+4, BLOCK_WOOD);

    // ============================================
    // 5b. PEAKED WOODEN ROOF on main keep
    // ============================================
    // Ridge runs along the col axis (east-west)
    // Roof slopes from front/back walls up to center ridge
    int roofBase = G + keepWallH + 1;
    int keepRmid = (keepR0 + keepR1) / 2;
    int halfDepth = keepR1 - keepRmid;

    for (int c = keepC0 - 1; c <= keepC1 + 1; c++) {
        for (int r = keepR0 - 1; r <= keepR1 + 1; r++) {
            int distFromCenter = abs(r - keepRmid);
            int roofH = roofBase + (halfDepth + 1 - distFromCenter);
            if (roofH > roofBase) {
                setBlock(c, r, roofH, BLOCK_WOOD);
                // Fill gable ends with stone
                if (c == keepC0 - 1 || c == keepC1 + 1) {
                    for (int h = roofBase; h < roofH; h++)
                        setBlock(c, r, h, BLOCK_STONE);
                }
            }
        }
    }
    // Ridge beam (dark wood along the peak)
    for (int c = keepC0 - 1; c <= keepC1 + 1; c++) {
        setBlock(c, keepRmid, roofBase + halfDepth + 1, BLOCK_COAL_ORE);
    }

    // Wooden cross-beams visible on gable ends
    for (int h = roofBase; h <= roofBase + halfDepth; h += 2) {
        setBlock(keepC0 - 1, keepRmid, h, BLOCK_WOOD);
        setBlock(keepC1 + 1, keepRmid, h, BLOCK_WOOD);
    }

    // ============================================
    // 6. SIDE BUILDING (smaller, right side)
    // ============================================
    int sideC0 = keepC1 + 1, sideC1 = c1 - 2;
    int sideR0 = r0 + 8, sideR1 = r0 + 18;
    int sideWallH = 7;

    // Side building walls
    fillRect(sideC0, sideC1, sideR0, sideR0, G+1, G+sideWallH, BLOCK_STONE); // back
    fillRect(sideC0, sideC1, sideR1, sideR1, G+1, G+sideWallH, BLOCK_STONE); // front
    fillRect(sideC1, sideC1, sideR0, sideR1, G+1, G+sideWallH, BLOCK_STONE); // right side
    // Left side connects to keep (shared wall, make doorway)
    fillRect(sideC0, sideC0, sideR0, sideR1, G+1, G+sideWallH, BLOCK_STONE);
    // Interior hollow
    fillRect(sideC0+1, sideC1-1, sideR0+1, sideR1-1, G+1, G+sideWallH-1, BLOCK_AIR);
    // Floor
    fillRect(sideC0+1, sideC1-1, sideR0+1, sideR1-1, G, G, BLOCK_WOOD);

    // Doorway connecting keep to side building
    int sideDoor = (sideR0 + sideR1) / 2;
    setBlock(sideC0, sideDoor, G+1, BLOCK_AIR);
    setBlock(sideC0, sideDoor, G+2, BLOCK_AIR);
    setBlock(sideC0, sideDoor, G+3, BLOCK_AIR);

    // Side building windows
    for (int r = sideR0 + 2; r <= sideR1 - 2; r += 3) {
        setBlock(sideC1, r, G+3, BLOCK_GLASS);
        setBlock(sideC1, r, G+4, BLOCK_GLASS);
        setBlock(sideC1, r, G+5, BLOCK_GLASS);
    }
    for (int c = sideC0 + 2; c <= sideC1 - 2; c += 3) {
        setBlock(c, sideR0, G+3, BLOCK_GLASS);
        setBlock(c, sideR0, G+4, BLOCK_GLASS);
        setBlock(c, sideR1, G+3, BLOCK_GLASS);
        setBlock(c, sideR1, G+4, BLOCK_GLASS);
    }

    // Dark accent band
    fillRect(sideC0, sideC1, sideR0, sideR0, G+2, G+2, BLOCK_COAL_ORE);
    fillRect(sideC0, sideC1, sideR1, sideR1, G+2, G+2, BLOCK_COAL_ORE);
    fillRect(sideC1, sideC1, sideR0, sideR1, G+2, G+2, BLOCK_COAL_ORE);

    // Side building peaked roof
    int sideRoofBase = G + sideWallH + 1;
    int sideRmid = (sideR0 + sideR1) / 2;
    int sideHalfD = sideR1 - sideRmid;
    for (int c = sideC0 - 1; c <= sideC1 + 1; c++) {
        for (int r = sideR0 - 1; r <= sideR1 + 1; r++) {
            int dist = abs(r - sideRmid);
            int rh = sideRoofBase + (sideHalfD + 1 - dist);
            if (rh > sideRoofBase) {
                setBlock(c, r, rh, BLOCK_WOOD);
                // Gable ends
                if (c == sideC0 - 1 || c == sideC1 + 1) {
                    for (int h = sideRoofBase; h < rh; h++)
                        setBlock(c, r, h, BLOCK_STONE);
                }
            }
        }
    }

    // ============================================
    // 7. LEFT WING BUILDING (kitchen/armory)
    // ============================================
    int leftC0 = c0 + 2, leftC1 = keepC0 - 1;
    int leftR0 = r0 + 8, leftR1 = r0 + 16;
    int leftWallH = 6;

    fillRect(leftC0, leftC1, leftR0, leftR0, G+1, G+leftWallH, BLOCK_STONE);
    fillRect(leftC0, leftC1, leftR1, leftR1, G+1, G+leftWallH, BLOCK_STONE);
    fillRect(leftC0, leftC0, leftR0, leftR1, G+1, G+leftWallH, BLOCK_STONE);
    fillRect(leftC1, leftC1, leftR0, leftR1, G+1, G+leftWallH, BLOCK_STONE);
    fillRect(leftC0+1, leftC1-1, leftR0+1, leftR1-1, G+1, G+leftWallH-1, BLOCK_AIR);
    fillRect(leftC0+1, leftC1-1, leftR0+1, leftR1-1, G, G, BLOCK_WOOD);

    // Doorway to courtyard
    setBlock(leftC1, (leftR0+leftR1)/2, G+1, BLOCK_AIR);
    setBlock(leftC1, (leftR0+leftR1)/2, G+2, BLOCK_AIR);
    setBlock(leftC1, (leftR0+leftR1)/2, G+3, BLOCK_AIR);

    // Windows
    for (int r = leftR0 + 2; r <= leftR1 - 2; r += 3) {
        setBlock(leftC0, r, G+3, BLOCK_GLASS);
        setBlock(leftC0, r, G+4, BLOCK_GLASS);
    }

    // Flat roof with battlements
    fillRect(leftC0, leftC1, leftR0, leftR1, G+leftWallH+1, G+leftWallH+1, BLOCK_STONE);
    for (int c = leftC0; c <= leftC1; c += 2)
        setBlock(c, leftR0, G+leftWallH+2, BLOCK_STONE);
    for (int c = leftC0; c <= leftC1; c += 2)
        setBlock(c, leftR1, G+leftWallH+2, BLOCK_STONE);

    // ============================================
    // 8. WOODEN BALCONY on keep front (like reference)
    // ============================================
    int balcC0 = keepC0 + 2, balcC1 = keepC1 - 2;
    int balcR = keepR1 + 1;
    int balcH = G + 6; // second floor level
    // Wooden platform
    fillRect(balcC0, balcC1, balcR, balcR+1, balcH, balcH, BLOCK_WOOD);
    // Wooden fence railing
    for (int c = balcC0; c <= balcC1; c += 2) {
        setBlock(c, balcR+1, balcH+1, BLOCK_WOOD);
    }
    // Support brackets under balcony
    for (int c = balcC0; c <= balcC1; c += 4) {
        setBlock(c, keepR1, balcH-1, BLOCK_WOOD);
        setBlock(c, balcR, balcH-1, BLOCK_WOOD);
    }

    // ============================================
    // 9. INTERIOR DETAILS
    // ============================================
    // Great hall table (wood)
    fillRect(keepMid-3, keepMid+2, keepR0+3, keepR0+5, G+1, G+1, BLOCK_WOOD);

    // Throne at back of keep
    setBlock(keepMid, keepR0+1, G+1, BLOCK_STONE_LIGHT);
    setBlock(keepMid, keepR0+1, G+2, BLOCK_STONE_LIGHT);
    setBlock(keepMid-1, keepR0+1, G+1, BLOCK_WOOD);
    setBlock(keepMid+1, keepR0+1, G+1, BLOCK_WOOD);

    // Stairs to second floor (along left wall inside keep)
    for (int i = 0; i < 5; i++) {
        setBlock(keepC0+1, keepR0+2+i, G+1+i, BLOCK_STONE);
    }

    // Interior torches in keep
    torchLocations.push_back({keepC0+2, keepR0+2, G+4});
    torchLocations.push_back({keepC1-2, keepR0+2, G+4});
    torchLocations.push_back({keepC0+2, keepR1-2, G+4});
    torchLocations.push_back({keepC1-2, keepR1-2, G+4});

    // ============================================
    // 10. EXTERIOR DETAILS
    // ============================================
    // Torches on outer walls (every 8 blocks)
    for (int c = c0 + 4; c <= c1 - 4; c += 8) {
        torchLocations.push_back({c, r1, G+wallH-1});
        torchLocations.push_back({c, r0, G+wallH-1});
    }
    for (int r = r0 + 4; r <= r1 - 4; r += 8) {
        torchLocations.push_back({c0, r, G+wallH-1});
        torchLocations.push_back({c1, r, G+wallH-1});
    }

    // Gate torches (flanking the entrance)
    torchLocations.push_back({gateL, r1+1, G+wallH});
    torchLocations.push_back({gateR+1, r1+1, G+wallH});

    // Flower boxes / bushes near gate (leaf blocks)
    setBlock(gateL-1, r1+1, G+1, BLOCK_LEAF);
    setBlock(gateR+2, r1+1, G+1, BLOCK_LEAF);
    setBlock(gateL-1, r1+2, G+1, BLOCK_LEAF);
    setBlock(gateR+2, r1+2, G+1, BLOCK_LEAF);

    // Stone path leading from gate outward
    for (int r = r1 + 1; r <= r1 + 8; r++) {
        setBlock(c0 + W/2 - 1, r, G, BLOCK_STONE);
        setBlock(c0 + W/2, r, G, BLOCK_STONE);
        setBlock(c0 + W/2 + 1, r, G, BLOCK_STONE_LIGHT);
    }

    // Well in courtyard (stone ring + water)
    int wellC = c0 + W/2 + 6, wellR = r1 - 8;
    for (int dc = -1; dc <= 1; dc++) {
        for (int dr = -1; dr <= 1; dr++) {
            if (dc == 0 && dr == 0) {
                setBlock(wellC, wellR, G+1, BLOCK_WATER);
            } else {
                setBlock(wellC+dc, wellR+dr, G+1, BLOCK_STONE);
            }
        }
    }
    // Well posts and roof
    setBlock(wellC-1, wellR-1, G+2, BLOCK_WOOD);
    setBlock(wellC-1, wellR-1, G+3, BLOCK_WOOD);
    setBlock(wellC+1, wellR+1, G+2, BLOCK_WOOD);
    setBlock(wellC+1, wellR+1, G+3, BLOCK_WOOD);
    fillRect(wellC-1, wellC+1, wellR-1, wellR+1, G+4, G+4, BLOCK_WOOD);

    // Courtyard trees (2 decorative trees)
    treeLocations.push_back({c0+W/2 - 8, r1-10, glm::vec3(0.2f,0.65f,0.15f), false, 3});
    treeLocations.push_back({c0+W/2 + 8, r1-12, glm::vec3(0.15f,0.55f,0.12f), false, 11});

    printf("[Castle] Built Medieval Castle at col %d..%d, row %d..%d\n", c0, c1, r0, r1);
}

// =========================================================
// PHASE 16: STRUCTURE GENERATORS
// =========================================================

void buildDungeon(int c0, int r0, int groundH) {
    int W = 9, D = 9;
    int c1 = c0 + W - 1;
    int r1 = r0 + D - 1;
    // Carve room
    for (int c = c0; c <= c1; c++) {
        for (int r = r0; r <= r1; r++) {
            for (int h = groundH; h < groundH + 5; h++) {
                if (c==c0 || c==c1 || r==r0 || r==r1 || h==groundH || h==groundH+4) {
                    setBlock(c, r, h, BLOCK_STONE_LIGHT); // stone brick proxy
                } else {
                    setBlock(c, r, h, BLOCK_AIR);
                }
            }
        }
    }
    // Mob spawner & chest
    setBlock(c0 + 4, r0 + 4, groundH + 1, BLOCK_ORE_DIAMOND); // proxy for spawner
    setBlock(c0 + 4, r0 + 4, groundH + 2, BLOCK_GLASS);
    setBlock(c0 + 2, r0 + 2, groundH + 1, BLOCK_WOOD); // proxy for chest
}

void buildHut(int c0, int r0, int G) {
    // 5x5 hut
    for(int c=c0; c<=c0+4; c++){
        for(int r=r0; r<=r0+4; r++){
            for(int h=G; h<=G+3; h++){
                if(c==c0||c==c0+4||r==r0||r==r0+4){
                    setBlock(c, r, h, BLOCK_WOOD);
                } else {
                    setBlock(c, r, h, BLOCK_AIR);
                }
            }
            setBlock(c, r, G+4, BLOCK_WOOD); // roof
        }
    }
    setBlock(c0+2, r0+4, G+1, BLOCK_AIR); // door
    setBlock(c0+2, r0+4, G+2, BLOCK_AIR);
}

void buildVillage(int c0, int r0) {
    int G = UNDERGROUND_DEPTH;
    // Central well
    buildHut(c0, r0, G);
    buildHut(c0+8, r0+2, G);
    buildHut(c0-2, r0+8, G);
    buildHut(c0+10, r0+10, G);
    
    // Well in center
    setBlock(c0+5, r0+5, G+1, BLOCK_STONE);
    setBlock(c0+5, r0+5, G, BLOCK_WATER);
    setBlock(c0+4, r0+5, G+1, BLOCK_STONE);
    setBlock(c0+6, r0+5, G+1, BLOCK_STONE);
    setBlock(c0+5, r0+4, G+1, BLOCK_STONE);
    setBlock(c0+5, r0+6, G+1, BLOCK_STONE);
}

void buildPyramid(int c0, int r0) {
    int G = UNDERGROUND_DEPTH;
    int size = 15; // must be odd
    int maxH = size / 2;
    for (int h = 0; h <= maxH; h++) {
        for (int c = c0 + h; c < c0 + size - h; c++) {
            for (int r = r0 + h; r < r0 + size - h; r++) {
                setBlock(c, r, G + 1 + h, BLOCK_SAND);
                // Hollow center
                if (h == 0 && c > c0+1 && c < c0+size-2 && r > r0+1 && r < r0+size-2) {
                    setBlock(c, r, G + 1, BLOCK_AIR);
                }
            }
        }
    }
    setBlock(c0 + size/2, r0 + size - 1, G + 1, BLOCK_AIR);
    setBlock(c0 + size/2, r0 + size - 1, G + 2, BLOCK_AIR);
}

void buildWatchtower(int c0, int r0) {
    int G = UNDERGROUND_DEPTH;
    for (int h = G; h < G + 15; h++) {
        for (int c = c0; c <= c0+2; c++) {
            for (int r = r0; r <= r0+2; r++) {
                if (c==c0+1 && r==r0+1) setBlock(c, r, h, BLOCK_AIR); // hollow
                else setBlock(c, r, h, BLOCK_STONE);
            }
        }
    }
    setBlock(c0+1, r0, G+1, BLOCK_AIR);
    setBlock(c0+1, r0, G+2, BLOCK_AIR);
    for (int c = c0-1; c <= c0+3; c++) {
        for (int r = r0-1; r <= r0+3; r++) {
            setBlock(c, r, G+15, BLOCK_WOOD); // viewing platform
            if (c==c0-1||c==c0+3||r==r0-1||r==r0+3) setBlock(c, r, G+16, BLOCK_WOOD); // rail
        }
    }
}
// =====================================================
// 16H: Cellular Automata Fluid Dynamics
// =====================================================
void updateFluids(float time) {
    static float lastFluidUpdate = 0;
    if (time - lastFluidUpdate < 0.2f) return; // 5 updates per second
    lastFluidUpdate = time;

    // Track original state to prevent cascading updates within same frame
    static int origWater[GRID_W][GRID_D][GRID_H];
    for (int col = 0; col < GRID_W; col++) {
        for (int row = 0; row < GRID_D; row++) {
            for (int h = 0; h < GRID_H; h++) {
                if (blockGrid[col][row][h] == BLOCK_WATER) {
                    origWater[col][row][h] = 1;
                } else {
                    origWater[col][row][h] = 0;
                }
            }
        }
    }

    // Process fluids around the camera
    int cCol = GRID_OFF_X - 20; // approximate center chunk
    for (int col = 5; col < GRID_W - 5; col++) {
        for (int row = 5; row < GRID_D - 5; row++) {
            for (int h = 1; h < GRID_H; h++) {
                if (origWater[col][row][h] == 1) {
                    // Try flowing down first
                    if (h > 1 && blockGrid[col][row][h-1] == BLOCK_AIR) {
                        setBlock(col - GRID_OFF_X, row - GRID_OFF_Z, h-1, BLOCK_WATER);
                    } 
                    // Support below -> spread outward
                    else if (h > 1 && blockGrid[col][row][h-1] != BLOCK_AIR && blockGrid[col][row][h-1] != BLOCK_WATER) {
                        int dc[4] = {1, -1, 0, 0};
                        int dr[4] = {0, 0, 1, -1};
                        for (int i=0; i<4; i++) {
                            if (blockGrid[col+dc[i]][row+dr[i]][h] == BLOCK_AIR) {
                                // Simplified spread limit
                                setBlock(col+dc[i] - GRID_OFF_X, row+dr[i] - GRID_OFF_Z, h, BLOCK_WATER);
                            }
                        }
                    }
                }
            }
        }
    }
}

void initBlockGrid() {

    memset(blockGrid, 0, sizeof(blockGrid));
    memset(columnMaxH, 0, sizeof(columnMaxH));
    treeLocations.clear();
    torchLocations.clear();

    for (int row = TERRAIN_MIN; row <= TERRAIN_MAX; row++) {
        for (int col = TERRAIN_MIN; col <= TERRAIN_MAX; col++) {
            unsigned int seed = gridSeed(col, row);

            // Water ponds (expanded with beaches)
            int pondType = isWaterPond(col, row);
            if (pondType == 1) {
                setBlock(col, row, 0, BLOCK_BEDROCK);
                for (int h = 1; h < UNDERGROUND_DEPTH; h++)
                    setBlock(col, row, h, BLOCK_STONE);
                // Sandy/clay bottom with gravel patches
                setBlock(col, row, UNDERGROUND_DEPTH - 2, BLOCK_CLAY);
                setBlock(col, row, UNDERGROUND_DEPTH - 1, (seed % 3 == 0) ? BLOCK_GRAVEL : BLOCK_SAND);
                setBlock(col, row, UNDERGROUND_DEPTH, BLOCK_WATER);
                setBlock(col, row, UNDERGROUND_DEPTH + 1, BLOCK_WATER);
                continue;
            }
            if (pondType == 2) {
                setBlock(col, row, 0, BLOCK_BEDROCK);
                for (int h = 1; h < UNDERGROUND_DEPTH; h++)
                    setBlock(col, row, h, BLOCK_STONE);
                // Beach: sand with gravel patches near water edge
                int surfBlock = (seed % 5 == 0) ? BLOCK_GRAVEL : BLOCK_SAND;
                setBlock(col, row, UNDERGROUND_DEPTH, surfBlock);
                setBlock(col, row, UNDERGROUND_DEPTH + 1, BLOCK_SAND);
                continue;
            }

            // ======== BIOME BLENDING ========
            // Sample this column's biome and neighbors to detect edges
            int biome = getBiome(col, row);
            int height = getTerrainHeightBiome(col, row, biome);

            // Check if we're at a biome edge (any neighbor has different biome)
            bool atBiomeEdge = false;
            int neighborBiome = biome;
            for (int dc = -2; dc <= 2; dc += 2) {
                for (int dr = -2; dr <= 2; dr += 2) {
                    if (dc == 0 && dr == 0) continue;
                    int nb = getBiome(col + dc, row + dr);
                    if (nb != biome) {
                        atBiomeEdge = true;
                        neighborBiome = nb;
                        break;
                    }
                }
                if (atBiomeEdge) break;
            }

            // Beach transition: sand strip where land meets water biome
            bool isBeachTransition = false;
            if (biome != 3) {
                for (int dc = -3; dc <= 3; dc++) {
                    for (int dr = -3; dr <= 3; dr++) {
                        if (dc * dc + dr * dr > 10) continue;
                        if (getBiome(col + dc, row + dr) == 3) {
                            isBeachTransition = true;
                            break;
                        }
                    }
                    if (isBeachTransition) break;
                }
            }

            // Blend height at biome edges for smooth transitions
            if (atBiomeEdge) {
                int neighborH = getTerrainHeightBiome(col, row, neighborBiome);
                height = (height * 2 + neighborH) / 3; // weighted blend toward own biome
            }

            // Beach: flatten and use sand near water biomes
            if (isBeachTransition) {
                if (height > UNDERGROUND_DEPTH + 2) height = UNDERGROUND_DEPTH + 2;
                biome = 0; // force sand surface
            }

            // ======== UNDERGROUND LAYERS ========
            setBlock(col, row, 0, BLOCK_BEDROCK);
            // Irregular bedrock layer (1-2 blocks thick)
            if (seed % 3 == 0) setBlock(col, row, 1, BLOCK_BEDROCK);

            for (int h = 1; h < UNDERGROUND_DEPTH; h++) {
                if (getBlock(col, row, h) == BLOCK_BEDROCK) continue; // skip if already bedrock

                // 16G: Ravines cutting down to y=3
                bool isRavine = abs(fbmNoise(col * 0.05f + 100.0f, row * 0.05f + 100.0f, 2)) < 0.03f;
                if (isRavine && h >= 3 && h < UNDERGROUND_DEPTH) {
                    setBlock(col, row, h, BLOCK_AIR);
                    continue;
                }

                // 16A: Cave networks
                float caveN1 = fbmNoise(col * 0.12f + h * 0.4f, row * 0.12f + h * 0.6f, 2);
                float caveN2 = fbmNoise(col * 0.08f + h * 0.3f + 50.0f, row * 0.08f + h * 0.5f + 50.0f, 2);
                if (h >= 3 && h <= UNDERGROUND_DEPTH - 2 && caveN1 > 0.42f && caveN2 > 0.3f) {
                    setBlock(col, row, h, BLOCK_AIR);
                    continue;
                }

                // 16B: Clustered Ore Veins
                int blockType = BLOCK_STONE;
                unsigned int oreSeed = gridSeed(col/2 + h*11, row/2 + h*13); 
                if (h <= 3) {
                    if (oreSeed % 18 == 0) blockType = BLOCK_ORE_DIAMOND;
                } else if (h <= 6) {
                    if (oreSeed % 15 == 0) blockType = BLOCK_ORE_GOLD;
                    else if (oreSeed % 10 == 0) blockType = BLOCK_COAL_ORE;
                } else {
                    if (oreSeed % 8 == 0) blockType = BLOCK_COAL_ORE;
                    else if (oreSeed % 20 == 0) blockType = BLOCK_GRAVEL;
                }
                setBlock(col, row, h, blockType);
            }

            // ======== SURFACE LAYERS ========
            int topH = height + 1;
            if (topH >= GRID_H) topH = GRID_H - 1;

            for (int h = UNDERGROUND_DEPTH; h <= topH; h++) {
                int blockType;
                int depthFromTop = topH - h;

                if (h == topH) {
                    // ---- TOP SURFACE BLOCK ----
                    switch (biome) {
                        case 4: blockType = BLOCK_SNOW; break;
                        case 0: blockType = BLOCK_SAND; break;
                        case 1: {
                            // Snow on very tall grass hills
                            if (topH >= UNDERGROUND_DEPTH + 10)
                                blockType = BLOCK_SNOW;
                            else
                                blockType = BLOCK_GRASS;
                            break;
                        }
                        case 2: {
                            // Snow on mountain peaks, stone/gravel mix lower
                            if (topH >= UNDERGROUND_DEPTH + 16)
                                blockType = BLOCK_SNOW;
                            else if (topH >= UNDERGROUND_DEPTH + 12)
                                blockType = (seed % 3 == 0) ? BLOCK_GRAVEL : BLOCK_STONE;
                            else
                                blockType = (seed % 2) ? BLOCK_STONE : BLOCK_STONE_LIGHT;
                            break;
                        }
                        default: blockType = BLOCK_SAND; break;
                    }
                } else if (biome == 1) {
                    // ---- GRASS BIOME SUBSURFACE ----
                    if (depthFromTop <= 1) {
                        blockType = BLOCK_DIRT; // 1 block dirt right under grass
                    } else if (depthFromTop <= 4) {
                        blockType = BLOCK_DIRT; // 4 layers of dirt total (visible on cliffs!)
                    } else if (depthFromTop <= 6) {
                        // Transition: mixed dirt/stone/gravel
                        unsigned int ts = gridSeed(col + h * 3, row + h * 5);
                        if (ts % 4 == 0) blockType = BLOCK_GRAVEL;
                        else if (ts % 3 == 0) blockType = BLOCK_DIRT;
                        else blockType = BLOCK_STONE;
                    } else {
                        // Deep: stone with ores
                        unsigned int oreSeed = gridSeed(col + h * 7, row + h * 13);
                        if (oreSeed % 40 == 0) blockType = BLOCK_ORE_DIAMOND;
                        else if (oreSeed % 30 == 0) blockType = BLOCK_ORE_GOLD;
                        else if (oreSeed % 15 == 0) blockType = BLOCK_COAL_ORE;
                        else blockType = BLOCK_STONE;
                    }
                } else if (biome == 0) {
                    // ---- SAND BIOME SUBSURFACE ----
                    if (depthFromTop <= 3) {
                        blockType = BLOCK_SAND;
                    } else if (depthFromTop <= 5) {
                        // Sandstone-like layer (use stone_light)
                        blockType = BLOCK_STONE_LIGHT;
                    } else {
                        blockType = BLOCK_STONE;
                    }
                } else if (biome == 2) {
                    // ---- STONE BIOME SUBSURFACE ----
                    if (depthFromTop <= 2 && topH >= UNDERGROUND_DEPTH + 16) {
                        // Snow layer on peaks
                        blockType = BLOCK_SNOW;
                    } else if (depthFromTop <= 1) {
                        blockType = (seed % 3 == 0) ? BLOCK_GRAVEL : BLOCK_STONE;
                    } else {
                        unsigned int oreSeed = gridSeed(col + h * 7, row + h * 13);
                        if (oreSeed % 35 == 0) blockType = BLOCK_ORE_DIAMOND;
                        else if (oreSeed % 25 == 0) blockType = BLOCK_ORE_GOLD;
                        else if (oreSeed % 12 == 0) blockType = BLOCK_COAL_ORE;
                        else if (oreSeed % 20 == 0) blockType = BLOCK_GRAVEL;
                        else blockType = BLOCK_STONE;
                    }
                } else {
                    blockType = BLOCK_STONE;
                }
                setBlock(col, row, h, blockType);
            }

            // ======== CLAY DEPOSITS near rivers/ponds ========
            if (isBeachTransition && seed % 6 == 0) {
                setBlock(col, row, UNDERGROUND_DEPTH, BLOCK_CLAY);
            }

            // ======== TREES ========
            int surfaceH = topH;
            if (isForestZone(col, row) && biome == 1 && surfaceH >= UNDERGROUND_DEPTH + 1
                && surfaceH < UNDERGROUND_DEPTH + 10) { // no trees on snowy peaks
                if (seed % 20 == 0) {
                    TreeInfo ti;
                    ti.col = col; ti.row = row;
                    ti.leafColor = TREE_COLORS[seed % NUM_TREE_COLORS];
                    ti.large = (seed % 3 == 0);
                    ti.variant = (int)((seed >> 5) % (ti.large ? NUM_LARGE_TREE_VARIANTS : NUM_TREE_VARIANTS));
                    treeLocations.push_back(ti);
                }
            } else if (biome == 1 && surfaceH >= UNDERGROUND_DEPTH + 1
                       && surfaceH < UNDERGROUND_DEPTH + 10 && (seed % 80 == 0)) {
                TreeInfo ti;
                ti.col = col; ti.row = row;
                ti.leafColor = TREE_COLORS[seed % NUM_TREE_COLORS];
                ti.large = (seed % 3 == 0);
                ti.variant = (int)((seed >> 5) % (ti.large ? NUM_LARGE_TREE_VARIANTS : NUM_TREE_VARIANTS));
                treeLocations.push_back(ti);
            }
            // Sparse trees on stone biome at mid-altitude
            if (biome == 2 && surfaceH >= UNDERGROUND_DEPTH + 4
                && surfaceH < UNDERGROUND_DEPTH + 12 && (seed % 120 == 0)) {
                TreeInfo ti;
                ti.col = col; ti.row = row;
                ti.leafColor = glm::vec3(0.12f, 0.4f, 0.1f); // dark green pine
                ti.large = false;
                ti.variant = (int)((seed >> 5) % NUM_TREE_VARIANTS);
                treeLocations.push_back(ti);
            }

            // ======== TORCHES ========
            if (biome == 1 && surfaceH >= UNDERGROUND_DEPTH + 1 && (seed % 61 == 0)) {
                // height = the block the torch stands on, i.e. the surface itself
                TorchInfo t; t.col = col; t.row = row; t.height = surfaceH;
                torchLocations.push_back(t);
            }
        }
    }

    // Standalone large landmark trees
    {
        int landmarks[][2] = {{-30, -5}, {-35, -20}, {40, -15}, {-45, 5}, {40, 30}};
        for (auto& lm : landmarks) {
            TreeInfo ti; ti.col = lm[0]; ti.row = lm[1];
            ti.leafColor = glm::vec3(0); ti.large = true;
            ti.variant = (int)(&lm - &landmarks[0]) % NUM_LARGE_TREE_VARIANTS;
            treeLocations.push_back(ti);
        }
    }

    // ===========================================
    // MAIN SPAWN — MEDIEVAL CASTLE
    // ===========================================
    buildMedievalCastle(-30, -5);

    // 16C, 16D, 16E: Procedural Structures
    for (int sc = -80; sc <= 80; sc += 30) {
        for (int sr = -80; sr <= 80; sr += 30) {
            if (sc > -40 && sc < 10 && sr > -20 && sr < 10) continue; // avoid castle
            unsigned int ss = gridSeed(sc, sr);
            int sb = getBiome(sc, sr);
            int sh = getTerrainHeightBiome(sc, sr, sb);
            
            if (ss % 4 == 0) buildDungeon(sc, sr, 3 + (ss % 4));
            if (sb == 2 && ss % 3 == 0) buildWatchtower(sc, sr);
            if (sb == 0 && ss % 3 == 0) buildPyramid(sc, sr);
            if (sb == 1 && ss % 5 == 0) buildVillage(sc, sr);
        }
    }
}

// =====================================================
// Hex neighbor offsets (odd-row offset grid)
// =====================================================
static const int hexNeighborEven[6][2] = {
    {+1,  0}, {-1,  0},   // east, west
    { 0, +1}, { 0, -1},   // SE, NE (even row)
    {-1, +1}, {-1, -1}    // SW, NW (even row)
};
static const int hexNeighborOdd[6][2] = {
    {+1,  0}, {-1,  0},   // east, west
    {+1, +1}, {+1, -1},   // SE, NE (odd row)
    { 0, +1}, { 0, -1}    // SW, NW (odd row)
};

// Can a neighbouring block of this type actually hide the face behind it?
// Only a block that both fills its whole hex cell AND is opaque can. Testing
// `!= BLOCK_AIR` (the old rule) counted glass, water, ice, panes, fences,
// slabs, stairs, carpets, ladders and torches as solid walls, so anything
// behind them was culled and you saw straight through the world.
// Baked into a table: isBlockOccluded() calls this 8 times per candidate block,
// and getBlockProps() is a switch over ~150 cases. One pass at startup, then a
// byte load per query.
static bool g_blocksSight[BLOCK_COUNT];

void initSightTable() {
    int opaque = 0;
    for (int bt = 0; bt < BLOCK_COUNT; bt++) {
        BlockProperties p = getBlockProps(bt);
        g_blocksSight[bt] = (bt != BLOCK_AIR)
                         && (bt != BLOCK_LEAF)          // canopy is full of gaps
                         && !p.isTransparent            // glass, pane, water, ice
                         && p.shape == SHAPE_FULL_HEX;  // slab/stair/carpet/fence/door/...
        if (g_blocksSight[bt]) opaque++;
    }
    printf("[World] Sight table: %d of %d block types occlude\n", opaque, (int)BLOCK_COUNT);
}

inline bool blocksSight(int bt) {
    return (bt >= 0 && bt < BLOCK_COUNT) ? g_blocksSight[bt] : false;
}

// Index of the highest block in this column that something can stand on.
// Uses blocksSight() rather than "not air", so leaves, torches, glass and
// water do not read as ground. Starts from columnMaxH so it does not walk
// empty sky. Returns -1 for a column with nothing solid in it.
int findGroundH(int col, int row) {
    int gi = col + GRID_OFF_X, gj = row + GRID_OFF_Z;
    if (gi < 0 || gi >= GRID_W || gj < 0 || gj >= GRID_D) return -1;
    for (int h = columnMaxH[gi][gj]; h >= 0; h--) {
        if (blocksSight(getBlock(col, row, h))) return h;
    }
    return -1;
}

// Pick a spawn point that is actually outdoors (19E-4).
// The old spawn was hard-coded to grid (3,5), which lands inside the KUET hill
// build, so the first thing the player ever sees is the dark inside of a
// structure. A column is a good spawn when its highest block IS its ground —
// nothing built over the player's head — and that ground is dry land.
// Searches outward from (0,0) in rings and takes the first column that passes.
bool findSpawnColumn(int& outCol, int& outRow) {
    for (int ring = 0; ring < 80; ring++) {
        for (int dc = -ring; dc <= ring; dc++) {
            for (int dr = -ring; dr <= ring; dr++) {
                if (std::max(abs(dc), abs(dr)) != ring) continue; // perimeter only
                int c = dc, r = dr;
                int gi = c + GRID_OFF_X, gj = r + GRID_OFF_Z;
                if (gi < 1 || gi >= GRID_W - 1 || gj < 1 || gj >= GRID_D - 1) continue;

                int g = findGroundH(c, r);
                if (g < UNDERGROUND_DEPTH) continue;      // underwater or in a pit
                if (columnMaxH[gi][gj] != g) continue;    // something is overhead

                int surface = getBlock(c, r, g);
                if (surface == BLOCK_WATER || surface == BLOCK_ICE) continue;

                // Head-height clearance in all six directions, so the player is
                // not wedged against a wall on the first frame.
                bool boxed = false;
                for (int i = 0; i < 6 && !boxed; i++) {
                    bool odd = (((r % 2) + 2) % 2) == 1;
                    const int (*nb)[2] = odd ? hexNeighborOdd : hexNeighborEven;
                    if (getBlock(c + nb[i][0], r + nb[i][1], g + 2) != BLOCK_AIR) boxed = true;
                }
                if (boxed) continue;

                outCol = c; outRow = r;
                printf("[Player] Spawn search: grid (%d,%d), ground h=%d, ring %d\n",
                       c, r, g, ring);
                return true;
            }
        }
    }
    printf("[Player] Spawn search failed — falling back to (3,5)\n");
    outCol = 3; outRow = 5;
    return false;
}

// Resolve every tree that was placed without a recorded ground height. Runs once
// after world-gen; before this, the draw loop did the same scan per tree PER
// FRAME. Trees whose column turns out to be empty are dropped rather than left
// hovering at y=1.
void resolveGroundHeights() {
    int resolved = 0, dropped = 0;
    for (int i = (int)treeLocations.size() - 1; i >= 0; i--) {
        TreeInfo& ti = treeLocations[i];
        if (ti.groundH >= 0) continue;
        int g = findGroundH(ti.col, ti.row);
        if (g < 0) {
            treeLocations.erase(treeLocations.begin() + i);
            dropped++;
        } else {
            ti.groundH = g;
            resolved++;
        }
    }
    printf("[World] Ground heights: %d trees resolved, %d dropped (no ground), %d torches\n",
           resolved, dropped, (int)torchLocations.size());
}

bool isBlockOccluded(int col, int row, int h) {
    // Check top and bottom
    if (!blocksSight(getBlock(col, row, h + 1))) return false;
    if (h == 0 || !blocksSight(getBlock(col, row, h - 1))) return false;

    // Check 6 hex neighbors
    bool odd = (((row % 2) + 2) % 2) == 1; // handle negative rows
    const int (*nb)[2] = odd ? hexNeighborOdd : hexNeighborEven;
    for (int i = 0; i < 6; i++) {
        if (!blocksSight(getBlock(col + nb[i][0], row + nb[i][1], h)))
            return false;
    }
    return true;
}

// =====================================================
// Frustum culling — extract planes from VP matrix
// =====================================================
struct FrustumPlane { float a, b, c, d; };
FrustumPlane frustumPlanes[6];

void extractFrustum(const glm::mat4& vp) {
    // Left, Right, Bottom, Top, Near, Far
    for (int i = 0; i < 3; i++) {
        // plane 2i = row3 + row_i
        frustumPlanes[2*i].a = vp[0][3] + vp[0][i];
        frustumPlanes[2*i].b = vp[1][3] + vp[1][i];
        frustumPlanes[2*i].c = vp[2][3] + vp[2][i];
        frustumPlanes[2*i].d = vp[3][3] + vp[3][i];
        // plane 2i+1 = row3 - row_i
        frustumPlanes[2*i+1].a = vp[0][3] - vp[0][i];
        frustumPlanes[2*i+1].b = vp[1][3] - vp[1][i];
        frustumPlanes[2*i+1].c = vp[2][3] - vp[2][i];
        frustumPlanes[2*i+1].d = vp[3][3] - vp[3][i];
    }
    // Normalize
    for (int i = 0; i < 6; i++) {
        float len = sqrtf(frustumPlanes[i].a * frustumPlanes[i].a +
                          frustumPlanes[i].b * frustumPlanes[i].b +
                          frustumPlanes[i].c * frustumPlanes[i].c);
        if (len > 0.0001f) {
            frustumPlanes[i].a /= len;
            frustumPlanes[i].b /= len;
            frustumPlanes[i].c /= len;
            frustumPlanes[i].d /= len;
        }
    }
}

bool isInFrustum(const glm::vec3& pos, float radius = 1.0f) {
    for (int i = 0; i < 6; i++) {
        float dist = frustumPlanes[i].a * pos.x +
                     frustumPlanes[i].b * pos.y +
                     frustumPlanes[i].c * pos.z +
                     frustumPlanes[i].d;
        if (dist < -radius) return false;
    }
    return true;
}

// Global VP matrix for frustum culling (set before renderTerrain)
glm::mat4 currentVP(1.0f);

// Returns texture ID for a block type (0 = no texture, use color only)
// textureMode to use: 1 = texture only, 2 = texture*color
GLuint getBlockTexture(int type) {
    switch (type) {
        case BLOCK_STONE:         return texStoneFM;
        case BLOCK_SMOOTH_STONE:  return texStoneFM;
        case BLOCK_BEDROCK:       return texBedrock;
        case BLOCK_COBBLESTONE:   return texCobblestone;
        case BLOCK_MOSSY_COBBLESTONE: return texMossyCobble;
        case BLOCK_BRICKS:        return texStoneBricks;
        case BLOCK_MOSSY_BRICKS:  return texMossyStoneBricks;
        case BLOCK_SLAB_STONE:    return texCobblestone;
        case BLOCK_SLAB_BRICK:    return texStoneBricks;
        case BLOCK_STAIRS_STONE:  return texCobblestone;
        case BLOCK_PLANKS:        return texOakPlanks;
        case BLOCK_SLAB_WOOD:     return texOakPlanks;
        case BLOCK_STAIRS_WOOD:   return texOakPlanks;
        case BLOCK_FENCE_WOOD:    return texOakPlanks;
        case BLOCK_FENCE_GATE:    return texOakPlanks;
        case BLOCK_LADDER:        return texLadder;
        case BLOCK_DOOR_IRON:     return texIronDoorBot;
        case BLOCK_IRON_BARS:     return texIronBars;
        case BLOCK_GLASS:         return texGlassFM;
        case BLOCK_GLASS_PANE:    return texGlassFM;
        case BLOCK_BOOKSHELF:     return texBookshelf;
        case BLOCK_CRAFTING_TABLE: return texCraftingTop;
        case BLOCK_TORCH_BLOCK:   return texTorchBlock;
        case BLOCK_LANTERN:       return texSeaLantern;
        case BLOCK_COAL_ORE:      return texCoalOre;
        case BLOCK_ORE_DIAMOND:   return texDiamondOre;
        case BLOCK_ORE_GOLD:      return texGoldOre;

        // Gold pack textures
        case BLOCK_GRASS:         return texGrassTop;
        case BLOCK_SAND:          return texSandGold;
        case BLOCK_WOOD:          return texOakLog;
        case BLOCK_LEAF:          return texOakLeaves;
        case BLOCK_SNOW:          return texSnow;
        case BLOCK_ICE:           return texIceGold;
        case BLOCK_CLAY:          return texClayGold;
        case BLOCK_GRAVEL:        return texGravelGold;
        case BLOCK_GLOWSTONE:     return texGlowstoneGold;
        case BLOCK_DIAMOND_BLOCK: return texDiamondBlock;
        case BLOCK_GOLD_BLOCK:    return texGoldBlock;
        case BLOCK_IRON_BLOCK:    return texIronBlockTex;
        case BLOCK_OBSIDIAN:      return texObsidian;
        case BLOCK_SANDSTONE:     return texSandstoneGold;
        case BLOCK_SLAB_SANDSTONE:return texSandstoneGold;
        case BLOCK_CUT_SANDSTONE: return texCutSandstone;
        case BLOCK_QUARTZ_BLOCK:  return texQuartzTop;
        case BLOCK_POLISHED_ANDESITE: return texPolAndesite;
        case BLOCK_POLISHED_DIORITE:  return texPolDiorite;
        case BLOCK_POLISHED_GRANITE:  return texPolGranite;
        case BLOCK_TRAPDOOR_OAK:  return texOakTrapdoor;
        case BLOCK_DOOR_OAK:      return texOakDoorBot;
        case BLOCK_WATER:         return texWaterStill;

        // Wool, carpets, dirt — use color only (no texture in pack)

        // Tool item sprites
        case ITEM_STICK:           return texItemStick;
        case ITEM_SWORD_WOOD:      return texItemSwordWood;
        case ITEM_AXE_WOOD:        return texItemAxeWood;
        case ITEM_PICKAXE_WOOD:    return texItemPickaxeWood;
        case ITEM_SHOVEL_WOOD:     return texItemShovelWood;
        case ITEM_SWORD_STONE:     return texItemSwordStone;
        case ITEM_AXE_STONE:       return texItemAxeStone;
        case ITEM_PICKAXE_STONE:   return texItemPickaxeStone;
        case ITEM_SHOVEL_STONE:    return texItemShovelStone;
        case ITEM_SWORD_IRON:      return texItemSwordIron;
        case ITEM_AXE_IRON:        return texItemAxeIron;
        case ITEM_PICKAXE_IRON:    return texItemPickaxeIron;
        case ITEM_SHOVEL_IRON:     return texItemShovelIron;
        case ITEM_SWORD_DIAMOND:   return texItemSwordDiamond;
        case ITEM_AXE_DIAMOND:     return texItemAxeDiamond;
        case ITEM_PICKAXE_DIAMOND: return texItemPickaxeDiamond;
        case ITEM_SHOVEL_DIAMOND:  return texItemShovelDiamond;
        case ITEM_BOW:             return texItemBow;
        // POND reuses the existing still-water texture rather than importing a
        // second icon — it is exactly the material the tool produces.
        case ITEM_TOOL_HILL:       return texHillIcon;
        case ITEM_TOOL_POND:       return texWaterStill;

        default: return 0;
    }
}

// Bind a texture for block rendering; returns textureMode to set
// mode 1 = texture only (stone, cobble — self-colored)
// mode 2 = texture*color (planks, wool blended)
// How glossy each block family is.
//
// The shader used to apply pow(spec, 32.0) at a fixed strength to every surface
// alike, so a grass field carried the same tight specular highlight as a polished
// metal block — a bright spot that slid across the terrain as the camera turned,
// which is the single most artificial-looking thing about a wide daylight view.
// Exponent controls how tight the highlight is, strength how bright.
// Last values uploaded, so a terrain pass over thousands of blocks does not issue
// two redundant glUniform1f calls per block. Long runs of one block type are the
// common case, so this collapses to almost nothing.
static float g_lastSpecPower = -1.0f, g_lastSpecStrength = -1.0f;

static void uploadSpecular(float power, float strength) {
    if (power == g_lastSpecPower && strength == g_lastSpecStrength) return;
    g_lastSpecPower = power;
    g_lastSpecStrength = strength;
    setFloat(shaderProgram, "specPower", power);
    setFloat(shaderProgram, "specStrength", strength);
}

// Put the shader back to the neutral response main.cpp uploads each frame. Must
// be called when leaving a run of block draws, or the next thing drawn (trees,
// mobs, props) inherits whatever the last block happened to be.
void resetBlockSpecular() { uploadSpecular(32.0f, 1.0f); }

// Wind sway amplitude for the next draw, in world units. Same
// skip-redundant-uploads treatment as the specular pair, for the same reason:
// this is set per block in the terrain pass.
static float g_lastSway = -1.0f;
void setSway(float amount) {
    if (amount == g_lastSway) return;
    g_lastSway = amount;
    setFloat(shaderProgram, "swayAmount", amount);
}

void setBlockSpecular(int type) {
    float power = 32.0f, strength = 1.0f;
    switch (type) {
        // Wet and glassy — a tight, bright highlight is the whole point
        case BLOCK_WATER: case BLOCK_ICE:
            power = 96.0f; strength = 2.0f; break;
        // Metal and gem: polished, so tight but not mirror-like
        case BLOCK_IRON_BLOCK: case BLOCK_GOLD_BLOCK: case BLOCK_DIAMOND_BLOCK:
        case BLOCK_ORE_DIAMOND: case BLOCK_ORE_GOLD: case BLOCK_IRON_BARS:
        case BLOCK_OBSIDIAN: case BLOCK_QUARTZ_BLOCK: case BLOCK_GLASS:
        case BLOCK_GLASS_PANE:
            power = 64.0f; strength = 1.2f; break;
        // Dressed stone: some sheen
        case BLOCK_POLISHED_DIORITE: case BLOCK_POLISHED_GRANITE:
        case BLOCK_POLISHED_ANDESITE: case BLOCK_SMOOTH_STONE:
            power = 48.0f; strength = 0.6f; break;
        // Matte organics and cloth: essentially no highlight. This is the case
        // that matters most, because it covers almost all visible terrain.
        case BLOCK_GRASS: case BLOCK_DIRT: case BLOCK_SAND: case BLOCK_GRAVEL:
        case BLOCK_CLAY: case BLOCK_LEAF: case BLOCK_SNOW:
        case BLOCK_WOOL_WHITE: case BLOCK_WOOL_RED: case BLOCK_WOOL_BLUE:
        case BLOCK_WOOL_GREEN: case BLOCK_WOOL_YELLOW: case BLOCK_WOOL_BLACK:
        case BLOCK_WOOL_ORANGE: case BLOCK_WOOL_PINK: case BLOCK_WOOL_PURPLE:
        case BLOCK_WOOL_CYAN: case BLOCK_WOOL_BROWN: case BLOCK_WOOL_GRAY:
        case BLOCK_WOOL_LIGHT_GRAY: case BLOCK_WOOL_MAGENTA: case BLOCK_WOOL_LIME:
        case BLOCK_CARPET_WHITE: case BLOCK_CARPET_RED: case BLOCK_CARPET_BLUE:
            power = 16.0f; strength = 0.08f; break;
        default:
            // Rough stone and bare wood: broad, dim highlight.
            if (isStoneType(type))     { power = 40.0f; strength = 0.35f; }
            else if (isWoodType(type)) { power = 32.0f; strength = 0.30f; }
            else if (isDirtType(type)) { power = 16.0f; strength = 0.08f; }
            break;
    }
    uploadSpecular(power, strength);
}

int bindBlockTexture(int type) {
    setBlockSpecular(type);
    // Leaf blocks are foliage too — a hedge or a player-built canopy should move
    // with the same wind as the trees. Smaller amplitude than a tree canopy,
    // since a leaf block is anchored on all sides rather than hanging off a limb.
    setSway(type == BLOCK_LEAF ? 0.05f : 0.0f);
    GLuint tex = getBlockTexture(type);
    if (!tex) {
        setInt(shaderProgram, "textureMode", 0);
        return 0;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    setInt(shaderProgram, "tex", 0);
    // Stone-type blocks: texture only (they have no tint)
    switch (type) {
        case BLOCK_STONE: case BLOCK_SMOOTH_STONE: case BLOCK_BEDROCK:
        case BLOCK_COBBLESTONE: case BLOCK_MOSSY_COBBLESTONE:
        case BLOCK_BRICKS: case BLOCK_MOSSY_BRICKS:
        case BLOCK_SLAB_STONE: case BLOCK_SLAB_BRICK:
        case BLOCK_STAIRS_STONE:
        case BLOCK_COAL_ORE: case BLOCK_ORE_DIAMOND: case BLOCK_ORE_GOLD:
        case BLOCK_IRON_BARS: case BLOCK_LADDER:
        case BLOCK_TORCH_BLOCK: case BLOCK_LANTERN:
        case BLOCK_CRAFTING_TABLE:
            setInt(shaderProgram, "textureMode", 1);
            return 1;
        default:
            // Wood, planks, glass — blend texture with color tint
            setInt(shaderProgram, "textureMode", 2);
            return 2;
    }
}

// =====================================================
// Render terrain from voxel grid (with occlusion + frustum culling)
// =====================================================
// =====================================================
// Collect the torch lights for this frame.
// Must run BEFORE the draw pass: torchPositions used to be filled *during*
// renderTerrain() while the uniforms were uploaded *after* it, so every frame
// was lit by the previous frame's light set (a visible one-frame lag when
// moving). Lights are also not frustum-culled — a torch behind the camera still
// illuminates the wall in front of it — and the list is sorted nearest-first so
// that truncating to MAX_POINT_LIGHTS keeps the 8 that matter instead of the
// first 8 in grid order (which made lights pop in and out as the player walked).
// =====================================================
void gatherTorchLights() {
    torchPositions.clear();
    for (auto& t : torchLocations) {
        glm::vec3 pos = hexGridPos(t.col, t.row, 0.0f);
        float tdx = pos.x - camPos.x;
        float tdz = pos.z - camPos.z;
        if (tdx * tdx + tdz * tdz > RENDER_DIST_SQ) continue;

        // Same cached ground height the draw loop uses, so the light sits exactly
        // where the torch mesh is instead of tracking a rescanned column top.
        torchPositions.push_back({ pos + glm::vec3(0, (t.height + 1) * HEX_HEIGHT + 0.8f, 0),
                                   COL_TORCH_FLAME });
    }
    // Emissive blocks in the grid — glowstone, lanterns, torch blocks. These are
    // player-placeable, so unlike torchLocations the set changes at runtime; the
    // registry is maintained by setBlock rather than rebuilt by scanning, because
    // scanning 40,000 columns for them every frame is not affordable.
    for (auto& kv : lightBlocks) {
        float ldx = kv.second.pos.x - camPos.x;
        float ldz = kv.second.pos.z - camPos.z;
        if (ldx * ldx + ldz * ldz > RENDER_DIST_SQ) continue;
        torchPositions.push_back(kv.second);
    }

    // Crafted torches and fireplaces (plan_2 Step 4). These are objects, not
    // blocks, so neither torchLocations nor lightBlocks knows about them — without
    // this they would draw a glowing head that lights nothing around it, which is
    // exactly the failure plan_2 warned about.
    for (const glm::vec3& p : craftedLights) {
        float dx = p.x - camPos.x, dz = p.z - camPos.z;
        if (dx * dx + dz * dz > RENDER_DIST_SQ) continue;
        torchPositions.push_back({ p + glm::vec3(0, 0.8f, 0), COL_TORCH_FLAME });
    }
    for (const PlacedStructure& f : craftedFireplaces) {
        float dx = f.pos.x - camPos.x, dz = f.pos.z - camPos.z;
        if (dx * dx + dz * dz > RENDER_DIST_SQ) continue;
        // Sits low — the light comes from inside the surround, at ember height.
        torchPositions.push_back({ f.pos + glm::vec3(0, 0.35f, 0), COL_FIRE_HEARTH });
    }

    auto distSqToCam = [](const glm::vec3& p) {
        glm::vec3 d = p - camPos;
        return d.x * d.x + d.y * d.y + d.z * d.z;
    };
    // Only the nearest MAX_POINT_LIGHTS survive the upload, so only that many need
    // to be in order — partial_sort instead of a full sort, which matters once a
    // lit build has hundreds of glowstone blocks in range.
    const size_t keep = 8;
    if (torchPositions.size() > keep) {
        std::partial_sort(torchPositions.begin(), torchPositions.begin() + keep,
                          torchPositions.end(),
                          [&](const PointLightSrc& a, const PointLightSrc& b) {
                              return distSqToCam(a.pos) < distSqToCam(b.pos);
                          });
    } else {
        std::sort(torchPositions.begin(), torchPositions.end(),
                  [&](const PointLightSrc& a, const PointLightSrc& b) {
                      return distSqToCam(a.pos) < distSqToCam(b.pos);
                  });
    }
}

// =====================================================
// Ground scatter — grass tufts, flowers, pebbles, twigs
// =====================================================
// Sub-block detail on top of the terrain. None of it is stored: presence,
// position, size, orientation and tint are all derived from the column's
// coordinates, so 40,000 columns cost zero memory, the props never flicker
// between frames, and a block edit under a prop is picked up for free next frame
// because everything is recomputed from (col,row) anyway.
//
// The radius is deliberately much shorter than RENDER_DIST. A 4cm pebble is
// invisible at 50 units, so drawing it there is pure cost.
const float SCATTER_DIST = 28.0f;
const float SCATTER_DIST_SQ = SCATTER_DIST * SCATTER_DIST;
// Tufts get their own, shorter radius. They are the densest prop by a wide margin
// and cost three draws each, and a 3cm-wide blade is under a pixel past ~20 units
// anyway — so the grass thins out well before the pebbles do.
const float TUFT_DIST_SQ = 18.0f * 18.0f;

// Independent [0,1) values for one column. hashNoise returns (-1,1], and the
// multipliers keep the streams from correlating with each other or with the
// terrain noise that already samples this same function.
static inline float scatterHash(int col, int row, int stream) {
    return 0.5f * (hashNoise(col * 131 + stream * 7919, row * 197 + stream * 104729) + 1.0f);
}

// One grass blade / stem: a thin box pivoted at its base so it bends instead of
// sliding. windX/windZ are the same wind the vertex shader applies to foliage
// (see swayAmount in vertexShader.glsl) — expressed here as a lean angle rather
// than a displacement, because these are drawn live with their own model matrix
// and a rotation about the base is what a blade of grass actually does.
static void drawBlade(glm::vec3 base, float yaw, float lean, float height, float width,
                      float windX, float windZ, glm::vec3 color) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), base);
    // Wind first (outermost) so every blade in the field leans the same way...
    m = myRotate(m, windX, glm::vec3(0, 0, 1));
    m = myRotate(m, -windZ, glm::vec3(1, 0, 0));
    // ...then the blade's own turn about its (now tilted) axis, so a tuft fans out.
    m = myRotate(m, yaw, glm::vec3(0, 1, 0));
    // `lean` is what makes a tuft look like grass rather than like a row of tiny
    // fence posts. A blade standing straight up is a thin box whose only faces
    // point sideways, so it takes almost nothing from an overhead sun and renders
    // as a near-black splinter. Arching it over turns its widest face upward, and
    // it lights like the ground it is standing on.
    //
    // About X, not Z. The scale below is (width, height, width*0.45), so the two
    // big faces are the ones facing +/-Z — and leaning about Z swings the blade
    // within its own thin edge, leaving those faces exactly as vertical as before.
    // That was the first attempt, and the blades stayed black.
    m = myRotate(m, lean, glm::vec3(1, 0, 0));
    m = glm::translate(m, glm::vec3(0, height * 0.5f, 0));
    m = glm::scale(m, glm::vec3(width, height, width * 0.45f));
    drawBoxModel(m, color);
}

static void drawPebble(glm::vec3 base, float r, float yaw, glm::vec3 color) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, r * 0.30f, 0));
    m = myRotate(m, yaw, glm::vec3(0, 1, 0));
    m = glm::scale(m, glm::vec3(r, r * 0.55f, r * 0.85f));
    setMat4(shaderProgram, "model", m);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(sphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, sphereVertCount);
}

void drawGroundScatter(float time) {
    // Leaving whatever state the block loop was in. Scatter props are untextured
    // solid colours, they are not terrain so they must not take the procedural
    // tint, and their wind is applied CPU-side above — so the shader's own sway
    // has to be off or it would be applied twice.
    setInt(shaderProgram, "textureMode", 0);
    setBool(shaderProgram, "proceduralTint", false);
    setSway(0.0f);
    // Dull and broad: none of this is polished, and a tight highlight on a pebble
    // the size of a thumbnail just reads as a white speck.
    uploadSpecular(24.0f, 0.15f);

    const float zSp = HEX_RADIUS * 1.5f;
    const float xSp = HEX_RADIUS * 2.0f * 0.866f;
    int camRow = (int)roundf(camPos.z / zSp);
    int rowLimit = (int)(SCATTER_DIST / zSp) + 2;
    int minRow = std::max(TERRAIN_MIN, camRow - rowLimit);
    int maxRow = std::min(TERRAIN_MAX, camRow + rowLimit);

    for (int row = minRow; row <= maxRow; row++) {
        // Same per-row circular narrowing as renderTerrain(): at this row the
        // circle leaves only sqrt(R^2 - dz^2) of x budget.
        float dzRow = row * zSp - camPos.z;
        float xBudgetSq = SCATTER_DIST_SQ - dzRow * dzRow;
        if (xBudgetSq <= 0.0f) continue;
        float xBudget = sqrtf(xBudgetSq);
        float rowXOff = (row % 2) * (xSp * 0.5f);
        int minCol = std::max(TERRAIN_MIN, (int)ceilf((camPos.x - xBudget - rowXOff) / xSp) - 1);
        int maxCol = std::min(TERRAIN_MAX, (int)floorf((camPos.x + xBudget - rowXOff) / xSp) + 1);

        for (int col = minCol; col <= maxCol; col++) {
            // Presence test first and cheapest — one hash, no grid lookup, no
            // biome call, no distance maths. This is the loosest of the
            // thresholds below, so it only rejects columns that no surface type
            // would have accepted.
            float present = scatterHash(col, row, 0);
            if (present > 0.30f) continue;

            glm::vec3 pos = hexGridPos(col, row, 0.0f);
            float dx = pos.x - camPos.x;
            float dz = pos.z - camPos.z;
            float dSq = dx * dx + dz * dz;
            if (dSq > SCATTER_DIST_SQ) continue;

            // Find the surface. columnMaxH is an upper bound on the occupied
            // range, not necessarily a solid block, so walk down to the first one.
            int topH = columnMaxH[col + GRID_OFF_X][row + GRID_OFF_Z];
            int surf = -1;
            for (int h = topH; h >= 0; h--) {
                if (getBlock(col, row, h) != BLOCK_AIR) { surf = h; break; }
            }
            if (surf < 0) continue;

            int st = getBlock(col, row, surf);

            // Per-surface density, off the hash value we already have. Meadow
            // grass wants to look like a field, which needs far more props per
            // square metre than a gravel flat wants pebbles — one shared 8%
            // threshold gave a plausible pebble field and an invisible lawn.
            float limit;
            if (st == BLOCK_GRASS)                            limit = 0.30f;
            else if (st == BLOCK_DIRT)                        limit = 0.16f;
            else if (st == BLOCK_SAND)                        limit = 0.09f;
            else if (st == BLOCK_STONE || st == BLOCK_GRAVEL) limit = 0.11f;
            else if (st == BLOCK_SNOW)                        limit = 0.07f;
            else continue;                                    // not a surface we decorate
            if (present > limit) continue;

            // Don't decorate anything the player built. A stone platform or a dirt
            // pillar has the right top block but sits well off the generated
            // height, so comparing against the generator's own answer excludes
            // structures without needing to track them. Tolerance of 2 because
            // world-gen blends heights across biome borders after this call.
            int biome = getBiome(col, row);
            if (abs(surf - getTerrainHeightBiome(col, row, biome)) > 2) continue;

            // A block at index h is centred at h*HEX_HEIGHT and spans +/-0.5, so
            // its top face — what a prop stands on — is half a block higher.
            float surfY = surf * HEX_HEIGHT + HEX_HEIGHT * 0.5f;
            glm::vec3 base(pos.x + (scatterHash(col, row, 1) - 0.5f) * 0.52f,
                           surfY,
                           pos.z + (scatterHash(col, row, 2) - 0.5f) * 0.52f);

            if (!isInFrustum(base, 1.0f)) continue;

            float pick = scatterHash(col, row, 3);
            float size = scatterHash(col, row, 4);
            float yaw  = scatterHash(col, row, 5) * 6.2831853f;
            float tint = 0.85f + 0.30f * scatterHash(col, row, 6);

            // Wind, shared with the tree canopy so the whole landscape moves as one
            // system. Same coefficients as vertexShader.glsl's sway phase.
            float phase = time * 1.25f + base.x * 0.45f + base.z * 0.35f;
            float windX = sinf(phase) * 0.22f;
            float windZ = cosf(phase * 0.8f) * 0.22f;

            // What grows here is decided by what is underfoot, not by the biome
            // index — a patch of exposed dirt in the middle of grassland should
            // look like dirt.
            if (st == BLOCK_GRASS || st == BLOCK_DIRT) {
                bool grassy = (st == BLOCK_GRASS);
                if (grassy && pick > 0.90f) {
                    // Flower: a stem with a bloom on top. Rare on purpose — a
                    // meadow that is 30% flowers reads as a toy.
                    float stemH = 0.26f + size * 0.16f;
                    drawBlade(base, yaw, 0.10f, stemH, 0.030f, windX, windZ,
                              glm::vec3(0.30f, 0.48f, 0.20f) * tint);
                    const glm::vec3 petals[4] = {
                        {0.92f, 0.88f, 0.35f}, {0.88f, 0.32f, 0.34f},
                        {0.72f, 0.55f, 0.90f}, {0.95f, 0.95f, 0.92f}
                    };
                    const glm::vec3& pc = petals[(int)(scatterHash(col, row, 7) * 4.0f) & 3];
                    // The bloom rides on the bent stem, so it needs the stem's own
                    // lean and the wind applied to its offset rather than sitting
                    // straight above the base. Same two rotations drawBlade uses.
                    // Leaning about X sends the local tip to (0, h*cos, h*sin);
                    // the yaw then turns that offset into world XZ.
                    float lz = sinf(0.10f) * stemH;
                    glm::vec3 tip = base + glm::vec3(sinf(yaw) * lz + sinf(windX) * stemH,
                                                     cosf(0.10f) * stemH,
                                                     cosf(yaw) * lz + sinf(windZ) * stemH);
                    glm::mat4 m = glm::translate(glm::mat4(1.0f), tip);
                    m = myRotate(m, yaw, glm::vec3(0, 1, 0));
                    m = glm::scale(m, glm::vec3(0.075f, 0.045f, 0.075f));
                    drawBoxModel(m, pc);
                } else if (!grassy && pick > 0.55f) {
                    drawPebble(base, 0.055f + size * 0.045f, yaw,
                               glm::vec3(0.42f, 0.39f, 0.35f) * tint);
                } else if (dSq <= TUFT_DIST_SQ) {
                    // Tuft: three blades fanned out. Two would read as a "V" from
                    // the side; three is enough to look like a clump from any angle
                    // and is still only three draws.
                    //
                    // Deliberately darker and yellower than the grass block
                    // texture. Matching the ground colour made the tufts vanish
                    // entirely — a blade is nearly vertical, so the shader's AO
                    // hint already dims it relative to the block top it stands on,
                    // and starting from the same colour left nothing to see.
                    // Brighter than the grass block texture, not darker. A blade is
                    // still tilted well off horizontal even after the arch, so it
                    // takes maybe half the diffuse the flat ground beside it does;
                    // starting from the ground's own colour lands it at half
                    // brightness, which is what made the first two attempts read as
                    // dead twigs on a lawn.
                    glm::vec3 gc = glm::vec3(0.31f, 0.72f, 0.19f) * tint;
                    if (!grassy) gc = glm::vec3(0.50f, 0.52f, 0.26f) * tint;
                    float bh = 0.34f + size * 0.24f;
                    // Three different leans as well as three different yaws: a fan
                    // that all arches by the same amount still reads as one object.
                    drawBlade(base, yaw,         0.45f, bh,         0.055f, windX, windZ, gc);
                    drawBlade(base, yaw + 2.09f, 0.75f, bh * 0.84f, 0.048f, windX, windZ, gc * 0.86f);
                    drawBlade(base, yaw + 4.19f, 1.00f, bh * 0.70f, 0.044f, windX, windZ, gc * 1.16f);
                }
            } else if (st == BLOCK_SAND) {
                if (pick > 0.55f) {
                    // Dead twig: two straight segments meeting at an angle, lying
                    // almost flat. Rotating them about the base like grass would
                    // make them stand up, which driftwood does not.
                    glm::vec3 wc = glm::vec3(0.40f, 0.31f, 0.20f) * tint;
                    float len = 0.20f + size * 0.16f;
                    for (int s = 0; s < 2; s++) {
                        glm::mat4 m = glm::translate(glm::mat4(1.0f), base + glm::vec3(0, 0.022f, 0));
                        m = myRotate(m, yaw + s * 0.9f, glm::vec3(0, 1, 0));
                        m = glm::translate(m, glm::vec3(len * 0.5f, 0, 0));
                        m = glm::scale(m, glm::vec3(len, 0.035f, 0.035f));
                        drawBoxModel(m, wc * (s ? 0.88f : 1.0f));
                    }
                } else {
                    drawPebble(base, 0.05f + size * 0.04f, yaw,
                               glm::vec3(0.66f, 0.58f, 0.42f) * tint);
                }
            } else {
                // Stone, gravel, snow — pebbles only. Snow gets a paler, smaller
                // one, as a stone half-buried in it rather than sitting on top.
                float r = 0.06f + size * 0.05f;
                glm::vec3 pc = (st == BLOCK_SNOW) ? glm::vec3(0.62f, 0.65f, 0.70f)
                                                  : glm::vec3(0.46f, 0.45f, 0.43f);
                if (st == BLOCK_SNOW) r *= 0.75f;
                drawPebble(base, r, yaw, pc * tint);
            }
        }
    }

    resetBlockSpecular();
}

void renderTerrain(float time = 0.0f) {
    extractFrustum(currentVP);
    setFloat(shaderProgram, "alpha", 1.0f);
    waterDraws.clear();   // queue for the deferred transparent pass below
    // World-space noise tint, for terrain blocks only. Switched off again before
    // the tree and torch passes below — it is there to break up large runs of one
    // tiled block texture, and applying it to a tree or a prop would just make
    // that object blotchy.
    setBool(shaderProgram, "proceduralTint", true);

    // RENDER_DIST / RENDER_DIST_SQ now live in globals.h so gatherTorchLights()
    // and the fog density in main.cpp can agree with this loop.

    // Bound the loops to the columns that can actually fall within RENDER_DIST.
    // The old bound was `RENDER_DIST / HEX_RADIUS` on both axes, but HEX_RADIUS
    // (0.5) is not the grid spacing — rows step by 0.75 and columns by 0.866. That
    // made the search box ~1.5x too tall and ~1.7x too wide, and every extra column
    // it walked was thrown straight back out by the distance test below.
    const float zSp = HEX_RADIUS * 1.5f;           // row spacing
    const float xSp = HEX_RADIUS * 2.0f * 0.866f;  // column spacing — same constant hexGridPos() uses
    int camRow = (int)roundf(camPos.z / zSp);
    int rowLimit = (int)(RENDER_DIST / zSp) + 2;
    int minRow = std::max(TERRAIN_MIN, camRow - rowLimit);
    int maxRow = std::min(TERRAIN_MAX, camRow + rowLimit);

    for (int row = minRow; row <= maxRow; row++) {
        // Narrow the column span per row instead of using one square box. At this
        // row the circle of radius RENDER_DIST leaves only sqrt(R^2 - dz^2) of x
        // budget, so rows near the near/far edge of the span scan far fewer columns
        // than the row the camera sits on.
        float dz = row * zSp - camPos.z;
        float xBudgetSq = RENDER_DIST_SQ - dz * dz;
        if (xBudgetSq <= 0.0f) continue;           // row lies entirely outside the circle
        float xBudget = sqrtf(xBudgetSq);
        // Odd rows are offset half a column; note (row % 2) is -1 for negative odd
        // rows in C++, which is exactly what hexGridPos() does, so they agree.
        float rowXOff = (row % 2) * (xSp * 0.5f);
        // +/-1 of slack absorbs the 0.866 vs 0.866025404 rounding; the distance
        // test below is still the authority on what actually gets drawn.
        int minCol = std::max(TERRAIN_MIN, (int)ceilf((camPos.x - xBudget - rowXOff) / xSp) - 1);
        int maxCol = std::min(TERRAIN_MAX, (int)floorf((camPos.x + xBudget - rowXOff) / xSp) + 1);

        for (int col = minCol; col <= maxCol; col++) {
            glm::vec3 pos = hexGridPos(col, row, 0.0f);

            // Distance culling: skip entire column if too far from camera (XZ only)
            float dx = pos.x - camPos.x;
            float dz = pos.z - camPos.z;
            float distSq = dx * dx + dz * dz;
            if (distSq > RENDER_DIST_SQ) continue;

            // Use columnMaxH to skip iterating through mostly empty sky
            int maxH = columnMaxH[col + GRID_OFF_X][row + GRID_OFF_Z];
            for (int h = 0; h <= maxH; h++) {
                int bt = getBlock(col, row, h);
                if (bt == BLOCK_AIR) continue;

                glm::vec3 p = pos + glm::vec3(0, h * HEX_HEIGHT, 0);

                // Frustum culling FIRST — it is a handful of dot products, while
                // isBlockOccluded() does up to 8 getBlock() lookups. Testing the
                // cheap one first skips ~35k occlusion tests per frame.
                if (!isInFrustum(p)) continue;

                // Skip fully occluded blocks (all neighbors solid)
                if (isBlockOccluded(col, row, h)) continue;

                glm::vec3 col3 = getBlockColor(bt);
                BlockProperties props = getBlockProps(bt);
                uint16_t state = getBlockState(col, row, h);
                bool isOpen = (state >> 2) & 1;
                int facing  = state & 3;

                // Bind texture (or clear it)
                bindBlockTexture(bt);

                // Dispatch by shape
                if (bt == BLOCK_WATER) {
                    // Deferred to the transparent pass at the end of this
                    // function. Drawing water inline meant a glEnable/glDisable
                    // (GL_BLEND) pair PER BLOCK, with depth writes left on and
                    // blocks arriving in grid order — so whichever water hex
                    // happened to be drawn first blocked the ones behind it.
                    waterDraws.push_back({p, col3, distSq + (p.y - camPos.y) * (p.y - camPos.y)});
                } else if (props.isEmissive) {
                    drawHexEmissive(p, col3);
                } else if (props.shape == SHAPE_SLAB) {
                    bool topHalf = (state >> 3) & 1;
                    drawSlab(p, col3, topHalf);
                } else if (props.shape == SHAPE_STAIR) {
                    drawStair(p, col3, facing);
                } else if (props.shape == SHAPE_CARPET) {
                    drawHex(p + glm::vec3(0, -HEX_HEIGHT * 0.47f, 0), col3,
                            glm::vec3(1.0f, 0.06f, 1.0f));
                } else if (props.shape == SHAPE_FENCE) {
                    if (props.isInteractive) {
                        // Fence gate: two side posts + crossbar panel that swings open
                        float baseAngle = facing * PI / 2.0f;
                        float postW = HEX_RADIUS * 0.25f;
                        // Left post
                        glm::vec3 side(cosf(baseAngle), 0, -sinf(baseAngle));
                        drawHex(p + side * (HEX_RADIUS * 0.7f), col3, glm::vec3(postW / HEX_RADIUS, 1.0f, postW / HEX_RADIUS));
                        // Right post
                        drawHex(p - side * (HEX_RADIUS * 0.7f), col3, glm::vec3(postW / HEX_RADIUS, 1.0f, postW / HEX_RADIUS));
                        // Crossbar: closes straight across, opens by rotating 90 deg at one post
                        float barAngle = baseAngle + (isOpen ? PI / 2.0f : 0.0f);
                        glm::vec3 barOrig = isOpen ? (p + side * (HEX_RADIUS * 0.7f)) : p;
                        glm::mat4 mb = glm::translate(glm::mat4(1.0f), barOrig);
                        mb = myRotate(mb, barAngle, glm::vec3(0, 1, 0));
                        drawPanel(mb, col3 * 0.85f, HEX_RADIUS * 1.4f, HEX_HEIGHT * 0.55f);
                    } else {
                        // Regular fence: thin post
                        drawHex(p, col3, glm::vec3(0.3f, 1.0f, 0.3f));
                    }
                } else if (props.shape == SHAPE_SMALL_HEX) {
                    drawHexEmissive(p, col3);
                } else if (props.shape == SHAPE_DOOR) {
                    // Door: 2 blocks tall, offset to cell edge, with slight thickness
                    float baseAngle = facing * PI / 2.0f;
                    float openAngle = isOpen ? PI / 2.0f : 0.0f;
                    // Offset toward the face direction so door sits at edge of cell
                    glm::vec3 faceDir(sinf(baseAngle), 0.0f, cosf(baseAngle));
                    glm::vec3 doorPos = p + faceDir * (HEX_RADIUS * 0.45f);
                    // Raise so door spans from bottom of this block to top of next block
                    doorPos.y = p.y + HEX_HEIGHT * 0.5f;
                    glm::mat4 m = glm::translate(glm::mat4(1.0f), doorPos);
                    m = myRotate(m, baseAngle + openAngle, glm::vec3(0, 1, 0));
                    // Draw front panel
                    drawPanel(m, col3, HEX_RADIUS * 1.8f, HEX_HEIGHT * 2.0f);
                    // Draw thin back panel slightly offset for thickness illusion
                    glm::mat4 mBack = glm::translate(m, glm::vec3(0, 0, -0.06f));
                    drawPanel(mBack, col3 * 0.75f, HEX_RADIUS * 1.8f, HEX_HEIGHT * 2.0f);
                } else if (props.shape == SHAPE_PANE || props.shape == SHAPE_FLAT_PANEL) {
                    // Thin flat panel for glass panes, iron bars, ladders, signs
                    float baseAngle = facing * PI / 2.0f;
                    glm::mat4 m = glm::translate(glm::mat4(1.0f), p);
                    m = myRotate(m, baseAngle, glm::vec3(0, 1, 0));
                    drawPanel(m, col3, HEX_RADIUS * 1.8f, HEX_HEIGHT);
                } else if (props.shape == SHAPE_TRAPDOOR) {
                    // Trapdoor: horizontal panel at top of block, flips vertical when open
                    float baseAngle = facing * PI / 2.0f;
                    glm::vec3 tdPos = p + glm::vec3(0, HEX_HEIGHT * 0.45f, 0);
                    glm::mat4 m = glm::translate(glm::mat4(1.0f), tdPos);
                    m = myRotate(m, baseAngle, glm::vec3(0, 1, 0));
                    if (isOpen)
                        m = myRotate(m, -PI / 2.0f, glm::vec3(1, 0, 0));
                    drawPanel(m, col3, HEX_RADIUS * 1.8f, HEX_RADIUS * 1.8f);
                    drawPanel(glm::translate(m, glm::vec3(0, 0, -0.06f)), col3 * 0.75f, HEX_RADIUS * 1.8f, HEX_RADIUS * 1.8f);
                } else {
                    drawHex(p, col3);
                }
                // Reset texture mode after each block
                setInt(shaderProgram, "textureMode", 0);
            }
        }
    }

    // Ground scatter, straight after the blocks it sits on. Before the tree pass
    // rather than after, so it does not have to undo the tree pass's sway setting.
    drawGroundScatter(time);

    // Draw trees (non-grid decorative objects) — with distance + frustum culling
    // Radius clamped to RENDER_DIST: drawing trees further out than the terrain
    // put them above ground that does not exist.
    const float TREE_RENDER_DIST_SQ = RENDER_DIST_SQ;
    // Leaving the block loop: drop whatever specular response the last block set,
    // so trees are not shaded as though they were made of that block, and switch
    // the terrain noise tint back off for the same reason.
    resetBlockSpecular();
    setBool(shaderProgram, "proceduralTint", false);
    // Wind, for the tree pass. The vertex shader masks this by leafiness and by
    // height up the trunk, so the amplitude here is what the outermost canopy
    // vertices get, not what the whole tree moves by.
    //
    // Baked meshes only. The mask reads aColor.r (the wood->leaf blend) and
    // aPos.y (height up the trunk), and both of those only mean that on a baked
    // tree mesh — on the live fractal path every part is drawn from the same unit
    // hex with its own model matrix, so aPos.y is a vertex offset within one
    // block and carries no information about where it sits on the tree. Applying
    // sway there would translate whole limbs rigidly. F7 (live fractal mode) is a
    // performance A/B toggle, so trading the wind for it is the right way round.
    setSway((useBakedTrees && g_treeMeshesReady) ? 0.09f : 0.0f);
    // Set colorMode/woodColor lazily, only once a tree actually survives culling.
    bool bakedTreesActive = false;
    for (auto& ti : treeLocations) {
        glm::vec3 pos = hexGridPos(ti.col, ti.row, 0.0f);
        // Distance cull trees
        float tdx = pos.x - camPos.x;
        float tdz = pos.z - camPos.z;
        if (tdx * tdx + tdz * tdz > TREE_RENDER_DIST_SQ) continue;

        // groundH was resolved once at startup (resolveGroundHeights); this used
        // to be a 32-step column rescan here, every frame, for every tree.
        glm::vec3 treeBase = pos + glm::vec3(0, (ti.groundH + 1) * HEX_HEIGHT, 0);
        // Frustum cull against a sphere big enough to cover trunk + canopy
        glm::vec3 treeCenter = treeBase + glm::vec3(0, 3.0f, 0);
        if (!isInFrustum(treeCenter, ti.large ? 7.0f : 4.5f)) continue;

        if (useBakedTrees && g_treeMeshesReady) {
            // One draw call per tree instead of ~1,190. The mesh is colour-free;
            // the leaf colour arrives as a uniform (see colorMode in the shaders).
            const TreeMesh& m = ti.large
                ? g_largeTreeMeshes[ti.variant % NUM_LARGE_TREE_VARIANTS]
                : g_treeMeshes[ti.variant % NUM_TREE_VARIANTS];
            // drawLargeTree() hardcodes its green and ignores TreeInfo::leafColor,
            // so baked large trees must use that same colour.
            setVec3(shaderProgram, "leafColor",
                    ti.large ? glm::vec3(0.15f, 0.55f, 0.12f) : ti.leafColor);
            setMat4(shaderProgram, "model", glm::translate(glm::mat4(1.0f), treeBase));
            if (!bakedTreesActive) {
                setInt(shaderProgram, "colorMode", 1);
                setVec3(shaderProgram, "woodColor", COL_WOOD_DARK);
                bakedTreesActive = true;
            }
            glBindVertexArray(m.vao);
            glDrawArrays(GL_TRIANGLES, 0, m.count);
        } else if (ti.large) {
            drawLargeTree(treeBase);
        } else {
            drawTree(treeBase, ti.leafColor);
        }
    }
    if (bakedTreesActive) setInt(shaderProgram, "colorMode", 0);
    // Wind off again — torches, props and mobs are not foliage.
    setSway(0.0f);

    // Draw torches — with distance + frustum culling (radius clamped to RENDER_DIST)
    const float TORCH_RENDER_DIST_SQ = RENDER_DIST_SQ;
    for (auto& t : torchLocations) {
        glm::vec3 pos = hexGridPos(t.col, t.row, 0.0f);
        float tdx = pos.x - camPos.x;
        float tdz = pos.z - camPos.z;
        if (tdx * tdx + tdz * tdz > TORCH_RENDER_DIST_SQ) continue;

        // t.height is the block the torch stands on, recorded at world-gen. The
        // old code rescanned the column and used the topmost NON-AIR block, so a
        // torch whose own flame block (or any later structure) sat above it got
        // pushed one step higher every time — that is the mid-air torch bug.
        glm::vec3 torchBase = pos + glm::vec3(0, (t.height + 1) * HEX_HEIGHT, 0);
        // The light itself is registered by gatherTorchLights(), which runs before
        // the draw pass; this loop only draws geometry, so it may frustum-cull.
        if (!isInFrustum(torchBase, 2.0f)) continue;
        drawTorchMesh(torchBase);
    }

    // Stone Ruins (east, explorable)
    {
        int rc = 55, rr = -10;
        glm::vec3 rp = hexGridPos(rc, rr, 0.0f);
        // Resolved once, not rescanned every frame.
        static int ruinsGroundH = -1;
        if (ruinsGroundH < 0) ruinsGroundH = std::max(findGroundH(rc, rr), 0);
        drawRuins(rp + glm::vec3(0, (ruinsGroundH + 1) * HEX_HEIGHT, 0));
    }

    // Ambulance
    {
        if (!ambInitialized) {
            int ac = 13, ar = -14;
            ambPos = hexGridPos(ac, ar, 0.0f);
            // The body is ~2 hexes across and ~5 long, so one column's height is
            // not enough — rest it on the HIGHEST ground under the footprint or
            // the uphill wheels hang in the air.
            int groundH = -1;
            for (int dc = -2; dc <= 2; dc++)
                for (int dr = -6; dr <= 6; dr++)
                    groundH = std::max(groundH, findGroundH(ac + dc, ar + dr));
            if (groundH < 0) groundH = UNDERGROUND_DEPTH;
            // A block at index h is drawn CENTRED at h*HEX_HEIGHT and spans +/-0.5,
            // so its top surface is h*HEX_HEIGHT + HEX_HEIGHT/2. The old code used
            // (h+1)*HEX_HEIGHT — half a block too high. The model's lowest voxel
            // row is also centred on its own origin, so half a voxel has to come
            // back down too. Together that is the gap under the ambulance.
            const float wheelDrop = (HEX_RADIUS * 0.32f / HEX_HEIGHT * 1.15f) * 0.5f;
            ambPos.y = groundH * HEX_HEIGHT + HEX_HEIGHT * 0.5f + wheelDrop;
            ambInitialized = true;
            printf("[Ambulance] Grounded at grid (%d,%d): groundH=%d y=%.2f\n",
                   ac, ar, groundH, ambPos.y);
        }
        drawAmbulance(ambPos, ambYaw);
    }

    // Transparent geometry last, once everything opaque has written depth.
    flushWaterPass(time);
}

// =====================================================
// Sky Rendering — sun, moon, stars, clouds
// =====================================================

// Deterministic star positions (generated once)
struct StarInfo { float azimuth, elevation; float brightness; };
const int NUM_STARS = 120;
StarInfo starField[NUM_STARS];
bool starsInitialized = false;

void initStars() {
    for (int i = 0; i < NUM_STARS; i++) {
        unsigned int s = gridSeed(i * 37, i * 53 + 7);
        starField[i].azimuth = (float)(s % 36000) / 100.0f; // 0-360 degrees
        s = s * 1103515245 + 12345;
        starField[i].elevation = 15.0f + (float)(s % 6000) / 100.0f; // 15-75 degrees above horizon
        s = s * 1103515245 + 12345;
        starField[i].brightness = 0.4f + (float)(s % 60) / 100.0f; // 0.4-1.0
    }
    starsInitialized = true;
}

// =====================================================
// KUET Hill — scenic hill in the NE corner of the map
// =====================================================
void buildKUETHill() {
    // ---------- configuration ----------
    const int HC       = 60;   // NE corner, well away from spawn (3,5)
    const int HR       = -55;

    const int HILL_R   = 30;   // hill footprint radius — wide enough to contain all 49 cols of letters
    const int HILL_H   = 7;    // how many blocks the summit rises above local terrain

    // Scale: each font pixel → SW cols wide, SH blocks tall
    // Half-height vs. before: SH=1 → 7 blocks tall  SW=2 → 10 cols wide
    const int SW      = 2;
    const int SH      = 1;     // half the old height (was 3)
    const int L_DEPTH = 3;     // depth (3-D thickness in rows)
    const int L_GAP   = 3;     // column gap between letters
    // total sign width = 4*(5*SW) + 3*L_GAP = 40+9 = 49 cols

    // ---------- Step 1: find base terrain height at centre ----------
    int baseH = 1;
    for (int h = GRID_H - 1; h >= 0; h--) {
        if (getBlock(HC, HR, h) != BLOCK_AIR) { baseH = h; break; }
    }
    int hilltopH = baseH + HILL_H;
    if (hilltopH >= GRID_H - 8) hilltopH = GRID_H - 9; // leave room for letters

    // ---------- Step 2: sculpt the hill ----------
    for (int dc = -HILL_R; dc <= HILL_R; dc++) {
        for (int dr = -HILL_R; dr <= HILL_R; dr++) {
            int c = HC + dc, r = HR + dr;
            if (!gridInBounds(c, r, 0)) continue;
            float dist = sqrtf((float)(dc*dc + dr*dr));
            if (dist > HILL_R) continue;

            // Smooth cosine dome — full height at centre, fades to zero at edge
            float t = 0.5f * (1.0f + cosf(dist / HILL_R * 3.14159f));
            int profileH = baseH + (int)(t * HILL_H + 0.5f);
            if (profileH >= GRID_H) profileH = GRID_H - 1;

            // Clear stray blocks above profile
            for (int h = profileH + 1; h < GRID_H; h++)
                if (getBlock(c, r, h) != BLOCK_AIR)
                    setBlock(c, r, h, BLOCK_AIR);

            // Fill from current top up to profile
            int curTop = 0;
            for (int h = GRID_H - 1; h >= 0; h--)
                if (getBlock(c, r, h) != BLOCK_AIR) { curTop = h; break; }

            for (int h = curTop + 1; h <= profileH; h++)
                setBlock(c, r, h, BLOCK_DIRT);

            // Grass cap on the surface
            setBlock(c, r, profileH, BLOCK_GRASS);
        }
    }

    // ---------- Step 3: KUET letters ----------
    // Bitmaps: row 0 = bottom, row 6 = top; col 0 = left (west)
    //
    // K — clean diagonal arms, single apex pixel at centre
    static const uint8_t K[7][5] = {
        {1,0,0,0,1},   // row 0  bot
        {1,0,0,1,0},   // row 1
        {1,0,1,0,0},   // row 2
        {1,1,0,0,0},   // row 3  mid — single apex, NOT a wide bar
        {1,0,1,0,0},   // row 4
        {1,0,0,1,0},   // row 5
        {1,0,0,0,1},   // row 6  top
    };
    static const uint8_t U[7][5] = {
        {0,1,1,1,0},   // row 0  bot
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},   // row 6  top
    };
    static const uint8_t E[7][5] = {
        {1,1,1,1,1},   // row 0  bot
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,1,1,1,0},   // row 3  mid-bar (4 wide, not 5)
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,1,1,1,1},   // row 6  top
    };
    static const uint8_t T[7][5] = {
        {0,0,1,0,0},   // row 0  bot
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {1,1,1,1,1},   // row 6  top bar
    };
    const uint8_t (*glyphs[4])[5] = { K, U, E, T };

    // Centre sign horizontally on HC; faces south so player walks toward it
    const int totalW = 4 * (5 * SW) + 3 * L_GAP;  // = 49
    int startC = HC - totalW / 2;
    int startR = HR;    // front (south) face at startR + L_DEPTH - 1

    for (int li = 0; li < 4; li++) {
        int cOff = li * (5 * SW + L_GAP);
        for (int lr = 0; lr < 7; lr++) {       // font row  0=bottom
            for (int lc = 0; lc < 5; lc++) {   // font col  0=left
                if (!glyphs[li][lr][lc]) continue;
                for (int sc = 0; sc < SW; sc++) {
                    int gc = startC + cOff + lc * SW + sc;
                    for (int sh = 0; sh < SH; sh++) {
                        int gh = hilltopH + 1 + lr * SH + sh;
                        if (gh >= GRID_H) continue;
                        for (int d = 0; d < L_DEPTH; d++) {
                            setBlock(gc, startR + d, gh, BLOCK_QUARTZ_BLOCK);
                        }
                    }
                }
            }
        }
    }

    int endC = startC + totalW - 1;

    // ---------- Step 4: remove trees from the hill area ----------
    // Trees are collected during terrain generation before this function runs,
    // so any tree whose (col,row) falls inside the hill must be pruned.
    for (int i = (int)treeLocations.size() - 1; i >= 0; i--) {
        int dc = treeLocations[i].col - HC;
        int dr = treeLocations[i].row - HR;
        float dist = sqrtf((float)(dc * dc + dr * dr));
        if (dist <= HILL_R + 4) {   // +4 margin so no canopy overhangs the letters
            treeLocations.erase(treeLocations.begin() + i);
        }
    }

    printf("[World] KUET sign built — hill (%d,%d) hilltop h=%d, cols %d..%d, h %d..%d\n",
           HC, HR, hilltopH, startC, endC, hilltopH+1, hilltopH+7*SH);
}

void renderSky(float time, glm::vec3 sunDir) {
    if (!starsInitialized) initStars();

    // Disable depth write so sky is always behind everything
    glDepthMask(GL_FALSE);
    setBool(shaderProgram, "isEmissive", true);

    float skyDist = 150.0f; // distance from camera to sky objects

    // Celestial bodies are drawn at skyDist, three times the fog cutoff, so
    // distance fog erases them completely — the moon and stars simply never
    // appeared at night. They are meant to read as infinitely far away, which is
    // exactly the thing fog should not be applied to.
    setFloat(shaderProgram, "fogDensity", 0.0f);

    // ============ SUN ============
    if (dayFactor > 0.1f) {
        // Sun position: opposite of light direction, far away
        glm::vec3 sunPos = camPos - glm::normalize(sunDir) * skyDist;

        // Sun glow color based on time of day
        glm::vec3 sunColor;
        if (dayMode == 1) // dawn
            sunColor = glm::vec3(1.0f, 0.6f, 0.2f);
        else if (dayMode == 3) // dusk
            sunColor = glm::vec3(1.0f, 0.4f, 0.15f);
        else // noon
            sunColor = glm::vec3(1.0f, 0.95f, 0.7f);

        // Sun disc (large emissive sphere made of hex)
        setVec3(shaderProgram, "emissiveColor", sunColor);
        drawHex(sunPos, sunColor, glm::vec3(6.0f, 6.0f, 6.0f));
        // Sun corona (slightly larger, dimmer)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        setFloat(shaderProgram, "alpha", 0.3f);
        setVec3(shaderProgram, "emissiveColor", sunColor * 0.5f);
        drawHex(sunPos, sunColor * 0.5f, glm::vec3(10.0f, 10.0f, 10.0f));
        setFloat(shaderProgram, "alpha", 1.0f);
        glDisable(GL_BLEND);
    }

    // ============ MOON ============
    if (dayFactor < 0.5f) {
        // Moon on opposite side from sun
        glm::vec3 moonDir = glm::normalize(sunDir); // same direction as light = opposite side of sky
        glm::vec3 moonPos = camPos + glm::vec3(moonDir.x, fabsf(moonDir.y) + 0.3f, moonDir.z) * skyDist;

        float moonAlpha = (dayFactor < 0.2f) ? 1.0f : (0.5f - dayFactor) / 0.3f;
        glm::vec3 moonColor(0.85f, 0.88f, 0.95f);

        setVec3(shaderProgram, "emissiveColor", moonColor * moonAlpha);
        drawHex(moonPos, moonColor * moonAlpha, glm::vec3(4.0f, 4.0f, 4.0f));
    }

    // ============ STARS ============
    if (dayFactor < 0.3f) {
        float starAlpha = (dayFactor < 0.1f) ? 1.0f : (0.3f - dayFactor) / 0.2f;
        // Slowly rotate stars
        float starRotation = time * 0.5f; // degrees per second

        for (int i = 0; i < NUM_STARS; i++) {
            float az = glm::radians(starField[i].azimuth + starRotation);
            float el = glm::radians(starField[i].elevation);

            glm::vec3 starPos = camPos + glm::vec3(
                cosf(el) * cosf(az),
                sinf(el),
                cosf(el) * sinf(az)
            ) * skyDist;

            float twinkle = 0.7f + 0.3f * sinf(time * 3.0f + (float)i * 1.7f);
            float b = starField[i].brightness * starAlpha * twinkle;
            glm::vec3 sc(b, b, b * 0.95f);

            setVec3(shaderProgram, "emissiveColor", sc);
            drawHex(starPos, sc, glm::vec3(0.5f + b * 0.3f));
        }
    }

    // ============ CLOUDS ============
    // Removed in 19D-6. There were two cloud systems visible at the same time:
    // these drifting hex blobs and the painted clouds in the skybox cubemap
    // (docs/bug_evidence/08_skybox_seams_double_clouds.png). The hex clouds were
    // the worse of the two — drawn with isEmissive off, so their undersides lit
    // dark and they read as floating rocks rather than cloud — so the cubemap
    // won. The cubemap is now generated seamlessly by tools/gen_skybox.py, which
    // is also where the clouds live. Sun, moon and stars stay here: they are not
    // duplicated in the cubemap and they need to parallax against it.

    setFloat(shaderProgram, "fogDensity", currentFogDensity); // restore scene fog
    setBool(shaderProgram, "isEmissive", false);
    glDepthMask(GL_TRUE);
}


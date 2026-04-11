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
void setBlock(int col, int row, int h, int type) {
    if (!gridInBounds(col, row, h)) return;
    int gi = col + GRID_OFF_X, gj = row + GRID_OFF_Z;
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

        default: return glm::vec3(1, 0, 1); // magenta = error
    }
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
// Trees — BIG, colorful, with thick trunk and large canopy
// =====================================================
void drawTree(glm::vec3 base, glm::vec3 leafColor) {
    // Thick trunk: 2 hex wide, 6 tall
    int trunkH = 6;
    for (int i = 0; i < trunkH; i++) {
        glm::vec3 p = base + glm::vec3(0, i * HEX_HEIGHT, 0);
        float v = 0.9f + 0.1f * hashNoise((int)(base.x * 10), i);
        // Main trunk (thick)
        drawHex(p, COL_WOOD_DARK * v, glm::vec3(0.9f, 1.0f, 0.9f));
        // Trunk buttress at base and lower sections
        if (i < 2) {
            drawHex(p + glm::vec3(0.35f, 0, 0), COL_WOOD_DARK * v, glm::vec3(0.45f, 0.7f, 0.45f));
            drawHex(p + glm::vec3(-0.35f, 0, 0.2f), COL_WOOD_DARK * v, glm::vec3(0.45f, 0.7f, 0.45f));
            drawHex(p + glm::vec3(0.0f, 0, -0.35f), COL_WOOD_DARK * v, glm::vec3(0.4f, 0.6f, 0.4f));
        }
        // Branch stubs mid-trunk
        if (i == 3) {
            drawHex(p + glm::vec3(0.5f, 0.2f, 0), COL_WOOD_DARK * v, glm::vec3(0.3f, 0.4f, 0.15f));
            drawHex(p + glm::vec3(-0.4f, 0.2f, 0.3f), COL_WOOD_DARK * v, glm::vec3(0.3f, 0.4f, 0.15f));
        }
    }

    // Large canopy: 5 layers, radius up to 4
    float topY = base.y + trunkH * HEX_HEIGHT;
    unsigned int seed = gridSeed((int)(base.x * 10), (int)(base.z * 10));
    glm::vec3 leafDark = leafColor * 0.7f;
    glm::vec3 leafBright = leafColor * 1.15f;

    for (int dy = 0; dy < 5; dy++) {
        int radius;
        if (dy == 0 || dy == 4) radius = 2;
        else if (dy == 2) radius = 4;
        else radius = 3;

        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                // Circular shape
                if (dx * dx + dz * dz > radius * radius + 1) continue;

                seed = seed * 1103515245 + 12345;
                if ((seed >> 16) % 10 < 1) continue; // 10% holes

                float ox = dx * HEX_RADIUS * 1.5f;
                float oz = dz * HEX_RADIUS * 1.5f;
                glm::vec3 lp(base.x + ox, topY + dy * HEX_HEIGHT * 0.65f, base.z + oz);

                glm::vec3 lc = ((seed >> 8) % 4 == 0) ? leafDark :
                               ((seed >> 8) % 4 == 3) ? leafBright : leafColor;
                drawHex(lp, lc, glm::vec3(0.6f, 0.6f, 0.6f));
            }
        }
    }
}

// Standalone large tree (green, even bigger)
void drawLargeTree(glm::vec3 base) {
    int trunkH = 8;
    for (int i = 0; i < trunkH; i++) {
        glm::vec3 p = base + glm::vec3(0, i * HEX_HEIGHT, 0);
        float v = 0.9f + 0.1f * hashNoise((int)(base.x * 10), i);
        // Extra thick trunk (2 hexes wide)
        drawHex(p, COL_WOOD_DARK * v, glm::vec3(1.1f, 1.0f, 1.1f));
        if (i < 3) {
            drawHex(p + glm::vec3(0.45f, 0, 0.2f), COL_WOOD_DARK * v, glm::vec3(0.55f, 0.7f, 0.55f));
            drawHex(p + glm::vec3(-0.4f, 0, -0.3f), COL_WOOD_DARK * v, glm::vec3(0.55f, 0.7f, 0.55f));
            drawHex(p + glm::vec3(-0.1f, 0, 0.45f), COL_WOOD_DARK * v, glm::vec3(0.5f, 0.65f, 0.5f));
        }
        // Branch stubs at mid and upper trunk
        if (i == 4 || i == 6) {
            drawHex(p + glm::vec3(0.6f, 0.15f, 0), COL_WOOD_DARK * v, glm::vec3(0.35f, 0.45f, 0.15f));
            drawHex(p + glm::vec3(-0.5f, 0.15f, 0.4f), COL_WOOD_DARK * v, glm::vec3(0.35f, 0.45f, 0.15f));
        }
    }
    float topY = base.y + trunkH * HEX_HEIGHT;
    unsigned int seed = gridSeed((int)(base.x * 7), (int)(base.z * 7));
    glm::vec3 green(0.15f, 0.55f, 0.12f);
    glm::vec3 greenD(0.1f, 0.4f, 0.08f);
    glm::vec3 greenB(0.2f, 0.65f, 0.18f);
    for (int dy = 0; dy < 6; dy++) {
        int radius;
        if (dy == 0 || dy == 5) radius = 3;
        else if (dy == 2 || dy == 3) radius = 6;
        else radius = 5;
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                if (dx * dx + dz * dz > radius * radius + 1) continue;
                seed = seed * 1103515245 + 12345;
                if ((seed >> 16) % 12 < 1) continue; // fewer holes
                float ox = dx * HEX_RADIUS * 1.4f;
                float oz = dz * HEX_RADIUS * 1.4f;
                glm::vec3 lc = ((seed >> 8) % 4 == 0) ? greenD :
                               ((seed >> 8) % 4 == 3) ? greenB : green;
                drawHex(glm::vec3(base.x + ox, topY + dy * HEX_HEIGHT * 0.6f, base.z + oz),
                        lc, glm::vec3(0.55f, 0.55f, 0.55f));
            }
        }
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
std::vector<glm::vec3> torchPositions;
const glm::vec3 COL_TORCH_FLAME(1.0f, 0.6f, 0.1f);

void drawTorch(glm::vec3 base) {
    drawHex(base + glm::vec3(0, 0.3f, 0), COL_WOOD, glm::vec3(0.15f, 0.6f, 0.15f));
    drawHexEmissive(base + glm::vec3(0, 0.7f, 0), COL_TORCH_FLAME, glm::vec3(0.2f, 0.25f, 0.2f));
    torchPositions.push_back(base + glm::vec3(0, 0.8f, 0));
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
struct TreeInfo { int col, row; glm::vec3 leafColor; bool large; };
std::vector<TreeInfo> treeLocations;
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
    treeLocations.push_back({c0+W/2 - 8, r1-10, glm::vec3(0.2f,0.65f,0.15f), false});
    treeLocations.push_back({c0+W/2 + 8, r1-12, glm::vec3(0.15f,0.55f,0.12f), false});

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
                    treeLocations.push_back(ti);
                }
            } else if (biome == 1 && surfaceH >= UNDERGROUND_DEPTH + 1
                       && surfaceH < UNDERGROUND_DEPTH + 10 && (seed % 80 == 0)) {
                TreeInfo ti;
                ti.col = col; ti.row = row;
                ti.leafColor = TREE_COLORS[seed % NUM_TREE_COLORS];
                ti.large = (seed % 3 == 0);
                treeLocations.push_back(ti);
            }
            // Sparse trees on stone biome at mid-altitude
            if (biome == 2 && surfaceH >= UNDERGROUND_DEPTH + 4
                && surfaceH < UNDERGROUND_DEPTH + 12 && (seed % 120 == 0)) {
                TreeInfo ti;
                ti.col = col; ti.row = row;
                ti.leafColor = glm::vec3(0.12f, 0.4f, 0.1f); // dark green pine
                ti.large = false;
                treeLocations.push_back(ti);
            }

            // ======== TORCHES ========
            if (biome == 1 && surfaceH >= UNDERGROUND_DEPTH + 1 && (seed % 61 == 0)) {
                TorchInfo t; t.col = col; t.row = row; t.height = surfaceH + 2;
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

bool isBlockOccluded(int col, int row, int h) {
    // Check top and bottom
    if (getBlock(col, row, h + 1) == BLOCK_AIR) return false;
    if (h == 0 || getBlock(col, row, h - 1) == BLOCK_AIR) return false;

    // Check 6 hex neighbors
    bool odd = (((row % 2) + 2) % 2) == 1; // handle negative rows
    const int (*nb)[2] = odd ? hexNeighborOdd : hexNeighborEven;
    for (int i = 0; i < 6; i++) {
        if (getBlock(col + nb[i][0], row + nb[i][1], h) == BLOCK_AIR)
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
        case BLOCK_DOOR_OAK:      return texOakPlanks;
        case BLOCK_TRAPDOOR_OAK:  return texOakPlanks;
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
        // All wool, carpets, terracotta, concrete — use color only (no FM texture)
        default: return 0;
    }
}

// Bind a texture for block rendering; returns textureMode to set
// mode 1 = texture only (stone, cobble — self-colored)
// mode 2 = texture*color (planks, wool blended)
int bindBlockTexture(int type) {
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
void renderTerrain(float time = 0.0f) {
    torchPositions.clear();
    extractFrustum(currentVP);
    setFloat(shaderProgram, "alpha", 1.0f);

    // Render distance: skip columns too far from camera (huge FPS boost)
    // Set larger than fog fade distance so terrain never pops in visibly
    const float RENDER_DIST = 75.0f;
    const float RENDER_DIST_SQ = RENDER_DIST * RENDER_DIST;

    // Bound the loops to only check columns within a box of RENDER_DIST around camera
    float zSp = HEX_RADIUS * 1.5f;
    float xSp = HEX_RADIUS * 2.0f * 0.866025404f;
    int camRow = (int)roundf(camPos.z / zSp);
    float xOff = (camRow % 2) * (xSp * 0.5f);
    int camCol = (int)roundf((camPos.x - xOff) / xSp);
    int hexLimit = (int)(RENDER_DIST / HEX_RADIUS) + 2;
    int minRow = std::max(TERRAIN_MIN, camRow - hexLimit);
    int maxRow = std::min(TERRAIN_MAX, camRow + hexLimit);
    int minCol = std::max(TERRAIN_MIN, camCol - hexLimit);
    int maxCol = std::min(TERRAIN_MAX, camCol + hexLimit);

    for (int row = minRow; row <= maxRow; row++) {
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

                // Skip fully occluded blocks (all neighbors solid)
                if (isBlockOccluded(col, row, h)) continue;

                glm::vec3 p = pos + glm::vec3(0, h * HEX_HEIGHT, 0);

                // Frustum culling
                if (!isInFrustum(p)) continue;

                glm::vec3 col3 = getBlockColor(bt);
                BlockProperties props = getBlockProps(bt);
                uint16_t state = getBlockState(col, row, h);
                bool isOpen = (state >> 2) & 1;
                int facing  = state & 3;

                // Bind texture (or clear it)
                bindBlockTexture(bt);

                // Dispatch by shape
                if (bt == BLOCK_WATER) {
                    setInt(shaderProgram, "textureMode", 0);
                    drawHexWater(p, col3, time);
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
                    // Thin vertical post
                    drawHex(p, col3, glm::vec3(0.3f, 1.0f, 0.3f));
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

    // Draw trees (non-grid decorative objects) — with distance culling
    const float TREE_RENDER_DIST_SQ = 70.0f * 70.0f;
    for (auto& ti : treeLocations) {
        glm::vec3 pos = hexGridPos(ti.col, ti.row, 0.0f);
        // Distance cull trees
        float tdx = pos.x - camPos.x;
        float tdz = pos.z - camPos.z;
        if (tdx * tdx + tdz * tdz > TREE_RENDER_DIST_SQ) continue;

        int topH = 0;
        for (int h = GRID_H - 1; h >= 0; h--) {
            if (getBlock(ti.col, ti.row, h) != BLOCK_AIR) { topH = h; break; }
        }
        glm::vec3 treeBase = pos + glm::vec3(0, (topH + 1) * HEX_HEIGHT, 0);
        if (ti.large)
            drawLargeTree(treeBase);
        else
            drawTree(treeBase, ti.leafColor);
    }

    // Draw torches — with distance culling
    const float TORCH_RENDER_DIST_SQ = 65.0f * 65.0f;
    for (auto& t : torchLocations) {
        glm::vec3 pos = hexGridPos(t.col, t.row, 0.0f);
        float tdx = pos.x - camPos.x;
        float tdz = pos.z - camPos.z;
        if (tdx * tdx + tdz * tdz > TORCH_RENDER_DIST_SQ) continue;

        int topH = 0;
        for (int h = GRID_H - 1; h >= 0; h--) {
            if (getBlock(t.col, t.row, h) != BLOCK_AIR) { topH = h; break; }
        }
        drawTorch(pos + glm::vec3(0, (topH + 1) * HEX_HEIGHT, 0));
    }

    // Stone Ruins (east, explorable)
    {
        int rc = 55, rr = -10;
        glm::vec3 rp = hexGridPos(rc, rr, 0.0f);
        int topH = 0;
        for (int h = GRID_H - 1; h >= 0; h--) {
            if (getBlock(rc, rr, h) != BLOCK_AIR) { topH = h; break; }
        }
        drawRuins(rp + glm::vec3(0, (topH + 1) * HEX_HEIGHT, 0));
    }
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

void renderSky(float time, glm::vec3 sunDir) {
    if (!starsInitialized) initStars();

    // Disable depth write so sky is always behind everything
    glDepthMask(GL_FALSE);
    setBool(shaderProgram, "isEmissive", true);

    float skyDist = 150.0f; // distance from camera to sky objects

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
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float cloudAlpha = (dayFactor > 0.3f) ? 0.6f : 0.2f;
        setFloat(shaderProgram, "alpha", cloudAlpha);

        // Cloud color: white in day, dark blue-gray at night
        glm::vec3 cloudDay(0.95f, 0.95f, 0.98f);
        glm::vec3 cloudNight(0.15f, 0.15f, 0.25f);
        glm::vec3 cloudColor = cloudNight + dayFactor * (cloudDay - cloudNight);

        setBool(shaderProgram, "isEmissive", false);
        float cloudY = camPos.y + 50.0f; // clouds always above camera
        float cloudDrift = time * 0.8f; // slow drift

        // Generate cloud clusters using noise
        for (int cx = -3; cx <= 3; cx++) {
            for (int cz = -3; cz <= 3; cz++) {
                float worldX = camPos.x + cx * 20.0f + cloudDrift;
                float worldZ = camPos.z + cz * 20.0f;
                // Snap to grid for stable clouds
                float snapX = floorf(worldX / 20.0f) * 20.0f;
                float snapZ = floorf(worldZ / 20.0f) * 20.0f;

                float n = fbmNoise(snapX * 0.02f, snapZ * 0.02f, 2);
                if (n < 0.1f) continue; // skip if no cloud here

                // Cloud cluster: several hex blobs
                float cloudSize = 3.0f + n * 4.0f;
                glm::vec3 base(snapX - cloudDrift + cloudDrift, cloudY, snapZ);

                // Main cloud body
                drawHex(base, cloudColor, glm::vec3(cloudSize, 1.5f, cloudSize * 0.8f));
                // Puffs
                if (n > 0.3f) {
                    drawHex(base + glm::vec3(cloudSize * 0.5f, 0.5f, 0), cloudColor,
                            glm::vec3(cloudSize * 0.6f, 1.0f, cloudSize * 0.5f));
                    drawHex(base + glm::vec3(-cloudSize * 0.4f, 0.3f, cloudSize * 0.3f), cloudColor,
                            glm::vec3(cloudSize * 0.5f, 0.8f, cloudSize * 0.4f));
                }
            }
        }
        setFloat(shaderProgram, "alpha", 1.0f);
        glDisable(GL_BLEND);
    }

    setBool(shaderProgram, "isEmissive", false);
    glDepthMask(GL_TRUE);
}


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
const glm::vec3 COL_STONE_LIGHT(0.62f, 0.6f, 0.55f);
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
    } else if (h >= columnMaxH[gi][gj]) {
        // Broke the top block — scan down for new max
        int newMax = 0;
        for (int hh = h; hh >= 0; hh--) {
            if (blockGrid[gi][gj][hh] != BLOCK_AIR) { newMax = hh; break; }
        }
        columnMaxH[gi][gj] = newMax;
    }
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
        case BLOCK_COAL_ORE: return glm::vec3(0.22f, 0.22f, 0.22f);
        default: return glm::vec3(1, 0, 1); // magenta = error
    }
}

// =====================================================
// Biome system: 0=sand, 1=grass, 2=stone, 3=water
// =====================================================
int getBiome(int col, int row) {
    float bx = col * 0.06f + 100.0f;
    float by = row * 0.06f + 100.0f;
    float n = fbmNoise(bx, by, 3);

    if (n < -0.3f) return 3; // water
    if (n < 0.0f)  return 0; // sand
    if (n < 0.5f)  return 1; // grass
    return 2;                 // stone
}

int getTerrainHeightBiome(int col, int row, int biome) {
    float nx = col * 0.08f;
    float ny = row * 0.08f;
    float n = fbmNoise(nx, ny, 4);
    float detail = fbmNoise(col * 0.2f, row * 0.2f, 2) * 0.3f;

    switch (biome) {
        case 0: { // sand: flat with gentle dunes
            int h = (int)floorf((n + detail + 1.0f) * 0.6f);
            if (h < 0) h = 0;
            if (h > 1) h = 1;
            return h + UNDERGROUND_DEPTH;
        }
        case 1: { // grass: rolling hills with stepped plateaus
            int h = (int)floorf((n + detail + 1.0f) * 2.5f);
            if (h < 1) h = 1;
            if (h > 6) h = 6;
            // Stepped terrain: snap to even numbers for plateau look
            h = (h / 2) * 2;
            if (h < 1) h = 1;
            return h + UNDERGROUND_DEPTH;
        }
        case 2: { // stone: elevated plateaus with dramatic cliff edges
            int h = (int)floorf((n + detail + 1.0f) * 2.0f) + 3;
            if (h < 3) h = 3;
            if (h > 8) h = 8;
            // Stepped terrain: snap to nearest 3 for cliff faces
            h = (h / 3) * 3;
            if (h < 3) h = 3;
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

void initBlockGrid() {
    memset(blockGrid, 0, sizeof(blockGrid));
    treeLocations.clear();
    torchLocations.clear();

    for (int row = TERRAIN_MIN; row <= TERRAIN_MAX; row++) {
        for (int col = TERRAIN_MIN; col <= TERRAIN_MAX; col++) {
            unsigned int seed = gridSeed(col, row);

            // Water ponds (expanded with beaches)
            int pondType = isWaterPond(col, row);
            if (pondType == 1) {
                // Water: fill underground + water on top
                setBlock(col, row, 0, BLOCK_BEDROCK);
                for (int h = 1; h < UNDERGROUND_DEPTH; h++)
                    setBlock(col, row, h, BLOCK_STONE);
                setBlock(col, row, UNDERGROUND_DEPTH - 1, BLOCK_SAND); // sandy bottom
                setBlock(col, row, UNDERGROUND_DEPTH, BLOCK_WATER);
                setBlock(col, row, UNDERGROUND_DEPTH + 1, BLOCK_WATER); // 2 layers deep
                continue;
            }
            if (pondType == 2) {
                // Beach: sand border around ponds
                setBlock(col, row, 0, BLOCK_BEDROCK);
                for (int h = 1; h < UNDERGROUND_DEPTH; h++)
                    setBlock(col, row, h, BLOCK_STONE);
                setBlock(col, row, UNDERGROUND_DEPTH, BLOCK_SAND);
                setBlock(col, row, UNDERGROUND_DEPTH + 1, BLOCK_SAND);
                continue;
            }

            int biome = getBiome(col, row);
            int height = getTerrainHeightBiome(col, row, biome);

            // ======== UNDERGROUND LAYERS (h=0 to UNDERGROUND_DEPTH-1) ========
            // h=0: Bedrock (unbreakable)
            setBlock(col, row, 0, BLOCK_BEDROCK);

            // h=1 to UNDERGROUND_DEPTH-1: stone with ores and small caves
            for (int h = 1; h < UNDERGROUND_DEPTH; h++) {
                // Cave air pockets: use 3D noise to carve small chambers
                float caveNoise = fbmNoise(col * 0.15f + h * 0.5f, row * 0.15f + h * 0.7f, 2);
                if (h >= 3 && h <= 7 && caveNoise > 0.55f) {
                    setBlock(col, row, h, BLOCK_AIR); // cave pocket
                    continue;
                }

                int blockType;
                unsigned int oreSeed = gridSeed(col + h * 7, row + h * 13);

                if (h <= 2) {
                    // Deep: diamond ore veins (~1 in 15)
                    if (oreSeed % 15 == 0)
                        blockType = BLOCK_ORE_DIAMOND;
                    else
                        blockType = BLOCK_STONE;
                } else if (h <= 5) {
                    // Mid: gold ore veins (~1 in 18)
                    if (oreSeed % 18 == 0)
                        blockType = BLOCK_ORE_GOLD;
                    else if (oreSeed % 12 == 0)
                        blockType = BLOCK_COAL_ORE;
                    else
                        blockType = BLOCK_STONE;
                } else {
                    // Upper underground: coal ore (~1 in 10), some iron
                    if (oreSeed % 10 == 0)
                        blockType = BLOCK_COAL_ORE;
                    else if (oreSeed % 25 == 0)
                        blockType = BLOCK_ORE_GOLD;
                    else
                        blockType = BLOCK_STONE;
                }
                setBlock(col, row, h, blockType);
            }

            // ======== SURFACE LAYERS (UNDERGROUND_DEPTH to topH) ========
            int topH = height + 1;
            for (int h = UNDERGROUND_DEPTH; h <= topH; h++) {
                int blockType;
                if (h == topH) {
                    // Top surface block
                    switch (biome) {
                        case 0: blockType = BLOCK_SAND; break;
                        case 1: blockType = BLOCK_GRASS; break;
                        case 2: blockType = (seed % 2) ? BLOCK_STONE : BLOCK_STONE_LIGHT; break;
                        default: blockType = BLOCK_SAND; break;
                    }
                } else if (biome == 1 && h >= topH - 3 && h < topH) {
                    // 3 layers of dirt below grass
                    blockType = BLOCK_DIRT;
                } else if (biome == 0 && h >= topH - 2 && h < topH) {
                    // 2 layers of sand below surface
                    blockType = BLOCK_SAND;
                } else {
                    // Transition stone between underground and surface
                    unsigned int oreSeed = gridSeed(col + h * 7, row + h * 13);
                    if (oreSeed % 40 == 0)
                        blockType = BLOCK_ORE_DIAMOND;
                    else if (oreSeed % 30 == 0)
                        blockType = BLOCK_ORE_GOLD;
                    else if (oreSeed % 15 == 0)
                        blockType = BLOCK_COAL_ORE;
                    else
                        blockType = BLOCK_STONE;
                }
                setBlock(col, row, h, blockType);
            }

            // Record tree locations — denser forest (1 in 3 instead of 1 in 4)
            if (isForestZone(col, row) && biome == 1 && height >= UNDERGROUND_DEPTH + 1) {
                if (seed % 3 == 0) {
                    TreeInfo ti;
                    ti.col = col; ti.row = row;
                    ti.leafColor = TREE_COLORS[seed % NUM_TREE_COLORS];
                    ti.large = (seed % 9 == 0);
                    treeLocations.push_back(ti);
                }
            } else if (biome == 1 && height >= UNDERGROUND_DEPTH + 1 && (seed % 28 == 0)) {
                // Scattered grass-biome trees outside forest
                TreeInfo ti;
                ti.col = col; ti.row = row;
                ti.leafColor = TREE_COLORS[seed % NUM_TREE_COLORS];
                ti.large = (seed % 56 == 0);
                treeLocations.push_back(ti);
            }

            // Record torch locations
            if (biome == 1 && height >= UNDERGROUND_DEPTH + 1 && (seed % 61 == 0)) {
                TorchInfo t; t.col = col; t.row = row; t.height = height + 2;
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

    // =========================================================
    // CASTLE — truly massive castle with courtyard and rooms
    // Outer walls: col -5..40, row -20..25 (46 wide x 46 deep)
    // Walls 12 blocks tall, towers 20 blocks tall
    // Grand entrance, courtyard, great hall, bedroom, kitchen,
    // library, throne room, armory, dungeon stairway
    // =========================================================
    {
        // --- Castle outer bounds ---
        const int C0 = -20, C1 = 25;   // col range (46 wide) — centered near spawn
        const int R0 = 12, R1 = 57;    // row range (46 deep) — entrance faces spawn
        const int wallH = 12;
        const int twrH = 20;            // tower height
        const int floorLevel = 1;
        const int baseH = floorLevel + 2; // wall start (above floor)
        const int fH = floorLevel + 2;    // furniture on top of floor

        // --- Helper lambdas ---
        auto foundation = [&](int rA, int rB, int cA, int cB) {
            for (int r = rA; r <= rB; r++)
                for (int c = cA; c <= cB; c++) {
                    for (int h = 0; h <= floorLevel; h++)
                        setBlock(c, r, h, BLOCK_STONE);
                    setBlock(c, r, floorLevel + 1, BLOCK_WOOD);
                    for (int h = floorLevel + 2; h < GRID_H; h++)
                        setBlock(c, r, h, BLOCK_AIR);
                }
        };

        auto walls = [&](int rA, int rB, int cA, int cB, int ht) {
            for (int r = rA; r <= rB; r++)
                for (int c = cA; c <= cB; c++) {
                    if (r != rA && r != rB && c != cA && c != cB) continue;
                    for (int h = baseH; h < baseH + ht && h < GRID_H; h++) {
                        int bt = ((c + r + h) % 2 == 0) ? BLOCK_STONE : BLOCK_STONE_LIGHT;
                        setBlock(c, r, h, bt);
                    }
                }
        };

        auto innerWall = [&](int fixedRow, int cA, int cB, int ht) {
            for (int c = cA; c <= cB; c++)
                for (int h = baseH; h < baseH + ht && h < GRID_H; h++)
                    setBlock(c, fixedRow, h, BLOCK_STONE);
        };

        auto innerWallCol = [&](int fixedCol, int rA, int rB, int ht) {
            for (int r = rA; r <= rB; r++)
                for (int h = baseH; h < baseH + ht && h < GRID_H; h++)
                    setBlock(fixedCol, r, h, BLOCK_STONE);
        };

        auto doorHole = [&](int cA, int cB, int row, int ht) {
            for (int c = cA; c <= cB; c++)
                for (int h = baseH; h < baseH + ht; h++)
                    setBlock(c, row, h, BLOCK_AIR);
        };

        auto doorHoleCol = [&](int col, int rA, int rB, int ht) {
            for (int r = rA; r <= rB; r++)
                for (int h = baseH; h < baseH + ht; h++)
                    setBlock(col, r, h, BLOCK_AIR);
        };

        auto win = [&](int c, int r) {
            setBlock(c, r, baseH + 3, BLOCK_GLASS);
            setBlock(c, r, baseH + 4, BLOCK_GLASS);
            setBlock(c, r, baseH + 5, BLOCK_GLASS);
        };

        auto torch = [&](int c, int r, int h) {
            TorchInfo t; t.col = c; t.row = r; t.height = h;
            torchLocations.push_back(t);
        };

        // ============ FOUNDATION + FLOOR ============
        foundation(R0, R1, C0, C1);

        // ============ OUTER WALLS ============
        walls(R0, R1, C0, C1, wallH);

        // ============ BATTLEMENTS ============
        {
            int bH = baseH + wallH;
            for (int c = C0; c <= C1; c += 2) {
                if (bH < GRID_H) { setBlock(c, R0, bH, BLOCK_STONE); setBlock(c, R1, bH, BLOCK_STONE); }
            }
            for (int r = R0; r <= R1; r += 2) {
                if (bH < GRID_H) { setBlock(C0, r, bH, BLOCK_STONE); setBlock(C1, r, bH, BLOCK_STONE); }
            }
        }

        // ============ 4 CORNER TOWERS (5x5 each, 20 blocks tall) ============
        {
            int tw = 4; // tower is 5x5 (0..4)
            int tCorners[][2] = { {C0, R0}, {C1 - tw, R0}, {C0, R1 - tw}, {C1 - tw, R1 - tw} };
            for (int t = 0; t < 4; t++) {
                int tc = tCorners[t][0], tr = tCorners[t][1];
                for (int r = tr; r <= tr + tw; r++)
                    for (int c = tc; c <= tc + tw; c++) {
                        for (int h = baseH; h < baseH + twrH && h < GRID_H; h++) {
                            bool edge = (r == tr || r == tr + tw || c == tc || c == tc + tw);
                            if (edge)
                                setBlock(c, r, h, ((c + r + h) % 3 == 0) ? BLOCK_STONE_LIGHT : BLOCK_STONE);
                            else
                                setBlock(c, r, h, BLOCK_AIR);
                        }
                        // Tower cap
                        int capH = baseH + twrH;
                        if (capH < GRID_H) setBlock(c, r, capH, BLOCK_STONE);
                    }
                // Tower battlement
                for (int c = tc; c <= tc + tw; c += 2)
                    if (baseH + twrH + 1 < GRID_H) {
                        setBlock(c, tr, baseH + twrH + 1, BLOCK_STONE);
                        setBlock(c, tr + tw, baseH + twrH + 1, BLOCK_STONE);
                    }
                for (int r = tr; r <= tr + tw; r += 2)
                    if (baseH + twrH + 1 < GRID_H) {
                        setBlock(tc, r, baseH + twrH + 1, BLOCK_STONE);
                        setBlock(tc + tw, r, baseH + twrH + 1, BLOCK_STONE);
                    }
                torch(tc + 2, tr + 2, baseH + twrH + 1);
            }
        }

        // ============ GRAND ENTRANCE — south wall, 7 wide x 5 tall ============
        {
            int dm = (C0 + C1) / 2;
            doorHole(dm - 3, dm + 3, R0, 5);
            // Wood frame pillars
            for (int h = baseH; h < baseH + 5; h++) {
                setBlock(dm - 4, R0, h, BLOCK_WOOD);
                setBlock(dm + 4, R0, h, BLOCK_WOOD);
            }
            // Arch lintel
            for (int c = dm - 3; c <= dm + 3; c++)
                setBlock(c, R0, baseH + 5, BLOCK_WOOD);
            // Path outside — extends from castle entrance toward spawn
            for (int r = R0 - 8; r <= R0 - 1; r++)
                for (int c = dm - 2; c <= dm + 2; c++)
                    setBlock(c, r, floorLevel + 1, BLOCK_SAND);
        }

        // ============ WINDOWS on all outer walls ============
        {
            int dm = (C0 + C1) / 2;
            for (int c = C0 + 6; c <= C1 - 6; c += 4) {
                if (c >= dm - 4 && c <= dm + 4) continue; // skip door
                win(c, R0);
            }
            for (int c = C0 + 6; c <= C1 - 6; c += 4) win(c, R1);
            for (int r = R0 + 6; r <= R1 - 6; r += 4) { win(C0, r); win(C1, r); }
        }

        // ============ ROOF over entire castle ============
        {
            int roofLvl = baseH + wallH;
            for (int r = R0; r <= R1; r++)
                for (int c = C0; c <= C1; c++)
                    if (roofLvl < GRID_H)
                        setBlock(c, r, roofLvl, BLOCK_WOOD);
        }

        // ============ INTERIOR LAYOUT ============
        // Divide castle into rooms with internal walls:
        //
        //  Row R1 (north)
        //  +-----------+-----------+
        //  | LIBRARY   | BEDROOM   |
        //  | (NW)      | (NE)      |
        //  +-----+-----+-----------+
        //  |ARMRY| THRONE ROOM     |
        //  |(MW) | (center-east)   |
        //  +-----+---------+-------+
        //  |  GREAT HALL   |KITCHEN|
        //  |  (south)      | (SE)  |
        //  +-------[DOOR]--+-------+
        //  Row R0 (south)

        // Dividing rows
        int divRow1 = R0 + 15;   // separates great hall / kitchen from throne room / armory
        int divRow2 = R0 + 30;   // separates throne/armory from library/bedroom
        // Dividing columns
        int divCol1 = C0 + 12;   // left column split (armory width)
        int divCol2 = C1 - 12;   // right column split (kitchen width)
        int midCol = (C0 + C1) / 2;

        // --- Horizontal walls ---
        innerWall(divRow1, C0 + 1, C1 - 1, wallH);
        innerWall(divRow2, C0 + 1, C1 - 1, wallH);

        // --- Vertical walls ---
        // Kitchen wall (south section, east side)
        innerWallCol(divCol2, R0 + 1, divRow1 - 1, wallH);
        // Armory wall (middle section, west side)
        innerWallCol(divCol1, divRow1 + 1, divRow2 - 1, wallH);
        // Bedroom/library divider (north section, center)
        innerWallCol(midCol, divRow2 + 1, R1 - 1, wallH);

        // --- Doorways between rooms (5 wide, 4 tall) ---
        // Great hall -> throne room
        doorHole(midCol - 2, midCol + 2, divRow1, 4);
        // Throne room -> library
        doorHole(midCol - 6, midCol - 2, divRow2, 4);
        // Throne room -> bedroom
        doorHole(midCol + 2, midCol + 6, divRow2, 4);
        // Great hall -> kitchen
        doorHoleCol(divCol2, R0 + 6, R0 + 10, 4);
        // Throne room -> armory
        doorHoleCol(divCol1, divRow1 + 5, divRow1 + 9, 4);
        // Library <-> bedroom
        doorHole(midCol - 1, midCol + 1, divRow2 + (R1 - divRow2) / 2, 4);

        // ============ ROOM 1: GREAT HALL (south-west, R0+1 to divRow1-1) ============
        {
            int rA = R0 + 2, rB = divRow1 - 2;
            int cA = C0 + 2, cB = divCol2 - 2;
            int tMid = (cA + cB) / 2;

            // Two long banquet tables
            for (int tr = rA + 2; tr <= rB - 2; tr++) {
                // Left table
                for (int tc = tMid - 8; tc <= tMid - 4; tc++)
                    setBlock(tc, tr, fH + 1, BLOCK_WOOD);
                if ((tr - rA) % 4 == 0) { setBlock(tMid - 8, tr, fH, BLOCK_WOOD); setBlock(tMid - 4, tr, fH, BLOCK_WOOD); }
                // Right table
                for (int tc = tMid + 4; tc <= tMid + 8; tc++)
                    setBlock(tc, tr, fH + 1, BLOCK_WOOD);
                if ((tr - rA) % 4 == 0) { setBlock(tMid + 4, tr, fH, BLOCK_WOOD); setBlock(tMid + 8, tr, fH, BLOCK_WOOD); }
            }

            // Chairs
            for (int tr = rA + 2; tr <= rB - 2; tr += 2) {
                setBlock(tMid - 9, tr, fH, BLOCK_STONE_LIGHT);
                setBlock(tMid - 3, tr, fH, BLOCK_STONE_LIGHT);
                setBlock(tMid + 3, tr, fH, BLOCK_STONE_LIGHT);
                setBlock(tMid + 9, tr, fH, BLOCK_STONE_LIGHT);
            }

            // Grand fireplace (west wall, center)
            {
                int fpR = (rA + rB) / 2;
                for (int dr = -2; dr <= 2; dr++)
                    for (int h = fH; h < fH + 6 && h < GRID_H; h++)
                        setBlock(cA, fpR + dr, h, BLOCK_STONE);
                setBlock(cA, fpR - 1, fH + 1, BLOCK_ORE_GOLD);
                setBlock(cA, fpR, fH + 1, BLOCK_ORE_GOLD);
                setBlock(cA, fpR + 1, fH + 1, BLOCK_ORE_GOLD);
            }

            // Carpet runner from entrance
            for (int r = R0 + 1; r <= divRow1 - 1; r++)
                for (int c = midCol - 2; c <= midCol + 2; c++)
                    setBlock(c, r, floorLevel + 1, BLOCK_SAND);

            // Torches
            torch(cA + 2, rA + 1, fH + 4);
            torch(cB - 2, rA + 1, fH + 4);
            torch(cA + 2, rB - 1, fH + 4);
            torch(cB - 2, rB - 1, fH + 4);
            torch(tMid, (rA + rB) / 2, fH + 4);
        }

        // ============ ROOM 2: KITCHEN (south-east corner) ============
        {
            int rA = R0 + 2, rB = divRow1 - 2;
            int cA = divCol2 + 2, cB = C1 - 2;

            // Long countertop along east wall
            for (int r = rA + 1; r <= rB - 1; r++) {
                setBlock(cB, r, fH, BLOCK_STONE);
                setBlock(cB, r, fH + 1, BLOCK_STONE_LIGHT);
            }
            // Furnaces (emissive)
            setBlock(cB, rA + 3, fH + 1, BLOCK_ORE_GOLD);
            setBlock(cB, rA + 7, fH + 1, BLOCK_ORE_GOLD);

            // Center prep table
            int kMid = (cA + cB) / 2;
            int kMidR = (rA + rB) / 2;
            for (int c = kMid - 2; c <= kMid + 2; c++)
                setBlock(c, kMidR, fH + 1, BLOCK_WOOD);
            setBlock(kMid - 2, kMidR, fH, BLOCK_WOOD);
            setBlock(kMid + 2, kMidR, fH, BLOCK_WOOD);

            // Barrels (wood)
            for (int c = cA; c <= cA + 3; c++) {
                setBlock(c, rB, fH, BLOCK_WOOD);
                if (c % 2 == 0) setBlock(c, rB, fH + 1, BLOCK_WOOD);
            }

            torch(kMid, rA + 1, fH + 4);
            torch(kMid, rB - 1, fH + 4);
        }

        // ============ ROOM 3: THRONE ROOM (center) ============
        {
            int rA = divRow1 + 2, rB = divRow2 - 2;
            int cA = divCol1 + 2, cB = C1 - 2;
            int tMid = (cA + cB) / 2;
            int tMidR = (rA + rB) / 2;

            // Throne (elevated platform with gold accents)
            for (int c = tMid - 2; c <= tMid + 2; c++)
                for (int r = rB - 3; r <= rB - 1; r++) {
                    setBlock(c, r, fH, BLOCK_STONE);       // platform
                    setBlock(c, r, fH + 1, BLOCK_STONE);   // raised
                }
            // Throne chair
            setBlock(tMid, rB - 2, fH + 2, BLOCK_WOOD);
            setBlock(tMid, rB - 2, fH + 3, BLOCK_WOOD);   // back
            setBlock(tMid - 1, rB - 2, fH + 2, BLOCK_WOOD);
            setBlock(tMid + 1, rB - 2, fH + 2, BLOCK_WOOD);
            // Gold accents
            setBlock(tMid - 1, rB - 2, fH + 3, BLOCK_ORE_GOLD);
            setBlock(tMid + 1, rB - 2, fH + 3, BLOCK_ORE_GOLD);

            // Red carpet to throne
            for (int r = rA; r <= rB - 4; r++)
                for (int c = tMid - 1; c <= tMid + 1; c++)
                    setBlock(c, r, floorLevel + 1, BLOCK_SAND);

            // Pillars (stone columns, 2x2 at regular intervals)
            for (int r = rA + 3; r <= rB - 5; r += 6) {
                for (int h = fH; h < baseH + wallH - 1 && h < GRID_H; h++) {
                    setBlock(cA + 3, r, h, BLOCK_STONE);
                    setBlock(cB - 3, r, h, BLOCK_STONE);
                }
            }

            // Diamond ore display stands
            setBlock(tMid - 5, rB - 2, fH, BLOCK_STONE);
            setBlock(tMid - 5, rB - 2, fH + 1, BLOCK_ORE_DIAMOND);
            setBlock(tMid + 5, rB - 2, fH, BLOCK_STONE);
            setBlock(tMid + 5, rB - 2, fH + 1, BLOCK_ORE_DIAMOND);

            torch(cA + 2, rA + 1, fH + 4);
            torch(cB - 2, rA + 1, fH + 4);
            torch(cA + 2, rB - 1, fH + 4);
            torch(cB - 2, rB - 1, fH + 4);
            torch(tMid, tMidR, fH + 4);
        }

        // ============ ROOM 4: ARMORY (middle-west) ============
        {
            int rA = divRow1 + 2, rB = divRow2 - 2;
            int cA = C0 + 2, cB = divCol1 - 2;

            // Weapon racks along walls (wood + stone)
            for (int r = rA + 1; r <= rB - 1; r += 2) {
                setBlock(cA, r, fH, BLOCK_WOOD);
                setBlock(cA, r, fH + 1, BLOCK_STONE);
                setBlock(cA, r, fH + 2, BLOCK_STONE_LIGHT);
            }

            // Armor stands (center)
            int aMid = (cA + cB) / 2;
            for (int r = rA + 3; r <= rB - 3; r += 4) {
                setBlock(aMid, r, fH, BLOCK_STONE);
                setBlock(aMid, r, fH + 1, BLOCK_STONE_LIGHT);
                setBlock(aMid, r, fH + 2, BLOCK_STONE_LIGHT);
            }

            // Anvil (stone + ore)
            setBlock(cB - 1, (rA + rB) / 2, fH, BLOCK_STONE);
            setBlock(cB - 1, (rA + rB) / 2, fH + 1, BLOCK_ORE_GOLD);

            torch(aMid, rA + 1, fH + 4);
            torch(aMid, rB - 1, fH + 4);
        }

        // ============ ROOM 5: LIBRARY (north-west) ============
        {
            int rA = divRow2 + 2, rB = R1 - 2;
            int cA = C0 + 2, cB = midCol - 2;

            // Floor-to-ceiling bookshelves along north and west walls
            for (int c = cA; c <= cB; c++) {
                for (int h = fH; h <= fH + 4 && h < GRID_H; h++)
                    setBlock(c, rB, h, BLOCK_WOOD);
            }
            for (int r = rA + 1; r <= rB - 1; r++) {
                for (int h = fH; h <= fH + 4 && h < GRID_H; h++)
                    setBlock(cA, r, h, BLOCK_WOOD);
            }

            // Reading tables (2 tables)
            int lMid = (cA + cB) / 2;
            int lMidR = (rA + rB) / 2;
            for (int c = lMid - 3; c <= lMid + 3; c++) {
                setBlock(c, lMidR - 2, fH + 1, BLOCK_WOOD);
                setBlock(c, lMidR + 2, fH + 1, BLOCK_WOOD);
            }
            // Table legs
            setBlock(lMid - 3, lMidR - 2, fH, BLOCK_WOOD); setBlock(lMid + 3, lMidR - 2, fH, BLOCK_WOOD);
            setBlock(lMid - 3, lMidR + 2, fH, BLOCK_WOOD); setBlock(lMid + 3, lMidR + 2, fH, BLOCK_WOOD);

            // Chairs
            for (int c = lMid - 2; c <= lMid + 2; c += 2) {
                setBlock(c, lMidR - 3, fH, BLOCK_STONE_LIGHT);
                setBlock(c, lMidR + 3, fH, BLOCK_STONE_LIGHT);
            }

            // Globe (diamond ore on pedestal)
            setBlock(cB - 1, rA + 2, fH, BLOCK_STONE);
            setBlock(cB - 1, rA + 2, fH + 1, BLOCK_ORE_DIAMOND);

            torch(lMid, rA + 1, fH + 4);
            torch(lMid, rB - 1, fH + 4);
            torch(cA + 2, lMidR, fH + 4);
        }

        // ============ ROOM 6: BEDROOM (north-east) ============
        {
            int rA = divRow2 + 2, rB = R1 - 2;
            int cA = midCol + 2, cB = C1 - 2;
            int bMid = (cA + cB) / 2;
            int bMidR = (rA + rB) / 2;

            // Large king bed (5x3, against north wall)
            for (int c = bMid - 2; c <= bMid + 2; c++)
                for (int r = rB - 3; r <= rB - 1; r++)
                    setBlock(c, r, fH, BLOCK_LEAF);
            // Headboard
            for (int c = bMid - 2; c <= bMid + 2; c++) {
                setBlock(c, rB - 1, fH + 1, BLOCK_WOOD);
                setBlock(c, rB - 1, fH + 2, BLOCK_WOOD);
            }

            // Nightstands with lamps
            setBlock(bMid - 3, rB - 2, fH, BLOCK_WOOD);
            setBlock(bMid - 3, rB - 2, fH + 1, BLOCK_ORE_GOLD);
            setBlock(bMid + 3, rB - 2, fH, BLOCK_WOOD);
            setBlock(bMid + 3, rB - 2, fH + 1, BLOCK_ORE_GOLD);

            // Wardrobe (east wall)
            for (int r = bMidR - 2; r <= bMidR + 2; r++) {
                setBlock(cB, r, fH, BLOCK_WOOD);
                setBlock(cB, r, fH + 1, BLOCK_WOOD);
                setBlock(cB, r, fH + 2, BLOCK_WOOD);
            }

            // Desk and chair
            for (int c = cA + 1; c <= cA + 4; c++)
                setBlock(c, rA + 2, fH + 1, BLOCK_WOOD);
            setBlock(cA + 1, rA + 2, fH, BLOCK_WOOD);
            setBlock(cA + 4, rA + 2, fH, BLOCK_WOOD);
            setBlock(cA + 2, rA + 3, fH, BLOCK_STONE_LIGHT);

            // Rug in center (sand floor)
            for (int r = bMidR - 2; r <= bMidR + 2; r++)
                for (int c = bMid - 3; c <= bMid + 3; c++)
                    setBlock(c, r, floorLevel + 1, BLOCK_SAND);

            torch(bMid, rA + 1, fH + 4);
            torch(bMid, bMidR, fH + 4);
            torch(cA + 2, rB - 1, fH + 4);
        }

        // Remove trees and torches that ended up inside the castle
        for (int i = (int)treeLocations.size() - 1; i >= 0; i--) {
            if (treeLocations[i].col >= C0 - 2 && treeLocations[i].col <= C1 + 2 &&
                treeLocations[i].row >= R0 - 2 && treeLocations[i].row <= R1 + 2) {
                treeLocations.erase(treeLocations.begin() + i);
            }
        }
        for (int i = (int)torchLocations.size() - 1; i >= 0; i--) {
            if (torchLocations[i].col >= C0 - 2 && torchLocations[i].col <= C1 + 2 &&
                torchLocations[i].row >= R0 - 2 && torchLocations[i].row <= R1 + 2) {
                torchLocations.erase(torchLocations.begin() + i);
            }
        }

        printf("[Castle] Built at col %d..%d, row %d..%d (%dx%d), walls=%d, towers=%d\n",
               C0, C1, R0, R1, C1 - C0 + 1, R1 - R0 + 1, wallH, twrH);
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

    for (int row = TERRAIN_MIN; row <= TERRAIN_MAX; row++) {
        for (int col = TERRAIN_MIN; col <= TERRAIN_MAX; col++) {
            glm::vec3 pos = hexGridPos(col, row, 0.0f);

            // Distance culling: skip entire column if too far from camera (XZ only)
            float dx = pos.x - camPos.x;
            float dz = pos.z - camPos.z;
            float distSq = dx * dx + dz * dz;
            if (distSq > RENDER_DIST_SQ) continue;

            for (int h = 0; h < GRID_H; h++) {
                int bt = getBlock(col, row, h);
                if (bt == BLOCK_AIR) continue;

                // Skip fully occluded blocks (all neighbors solid)
                if (isBlockOccluded(col, row, h)) continue;

                glm::vec3 p = pos + glm::vec3(0, h * HEX_HEIGHT, 0);

                // Frustum culling
                if (!isInFrustum(p)) continue;

                if (bt == BLOCK_WATER) {
                    drawHexWater(p, getBlockColor(bt), time);
                } else if (bt == BLOCK_ORE_DIAMOND || bt == BLOCK_ORE_GOLD) {
                    drawHexEmissive(p, getBlockColor(bt));
                } else {
                    drawHex(p, getBlockColor(bt));
                }
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


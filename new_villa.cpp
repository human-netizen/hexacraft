// =========================================================
// EPIC MODERN VILLA (Main Spawn Base)
// =========================================================
void buildModernVilla(int c0, int r0) {
    int W = 36, D = 32;
    int c1 = c0 + W - 1;
    int r1 = r0 + D - 1;
    int ground = UNDERGROUND_DEPTH;

    // 1. Foundation Frame & Plot Clear
    for (int c = c0; c <= c1; c++) {
        for (int r = r0; r <= r1; r++) {
            for (int h = ground + 1; h < GRID_H; h++) setBlock(c, r, h, BLOCK_AIR); // Clear air
            setBlock(c, r, 0, BLOCK_BEDROCK);
            for (int h = 1; h < ground; h++) setBlock(c, r, h, BLOCK_STONE); // Solid stone base

            if (c == c0 || c == c1 || r == r0 || r == r1) {
                setBlock(c, r, ground, BLOCK_STONE); // border
            } else {
                setBlock(c, r, ground, BLOCK_GRASS); // inside plot
            }
        }
    }
    
    // Clear trees/torches in area
    for (int i = (int)treeLocations.size() - 1; i >= 0; i--) {
        if (treeLocations[i].col >= c0 && treeLocations[i].col <= c1 &&
            treeLocations[i].row >= r0 && treeLocations[i].row <= r1) {
            treeLocations.erase(treeLocations.begin() + i);
        }
    }
    for (int i = (int)torchLocations.size() - 1; i >= 0; i--) {
        if (torchLocations[i].col >= c0 && torchLocations[i].col <= c1 &&
            torchLocations[i].row >= r0 && torchLocations[i].row <= r1) {
            torchLocations.erase(torchLocations.begin() + i);
        }
    }

    auto fillRect = [&](int cx0, int cx1, int rx0, int rx1, int h0, int h1, int type) {
        for (int c = cx0; c <= cx1; c++)
            for (int r = rx0; r <= rx1; r++)
                for (int h = h0; h <= h1; h++)
                    setBlock(c, r, h, type);
    };

    // The house footprint inside the plot
    int cx = c0 + 16; // Center column, width 4 (col 16, 17, 18, 19)
    int TowerR = r0 + 5; // back 
    int TowerR1 = r0 + 15; // depth 10
    
    // Central Tower (Anchor with central glass strip)
    fillRect(cx, cx+3, TowerR, TowerR1, ground+1, ground+18, BLOCK_COAL_ORE);
    fillRect(cx+1, cx+2, TowerR+1, TowerR1-1, ground+1, ground+17, BLOCK_AIR); // hollow interior
    fillRect(cx+1, cx+2, TowerR1, TowerR1, ground+5, ground+17, BLOCK_GLASS); // massive vertical glass strip!
    fillRect(cx+1, cx+2, TowerR1, TowerR1, ground+1, ground+3, BLOCK_AIR); // wide doorway
    fillRect(cx+1, cx+2, TowerR1, TowerR1, ground+4, ground+4, BLOCK_COAL_ORE); // brace over door

    // Left Wing (Ground: Living Room)
    int LW_c0 = c0 + 4;
    int LW_c1 = cx - 1; // cols 4 to 15
    int LW_r0 = r0 + 5; 
    int LW_r1 = r0 + 17; // depth 13
    
    // Left Wing Floors/Ceilings
    fillRect(LW_c0, LW_c1, LW_r0, LW_r1, ground, ground, BLOCK_WOOD);
    fillRect(LW_c0, LW_c1, LW_r0, LW_r1, ground+6, ground+6, BLOCK_STONE_LIGHT);
    fillRect(LW_c0, LW_c1, LW_r0, LW_r1, ground+12, ground+12, BLOCK_STONE_LIGHT);
    
    // Left Wing Walls (White Concrete / Stone Light)
    fillRect(LW_c0, LW_c1, LW_r0, LW_r0, ground+1, ground+11, BLOCK_STONE_LIGHT); // back
    fillRect(LW_c0, LW_c0, LW_r0, LW_r1, ground+1, ground+11, BLOCK_STONE_LIGHT); // side
    
    // Front Glass Facade
    fillRect(LW_c0+1, LW_c1, LW_r1, LW_r1, ground+1, ground+5, BLOCK_GLASS);
    fillRect(LW_c0+1, LW_c1, LW_r1, LW_r1, ground+7, ground+11, BLOCK_GLASS);
    
    // Left Wing Cantilever & Open Terrace Pergola
    fillRect(LW_c0, LW_c1, LW_r1+1, LW_r1+3, ground+6, ground+6, BLOCK_STONE_LIGHT); // huge floor overhang

    // 2nd Floor Pergola on the Left Wing terrace
    // Instead of a solid roof, we make a slotted wooden pergola over the balcony
    // Back support
    fillRect(LW_c0, LW_c1, LW_r1+1, LW_r1+1, ground+11, ground+11, BLOCK_WOOD);
    // Front support
    fillRect(LW_c0, LW_c1, LW_r1+3, LW_r1+3, ground+11, ground+11, BLOCK_WOOD);
    // Pillars for pergola
    fillRect(LW_c0, LW_c0, LW_r1+3, LW_r1+3, ground+7, ground+10, BLOCK_WOOD); // Left pillar
    fillRect(LW_c1, LW_c1, LW_r1+3, LW_r1+3, ground+7, ground+10, BLOCK_WOOD); // Right pillar
    // Slotted roof
    for (int c = LW_c0; c <= LW_c1; c += 2) {
        fillRect(c, c, LW_r1+1, LW_r1+3, ground+12, ground+12, BLOCK_WOOD);
    }
    
    // Lower wood supports for the cantilevered floor
    fillRect(LW_c0, LW_c0+1, LW_r1+3, LW_r1+3, ground+1, ground+5, BLOCK_WOOD);
    fillRect(LW_c1-1, LW_c1, LW_r1+3, LW_r1+3, ground+1, ground+5, BLOCK_WOOD);

    // Right Wing
    int RW_c0 = cx + 4; // cols 20 to 31
    int RW_c1 = c1 - 4; 
    int RW_r0 = r0 + 5;
    int RW_r1 = r0 + 17;
    
    // Right Wing Floors/Ceilings
    fillRect(RW_c0, RW_c1, RW_r0, RW_r1, ground, ground, BLOCK_WOOD);
    fillRect(RW_c0, RW_c1, RW_r0, RW_r1, ground+6, ground+6, BLOCK_STONE_LIGHT);
    fillRect(RW_c0, RW_c1, RW_r0, RW_r1, ground+12, ground+12, BLOCK_STONE_LIGHT); // solid roof here
    
    // Right Wing Walls
    fillRect(RW_c0, RW_c1, RW_r0, RW_r0, ground+1, ground+11, BLOCK_STONE_LIGHT); // back
    fillRect(RW_c1, RW_c1, RW_r0, RW_r1, ground+1, ground+11, BLOCK_STONE_LIGHT); // side
    
    // Front Glass Facade
    fillRect(RW_c0, RW_c1-1, RW_r1, RW_r1, ground+1, ground+5, BLOCK_GLASS);
    fillRect(RW_c0, RW_c1-1, RW_r1, RW_r1, ground+7, ground+11, BLOCK_GLASS);

    // Right Wing Subdued Overhang
    fillRect(RW_c0, RW_c1, RW_r1+1, RW_r1+2, ground+6, ground+6, BLOCK_STONE_LIGHT);
    fillRect(RW_c0, RW_c1, RW_r1+1, RW_r1+2, ground+12, ground+12, BLOCK_STONE_LIGHT);
    fillRect(RW_c0, RW_c1-1, RW_r1+2, RW_r1+2, ground+7, ground+11, BLOCK_GLASS);
    fillRect(RW_c0, RW_c0+1, RW_r1+2, RW_r1+2, ground+1, ground+5, BLOCK_WOOD);
    fillRect(RW_c1-1, RW_c1, RW_r1+2, RW_r1+2, ground+1, ground+5, BLOCK_WOOD);

    // ==========================================
    // 3. The Raised Glass Infinity Pool
    // ==========================================
    int Pool_c0 = cx + 5; 
    int Pool_c1 = RW_c1 + 3; // Wider pool extension
    int Pool_r0 = RW_r1 + 5; // Further out front
    int Pool_r1 = r1 - 3;
    int Pool_h = ground + 3; // Raised 3 blocks!

    // Pool Stone Base underneath
    fillRect(Pool_c0, Pool_c1, Pool_r0, Pool_r1, ground+1, ground+1, BLOCK_STONE_LIGHT);
    fillRect(Pool_c0, Pool_c1, Pool_r0, Pool_r0, ground+2, Pool_h, BLOCK_STONE_LIGHT); // Back wall solid
    
    // Glass Front & Side walls holding the water
    fillRect(Pool_c0, Pool_c1, Pool_r1, Pool_r1, ground+2, Pool_h, BLOCK_GLASS); // Front
    fillRect(Pool_c0, Pool_c0, Pool_r0+1, Pool_r1-1, ground+2, Pool_h, BLOCK_GLASS); // Left inner side
    fillRect(Pool_c1, Pool_c1, Pool_r0+1, Pool_r1-1, ground+2, Pool_h, BLOCK_GLASS); // Right side
    
    // Fill with water
    fillRect(Pool_c0+1, Pool_c1-1, Pool_r0+1, Pool_r1-1, ground+2, Pool_h-1, BLOCK_WATER);

    // Wooden stairs leading up to raised pool deck
    fillRect(Pool_c0-2, Pool_c0-1, Pool_r1-2, Pool_r1-2, ground+1, ground+1, BLOCK_WOOD); // Step 1
    fillRect(Pool_c0-2, Pool_c0-1, Pool_r1-3, Pool_r1-3, ground+1, ground+2, BLOCK_WOOD); // Step 2
    fillRect(Pool_c0-2, Pool_c0-1, Pool_r1-4, Pool_r1-4, ground+1, ground+3, BLOCK_WOOD); // Step 3
    // Deck landing next to pool
    fillRect(Pool_c0-2, Pool_c0-1, Pool_r0, Pool_r1-5, ground+1, ground+3, BLOCK_STONE_LIGHT);

    // Glass safety railing around the balcony terraces (only 1 block high)
    fillRect(LW_c0, LW_c1, LW_r1+4, LW_r1+4, ground+7, ground+7, BLOCK_GLASS); // Left balcony
    fillRect(LW_c0-1, LW_c0-1, LW_r1+1, LW_r1+4, ground+7, ground+7, BLOCK_GLASS);
    fillRect(RW_c0, RW_c1, RW_r1+3, RW_r1+3, ground+7, ground+7, BLOCK_GLASS); // Right balcony
    fillRect(RW_c1+1, RW_c1+1, RW_r1+1, RW_r1+3, ground+7, ground+7, BLOCK_GLASS);

    // Front hedges around the entire house boundary
    fillRect(c0+1, c1-1, r1-1, r1-1, ground+1, ground+1, BLOCK_LEAF);
    
    // Rooftop Gardens (Right wing and back half of left wing)
    fillRect(LW_c0+1, LW_c1-1, LW_r0+1, LW_r1, ground+13, ground+13, BLOCK_GRASS);
    fillRect(RW_c0+1, RW_c1-1, RW_r0+1, RW_r1+1, ground+13, ground+13, BLOCK_GRASS);
    for (int c = LW_c0+1; c <= LW_c1-1; c+=2) setBlock(c, LW_r1, ground+14, BLOCK_LEAF);
    for (int c = RW_c0+1; c <= RW_c1-1; c+=2) setBlock(c, RW_r1+1, ground+14, BLOCK_LEAF);

    printf("[Villa] Built exact Modern Villa replica at col %d..%d, row %d..%d\n", c0, c1, r0, r1);
}

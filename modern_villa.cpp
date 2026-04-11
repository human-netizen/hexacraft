// =========================================================
// Modern Minimalist Villa Generation
// =========================================================
void buildModernVilla(int c0, int r0) {
    int W = 30, D = 28;
    int c1 = c0 + W - 1;
    int r1 = r0 + D - 1;
    int ground = UNDERGROUND_DEPTH;

    // 1. Foundation Frame & Plot Clear
    for (int c = c0; c <= c1; c++) {
        for (int r = r0; r <= r1; r++) {
            // clear air above
            for (int h = ground + 1; h < GRID_H; h++) setBlock(c, r, h, BLOCK_AIR);
            // stone frame underneath
            setBlock(c, r, 0, BLOCK_BEDROCK);
            for (int h = 1; h < ground; h++) setBlock(c, r, h, BLOCK_STONE);

            if (c == c0 || c == c1 || r == r0 || r == r1) {
                setBlock(c, r, ground, BLOCK_STONE); // border
            } else {
                setBlock(c, r, ground, BLOCK_GRASS); // inside plot
            }
        }
    }
    
    // Clear trees/torches
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

    // Helper lambda to draw filled rects easily
    auto fillRect = [&](int cx0, int cx1, int rx0, int rx1, int h0, int h1, int type) {
        for (int c = cx0; c <= cx1; c++)
            for (int r = rx0; r <= rx1; r++)
                for (int h = h0; h <= h1; h++)
                    setBlock(c, r, h, type);
    };

    // The house footprint inside the plot
    int cx = c0 + 13; // Center column, width 4 (col 13, 14, 15, 16)
    int TowerR = r0 + 5; // back of the plot
    int TowerR1 = r0 + 15; // depth 10
    
    // Central Tower (Anchor)
    fillRect(cx, cx+3, TowerR, TowerR1, ground+1, ground+18, BLOCK_COAL_ORE);
    fillRect(cx+1, cx+2, TowerR+1, TowerR1-1, ground+1, ground+17, BLOCK_AIR); // hollow interior
    fillRect(cx+1, cx+2, TowerR1, TowerR1, ground+1, ground+17, BLOCK_GLASS); // glass front
    fillRect(cx+1, cx+2, TowerR1, TowerR1, ground+1, ground+2, BLOCK_AIR); // doorway

    // Left Wing (Ground: Living Room)
    int LW_c0 = c0 + 3;
    int LW_c1 = cx - 1; // cols 3 to 12
    int LW_r0 = r0 + 5; 
    int LW_r1 = r0 + 17; // depth 13
    
    // Left Wing Floors/Ceilings
    fillRect(LW_c0, LW_c1, LW_r0, LW_r1, ground, ground, BLOCK_WOOD);
    fillRect(LW_c0, LW_c1, LW_r0, LW_r1, ground+6, ground+6, BLOCK_STONE_LIGHT);
    fillRect(LW_c0, LW_c1, LW_r0, LW_r1, ground+12, ground+12, BLOCK_STONE_LIGHT);
    
    // Left Wing Walls (White Concrete)
    fillRect(LW_c0, LW_c1, LW_r0, LW_r0, ground+1, ground+11, BLOCK_STONE_LIGHT); // back
    fillRect(LW_c0, LW_c0, LW_r0, LW_r1, ground+1, ground+11, BLOCK_STONE_LIGHT); // side
    
    // Front Glass Facade
    fillRect(LW_c0+1, LW_c1, LW_r1, LW_r1, ground+1, ground+5, BLOCK_GLASS);
    fillRect(LW_c0+1, LW_c1, LW_r1, LW_r1, ground+7, ground+11, BLOCK_GLASS);
    
    // Left Wing Cantilever details (Overhang)
    fillRect(LW_c0, LW_c1, LW_r1+1, LW_r1+2, ground+6, ground+6, BLOCK_STONE_LIGHT); // floor overhang
    fillRect(LW_c0, LW_c1, LW_r1+1, LW_r1+2, ground+12, ground+12, BLOCK_STONE_LIGHT); // roof overhang
    fillRect(LW_c0+1, LW_c1, LW_r1+2, LW_r1+2, ground+7, ground+11, BLOCK_GLASS); // upper front glass
    fillRect(LW_c0, LW_c0+1, LW_r1+2, LW_r1+2, ground+1, ground+5, BLOCK_WOOD); // wood support
    fillRect(LW_c1-1, LW_c1, LW_r1+2, LW_r1+2, ground+1, ground+5, BLOCK_WOOD);

    // Right Wing
    int RW_c0 = cx + 4; // cols 17 to 26
    int RW_c1 = c1 - 3; 
    int RW_r0 = r0 + 5;
    int RW_r1 = r0 + 17;
    
    // Right Wing Floors/Ceilings
    fillRect(RW_c0, RW_c1, RW_r0, RW_r1, ground, ground, BLOCK_WOOD);
    fillRect(RW_c0, RW_c1, RW_r0, RW_r1, ground+6, ground+6, BLOCK_STONE_LIGHT);
    fillRect(RW_c0, RW_c1, RW_r0, RW_r1, ground+12, ground+12, BLOCK_STONE_LIGHT);
    
    // Right Wing Walls
    fillRect(RW_c0, RW_c1, RW_r0, RW_r0, ground+1, ground+11, BLOCK_STONE_LIGHT); // back
    fillRect(RW_c1, RW_c1, RW_r0, RW_r1, ground+1, ground+11, BLOCK_STONE_LIGHT); // side
    
    // Front Glass Facade
    fillRect(RW_c0, RW_c1-1, RW_r1, RW_r1, ground+1, ground+5, BLOCK_GLASS);
    fillRect(RW_c0, RW_c1-1, RW_r1, RW_r1, ground+7, ground+11, BLOCK_GLASS);

    // Right Wing Overhang
    fillRect(RW_c0, RW_c1, RW_r1+1, RW_r1+2, ground+6, ground+6, BLOCK_STONE_LIGHT);
    fillRect(RW_c0, RW_c1, RW_r1+1, RW_r1+2, ground+12, ground+12, BLOCK_STONE_LIGHT);
    fillRect(RW_c0, RW_c1-1, RW_r1+2, RW_r1+2, ground+7, ground+11, BLOCK_GLASS);
    fillRect(RW_c0, RW_c0+1, RW_r1+2, RW_r1+2, ground+1, ground+5, BLOCK_WOOD);
    fillRect(RW_c1-1, RW_c1, RW_r1+2, RW_r1+2, ground+1, ground+5, BLOCK_WOOD);

    // Infinity Pool Setup (Front Right)
    int Pool_c0 = cx + 2; 
    int Pool_c1 = RW_c1; 
    int Pool_r0 = RW_r1 + 3; // in front of the house
    int Pool_r1 = r1 - 2;
    // Deck
    fillRect(Pool_c0-1, Pool_c1+1, Pool_r0-1, Pool_r1+1, ground, ground, BLOCK_STONE_LIGHT);
    // Pool water Basin
    fillRect(Pool_c0, Pool_c1, Pool_r0, Pool_r1, ground-1, ground-1, BLOCK_STONE_LIGHT);
    fillRect(Pool_c0, Pool_c1, Pool_r0, Pool_r1, ground, ground, BLOCK_WATER);
    // Glass safety railing around pool edge
    fillRect(Pool_c0-1, Pool_c1+1, Pool_r1+1, Pool_r1+1, ground+1, ground+1, BLOCK_GLASS);
    fillRect(Pool_c1+1, Pool_c1+1, Pool_r0-1, Pool_r1+1, ground+1, ground+1, BLOCK_GLASS);

    // Connecting Foyer interior floor
    fillRect(cx, cx+3, TowerR1-4, TowerR1, ground, ground, BLOCK_WOOD);
    
    // Rooftop Gardens
    fillRect(LW_c0+1, LW_c1-1, LW_r0+1, LW_r1+1, ground+13, ground+13, BLOCK_GRASS);
    fillRect(RW_c0+1, RW_c1-1, RW_r0+1, RW_r1+1, ground+13, ground+13, BLOCK_GRASS);
    for (int c = LW_c0+1; c <= LW_c1-1; c+=2) setBlock(c, LW_r1+1, ground+14, BLOCK_LEAF);
    for (int c = RW_c0+1; c <= RW_c1-1; c+=2) setBlock(c, RW_r1+1, ground+14, BLOCK_LEAF);

    // Balcony blue glass railings
    fillRect(LW_c0, LW_c1, LW_r1+3, LW_r1+3, ground+7, ground+7, BLOCK_GLASS);
    fillRect(RW_c0, RW_c1, RW_r1+3, RW_r1+3, ground+7, ground+7, BLOCK_GLASS);

    // Front hedges
    fillRect(c0+1, Pool_c0-3, r1-1, r1-1, ground+1, ground+1, BLOCK_LEAF);
    
    // Simple block furniture (Living Room)
    int sofaC = LW_c0 + 4, sofaR = LW_r1 - 4;
    setBlock(sofaC, sofaR, ground+1, BLOCK_STONE_LIGHT);
    setBlock(sofaC+1, sofaR, ground+1, BLOCK_STONE_LIGHT);
    setBlock(sofaC+2, sofaR, ground+1, BLOCK_STONE_LIGHT);
    setBlock(sofaC, sofaR-1, ground+1, BLOCK_STONE_LIGHT);

    // Simple table (Pool lounge)
    setBlock(RW_c0 + 4, RW_r1 - 3, ground+1, BLOCK_WOOD);
    setBlock(RW_c0 + 5, RW_r1 - 3, ground+1, BLOCK_GLASS);
    setBlock(RW_c0 + 6, RW_r1 - 3, ground+1, BLOCK_WOOD);
    
    printf("[Villa] Built modern villa at col %d..%d, row %d..%d\n", c0, c1, r0, r1);
}

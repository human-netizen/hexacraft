// =========================================================
// Epic Castle Generation (Replaces old 'lame' castle)
// =========================================================
void buildEpicCastle(int c0, int r0) {
    int W = 50, D = 50;
    int c1 = c0 + W - 1;
    int r1 = r0 + D - 1;
    int ground = UNDERGROUND_DEPTH;

    auto fillRect = [&](int cx0, int cx1, int rx0, int rx1, int h0, int h1, int type) {
        for (int c = cx0; c <= cx1; c++) {
            for (int r = rx0; r <= rx1; r++) {
                if(c >= c0 && c <= c1 && r >= r0 && r <= r1) {
                    for (int h = h0; h <= h1; h++) {
                        setBlock(c, r, h, type);
                    }
                }
            }
        }
    };
    
    // Clear the whole 50x50 area above ground, build solid bedrock/stone foundation
    for (int c = c0-2; c <= c1+2; c++) {
        for (int r = r0-2; r <= r1+2; r++) {
            for (int h = ground + 1; h < GRID_H; h++) setBlock(c, r, h, BLOCK_AIR);
            setBlock(c, r, 0, BLOCK_BEDROCK);
            for (int h = 1; h < ground; h++) setBlock(c, r, h, BLOCK_STONE);
            setBlock(c, r, ground, BLOCK_GRASS); // Base is grass
        }
    }
    
    // The Moat (Outer ring)
    // Width 3 moat around the border
    fillRect(c0, c1, r0, r0+3, ground-2, ground, BLOCK_WATER); // Back moat
    fillRect(c0, c1, r1-3, r1, ground-2, ground, BLOCK_WATER); // Front moat
    fillRect(c0, c0+3, r0, r1, ground-2, ground, BLOCK_WATER); // Left moat
    fillRect(c1-3, c1, r0, r1, ground-2, ground, BLOCK_WATER); // Right moat
    
    // The Main Wall framing the castle (inside the moat)
    int wC0 = c0 + 4, wC1 = c1 - 4;
    int wR0 = r0 + 4, wR1 = r1 - 4;
    int wallH = 12; // 12 blocks high above ground
    
    // Massive Outer Walls
    fillRect(wC0, wC1, wR0, wR0+1, ground+1, ground+wallH, BLOCK_STONE); // Back wall
    fillRect(wC0, wC1, wR1-1, wR1, ground+1, ground+wallH, BLOCK_STONE); // Front wall
    fillRect(wC0, wC0+1, wR0, wR1, ground+1, ground+wallH, BLOCK_STONE); // Left wall
    fillRect(wC1-1, wC1, wR0, wR1, ground+1, ground+wallH, BLOCK_STONE); // Right wall
    
    // Crenellations (teeth) on outer walls
    for(int c = wC0; c <= wC1; c+=2) {
        setBlock(c, wR0, ground+wallH+1, BLOCK_STONE);
        setBlock(c, wR1, ground+wallH+1, BLOCK_STONE);
    }
    for(int r = wR0; r <= wR1; r+=2) {
        setBlock(wC0, r, ground+wallH+1, BLOCK_STONE);
        setBlock(wC1, r, ground+wallH+1, BLOCK_STONE);
    }

    // 4 Corner Towers
    int twR = 3; // radius
    int twH = 20; // taller than walls
    auto buildTower = [&](int tc, int tr) {
        for(int c = tc-twR; c <= tc+twR; c++) {
            for(int r = tr-twR; r <= tr+twR; r++) {
                // Approximate circle
                if ((c-tc)*(c-tc) + (r-tr)*(r-tr) <= twR*twR + 2) {
                    for(int h=ground+1; h<=ground+twH; h++) {
                        setBlock(c, r, h, BLOCK_STONE);
                    }
                    // Battlements on tower
                    if ((c+r)%2==0) setBlock(c, r, ground+twH+1, BLOCK_STONE);
                }
            }
        }
        // Hollow interior
        for(int c = tc-1; c <= tc+1; c++) {
            for(int r = tr-1; r <= tr+1; r++) {
                for(int h=ground+1; h<ground+twH; h++) {
                    setBlock(c, r, h, BLOCK_AIR);
                }
            }
        }
    };
    buildTower(wC0+1, wR0+1);
    buildTower(wC1-1, wR0+1);
    buildTower(wC0+1, wR1-1);
    buildTower(wC1-1, wR1-1);

    // Front Gatehouse & bridge over moat
    int midC = (wC0 + wC1) / 2;
    // Bridge
    fillRect(midC-2, midC+2, r1-3, wR1, ground, ground, BLOCK_WOOD);
    // Gatehole
    fillRect(midC-1, midC+1, wR1-1, wR1, ground+1, ground+6, BLOCK_AIR);
    // Iron Portcullis (glass bars as proxy)
    fillRect(midC-1, midC+1, wR1, wR1, ground+4, ground+6, BLOCK_GLASS);
    
    // Courtyard Pathing (Cobble/Stone proxy)
    fillRect(midC-2, midC+2, wR0+5, wR1-2, ground, ground, BLOCK_STONE_LIGHT);

    // The Great Keep (Center/Back of courtyard)
    int kC0 = wC0 + 8, kC1 = wC1 - 8;
    int kR0 = wR0 + 4, kR1 = wR1 - 12; // deep keep
    int kH = 25; // Massive keep
    
    // Keep Solid block & Hollow out
    fillRect(kC0, kC1, kR0, kR1, ground+1, ground+kH, BLOCK_STONE);
    fillRect(kC0+2, kC1-2, kR0+2, kR1-2, ground+1, ground+kH-1, BLOCK_AIR);
    
    // Keep inner floors
    for(int lvl=1; lvl<=4; lvl++) {
        int floorH = ground + lvl*6;
        if(floorH < ground+kH) {
            fillRect(kC0+2, kC1-2, kR0+2, kR1-2, floorH, floorH, BLOCK_WOOD);
        }
    }
    
    // Keep Entrance
    fillRect(midC-3, midC+3, kR1-1, kR1, ground+1, ground+7, BLOCK_AIR);
    // Grand wooden doors
    fillRect(midC-2, midC+2, kR1, kR1, ground+1, ground+5, BLOCK_WOOD);
    
    // Throne Room (ground floor of Keep)
    // Red carpet
    fillRect(midC-1, midC+1, kR0+4, kR1-2, ground+1, ground+1, BLOCK_SAND); // proxy for carpet
    // Throne
    setBlock(midC, kR0+4, ground+2, BLOCK_ORE_GOLD);
    setBlock(midC, kR0+4, ground+3, BLOCK_ORE_GOLD);
    
    // Tower spires on the keep
    int spH = 10;
    fillRect(kC0, kC0+3, kR0, kR0+3, ground+kH+1, ground+kH+spH, BLOCK_STONE_LIGHT);
    fillRect(kC1-3, kC1, kR0, kR0+3, ground+kH+1, ground+kH+spH, BLOCK_STONE_LIGHT);
    fillRect(kC0, kC0+3, kR1-3, kR1, ground+kH+1, ground+kH+spH, BLOCK_STONE_LIGHT);
    fillRect(kC1-3, kC1, kR1-3, kR1, ground+kH+1, ground+kH+spH, BLOCK_STONE_LIGHT);

    // Decorative flags/banners (proxy using grass/wood up very high)
    setBlock(midC, kR0+2, ground+kH+1, BLOCK_WOOD);
    setBlock(midC, kR0+2, ground+kH+2, BLOCK_WOOD);
    setBlock(midC, kR0+2, ground+kH+3, BLOCK_WOOD);
    setBlock(midC-1, kR0+2, ground+kH+3, BLOCK_GRASS);

    // Remove trees/torches inside
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
    
    printf("[Epic Castle] Built at col %d..%d, row %d..%d\n", c0, c1, r0, r1);
}

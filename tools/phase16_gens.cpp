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

import sys

with open('world.h', 'r') as f:
    lines = f.readlines()

new_lines = []
in_init = False
for i, line in enumerate(lines):
    # 1. Insert generators before initBlockGrid
    if 'void initBlockGrid()' in line:
        with open('phase16_gens.cpp', 'r') as gf:
            new_lines.extend(gf.readlines())
        new_lines.append(line)
        in_init = True
        continue
    
    # 2. Modify Underground Layers (Ores + Ravines)
    if '// Cave carving: 3D noise' in line:
        # We will replace the entire Cave and Ore block with cluster logic
        pass

    if 'unsigned int oreSeed = gridSeed(col + h * 7, row + h * 13);' in line:
        pass # replacing

    if 'if (h <= 2) {' in line and in_init and 'oreSeed' in lines[i-1]:
        # Skip the old ore logic block entirely
        pass

    if 'setBlock(col, row, h, blockType);' in line and in_init and 'else blockType = BLOCK_STONE;' in lines[i-3]:
        # Insert new 16A/16B logic here, replacing the old 30 lines
        new_lines.append("""
                // 16G: Ravines cutting down to y=3
                bool isRavine = abs(fbmNoise(col * 0.05f + 100.0f, row * 0.05f + 100.0f, 2)) < 0.02f;
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

                // 16B: Clustered Ore Veins (checking neighbor blocks mathematically to form clusters)
                int blockType = BLOCK_STONE;
                unsigned int oreSeed = gridSeed(col/2 + h*7, row/2 + h*13); // cluster seed spanning 2x2x2
                if (h <= 3) {
                    if (oreSeed % 15 == 0) blockType = BLOCK_ORE_DIAMOND;
                } else if (h <= 6) {
                    if (oreSeed % 18 == 0) blockType = BLOCK_ORE_GOLD;
                    else if (oreSeed % 12 == 0) blockType = BLOCK_COAL_ORE;
                } else {
                    if (oreSeed % 10 == 0) blockType = BLOCK_COAL_ORE;
                    else if (oreSeed % 20 == 0) blockType = BLOCK_GRAVEL;
                }
                setBlock(col, row, h, blockType);
""")
        continue

    # Skip old ore lines
    if in_init:
        if 'if (h <= 2) {' in line or 'if (oreSeed %' in line or 'else blockType =' in line or 'else if (oreSeed' in line or '} else if (h <=' in line or '} else {' in line:
            # We check if it's the specific old block. A bit hacky, let's keep it simple.
            if i > 1050 and i < 1100:
                continue

        if 'float caveN1 =' in line or 'float caveN2 =' in line or 'if (h >= 3 && h <= 8' in line or 'setBlock(col, row, h, BLOCK_AIR);' in line and i < 1100 and i > 1050:
            if 'cave carving' not in line.lower():
                continue

    # 3. Surface modifiers (16D, 16E)
    if 'buildMedievalCastle' in line:
        new_lines.append(line)
        new_lines.append("""
    // 16C, 16D, 16E: Procedural Structures
    // Scatter structures deterministically across the world map
    for (int sc = -80; sc <= 80; sc += 30) {
        for (int sr = -80; sr <= 80; sr += 30) {
            unsigned int ss = gridSeed(sc, sr);
            int sb = getBiome(sc, sr);
            int sh = getTerrainHeightBiome(sc, sr, sb);
            
            // 16C: Underground Dungeon
            if (ss % 4 == 0) {
                buildDungeon(sc, sr, 3 + (ss % 4));
            }
            
            // 16E: Watchtower in stone biome
            if (sb == 2 && ss % 3 == 0) buildWatchtower(sc, sr);
            
            // 16E: Pyramid in sand biome
            if (sb == 0 && ss % 3 == 0) buildPyramid(sc, sr);
            
            // 16D: Village in grass biome
            if (sb == 1 && ss % 5 == 0) buildVillage(sc, sr);
        }
    }
""")
        continue

    new_lines.append(line)

# Clean up any leftover old underground ore lines by replacing the whole segment manually using string replace over the whole file text.
with open('world.h', 'r') as f:
    full_text = f.read()

import re
# We will do a full text replacement to wipe out the old Cave+Ore loop entirely and insert the 16A/16B
old_underground = re.search(r'// Cave carving.*?setBlock\(col, row, h, blockType\);\n\s+}', full_text, re.DOTALL)
if old_underground:
    new_underground = """// 16G: Ravines cutting down to y=3
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
            }"""
    full_text = full_text.replace(old_underground.group(0), new_underground)

# Insert the functions right before initBlockGrid
with open('phase16_gens.cpp', 'r') as f:
    funcs = f.read()

full_text = full_text.replace('void initBlockGrid() {', funcs + '\\nvoid initBlockGrid() {')

# Insert the scatter logic after buildMedievalCastle
scatter_logic = """
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
"""
full_text = full_text.replace('    buildMedievalCastle(-30, -5);\n', '    buildMedievalCastle(-30, -5);\n' + scatter_logic)

# Setup Biome 4 (Snow) in getBiome
old_biome = """    if (n < -0.35f) return 3; // water
    if (n < -0.05f) return 0; // sand
    if (n < 0.45f)  return 1; // grass
    return 2;                  // stone"""
    
new_biome = """    float temp = fbmNoise(col * 0.015f - 50.0f, row * 0.015f + 200.0f, 2);
    if (temp < -0.3f && n >= -0.05f) return 4; // snow biome
    if (n < -0.35f) return 3; // water
    if (n < -0.05f) return 0; // sand
    if (n < 0.45f)  return 1; // grass
    return 2;                  // stone"""
full_text = full_text.replace(old_biome, new_biome)

# Add Snow Biome surface block selection
old_surface = """                        case 0: blockType = BLOCK_SAND; break;
                        case 1: {"""
new_surface = """                        case 4: blockType = BLOCK_SNOW; break;
                        case 0: blockType = BLOCK_SAND; break;
                        case 1: {"""
full_text = full_text.replace(old_surface, new_surface)

# Add Snow Biome depth handling
old_depth = """                        case 0: blockType = (depthFromTop < 3) ? BLOCK_SAND : BLOCK_STONE_LIGHT; break;
                        case 1: blockType = BLOCK_DIRT; break;"""
new_depth = """                        case 4: blockType = BLOCK_DIRT; break;
                        case 0: blockType = (depthFromTop < 3) ? BLOCK_SAND : BLOCK_STONE_LIGHT; break;
                        case 1: blockType = BLOCK_DIRT; break;"""
full_text = full_text.replace(old_depth, new_depth)

# Fix ice in water biome mapping
old_water = """                    // Check if it's deeply underwater for sand/dirt
                    if (h < UNDERGROUND_DEPTH) {
                        blockType = BLOCK_DIRT;"""
new_water = """                    // 16F: Ice mapping over water
                    if (h == topH && getBiome(col, row) == 4) blockType = BLOCK_ICE; // wait, biome 4 land doesn't do water.
                    // Actually, if we are in water biomes near cold zones:
                    float wtemp = fbmNoise(col * 0.015f - 50.0f, row * 0.015f + 200.0f, 2);
                    if (h == topH && wtemp < -0.3f) {
                        setBlock(col, row, topH + 1, BLOCK_ICE);
                    }

                    if (h < UNDERGROUND_DEPTH) {
                        blockType = BLOCK_DIRT;"""
full_text = full_text.replace(old_water, new_water)


with open('world.h', 'w') as f:
    f.write(full_text)

print("Applied changes to world.h")

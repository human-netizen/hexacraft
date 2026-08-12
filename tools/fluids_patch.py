import sys

with open('world.h', 'r') as f:
    world_text = f.read()

fluid_logic = """
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
"""

world_text = world_text.replace('void initBlockGrid() {', fluid_logic + '\nvoid initBlockGrid() {\n')

with open('world.h', 'w') as f:
    f.write(world_text)

with open('main.cpp', 'r') as f:
    main_text = f.read()

main_text = main_text.replace('updateMobs(deltaTime, (float)glfwGetTime());', 'updateMobs(deltaTime, (float)glfwGetTime());\n        updateFluids((float)glfwGetTime());')

with open('main.cpp', 'w') as f:
    f.write(main_text)

print("Injected fluid dynamics!")

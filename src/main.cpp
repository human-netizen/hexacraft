// =====================================================
// HexaCraft — Main entry point
// All code is split into header files (single compilation unit)
// =====================================================

#include "globals.h"
#include "shaders.h"
#include "geometry.h"
#include "world.h"
#include "player.h"
#include "objects.h"
#include "hud.h"
#include "input.h"
#include "skybox.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Load a texture from file and return GL texture ID
GLuint loadTexture(const char* path) {
    int w, h, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &channels, 0);
    if (!data) {
        printf("[Texture] Failed to load: %s\n", path);
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // NEAREST magnification keeps 16x16 pixel-art blocks crisp instead of smeared.
    // Minification stays trilinear so distant blocks do not shimmer.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    printf("[Texture] Loaded: %s (%dx%d, %d channels)\n", path, w, h, channels);
    return tex;
}

// =====================================================
// Print controls
// =====================================================
void printControls() {
    printf("========================================\n");
    printf("  HexaCraft - Controls\n");
    printf("========================================\n");
    printf("  Player Movement:\n");
    printf("    W / S       - Walk Forward / Backward\n");
    printf("    A / D       - Strafe Left / Right\n");
    printf("    SPACE       - Jump\n");
    printf("    Q (hold)    - Fly up (release to fall)\n");
    printf("    L.SHIFT     - Sprint (1.5x speed, drains stamina)\n");
    printf("    E / R       - Fly Up / Down (alt)\n");
    printf("\n");
    printf("  Camera:\n");
    printf("    C           - Cycle: Third-Person > First-Person > Free-Fly\n");
    printf("    Mouse Move               - Look around\n");
    printf("    X           - Pitch up   (Shift+X = down)\n");
    printf("    Y           - Yaw right  (Shift+Y = left)\n");
    printf("    Z           - Roll CW    (Shift+Z = CCW)\n");
    printf("    B           - Toggle Bird's Eye View\n");
    printf("    F           - Rotate around look-at (Free-Fly only)\n");
    printf("    V           - Toggle 4-Viewport split\n");
    printf("\n");
    printf("  Lighting Toggles:\n");
    printf("    1           - Directional light on/off\n");
    printf("    2           - Point lights on/off\n");
    printf("    3           - Spot light on/off\n");
    printf("    5           - Ambient on/off\n");
    printf("    6           - Diffuse on/off\n");
    printf("    7           - Specular on/off\n");
    printf("    L           - Master light on/off\n");
    printf("    H           - Toggle Gouraud / Phong shading\n");
    printf("\n");
    printf("  Day-Night:\n");
    printf("    T           - Cycle: Night > Dawn > Noon > Dusk\n");
    printf("\n");
    printf("  Debug:\n");
    printf("    F7          - Toggle baked tree meshes (off = live fractal, slower)\n");
    printf("\n");
    printf("  Interactive Objects:\n");
    printf("    G           - Toggle rotating fan\n");
    printf("    O           - Toggle door open/close\n");
    printf("    P           - Toggle windows open/close (or respawn when dead)\n");
    printf("\n");
    printf("  MineCar:\n");
    printf("    Arrow Keys  - Drive (UP/DOWN = accel, LEFT/RIGHT = steer)\n");
    printf("\n");
    printf("  Block Building:\n");
    printf("    Left Click               - Break block (bedrock unbreakable)\n");
    printf("    Right Click              - Place block\n");
    printf("    Scroll                   - Cycle hotbar slot\n");
    printf("    Ctrl + Scroll            - Zoom in/out\n");
    printf("    Numpad 1-9               - Quick-select hotbar\n");
    printf("\n");
    printf("  ESC           - Quit\n");
    printf("========================================\n");
}

// =====================================================
// Main
// =====================================================
int main() {
    if (!glfwInit()) { printf("GLFW init failed\n"); return -1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, "HexaCraft", NULL, NULL);
    if (!window) { printf("Window creation failed\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    // Vsync. Nothing was calling glfwSwapInterval() at all, so the swap chain ran
    // free and the image could tear.
    //
    // Prefer ADAPTIVE vsync: sync when the frame makes the refresh deadline, tear
    // rather than wait when it misses. Plain vsync would round a 37 ms frame up to
    // a full 50 ms on a 60 Hz display — a real frame-rate loss to fix tearing.
    // HEXA_NOVSYNC=1 turns it off entirely so the frame profiler can still measure
    // uncapped frame times.
    if (getenv("HEXA_NOVSYNC")) {
        glfwSwapInterval(0);
        printf("[Boot] Vsync disabled (HEXA_NOVSYNC)\n");
    } else if (glfwExtensionSupported("GLX_EXT_swap_control_tear") ||
               glfwExtensionSupported("WGL_EXT_swap_control_tear")) {
        glfwSwapInterval(-1);
        printf("[Boot] Adaptive vsync enabled\n");
    } else {
        glfwSwapInterval(1);
        printf("[Boot] Adaptive vsync unavailable; plain vsync enabled\n");
    }
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // capture mouse like Minecraft

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("GLAD init failed\n"); return -1;
    }

    printf("OpenGL %s\n", glGetString(GL_VERSION));
    printControls();

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.02f, 0.02f, 0.06f, 1.0f);

    // Paint one frame before the ~300 ms of texture and terrain loading below.
    // Without this the window sits blank (whatever the compositor last had there)
    // from creation until the first swap, which reads as a hang on startup.
    // Nothing is loaded yet — no shader, no mesh — so this is a bare clear, which
    // is exactly what a loading screen can be at this point in startup.
    {
        double t0 = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
        printf("[Boot] Loading frame shown at %.0f ms\n", t0 * 1000.0);
    }

    shaderProgram = loadShaders("shaders/vertexShader.glsl", "shaders/fragmentShader.glsl");
    // uvRect defaults to all-zero in GL, which would collapse every UV to a
    // single texel. Establish identity before anything draws.
    glUseProgram(shaderProgram);
    resetUVRect(shaderProgram);
    initSkybox();
    initHexMesh();
    initBoxMesh();
    initSphere();
    initCone();
    initBezier();
    initSpline();
    initRuledSurface();
    initWineGlass();
    initSlabMesh();
    initPanelMesh();
    initHUD();

    // Load textures — local
    texBrick = loadTexture("assets/srcs/brick.png");
    texGrass = loadTexture("assets/srcs/grass.png");
    texWood  = loadTexture("assets/srcs/wood.png");
    texAmbu  = loadTexture("assets/srcs/ambu.png");

    // Load textures — FM Default pack
    texStoneFM          = loadTexture("assets/srcs/stone.png");
    texCobblestone      = loadTexture("assets/srcs/cobblestone.png");
    texMossyCobble      = loadTexture("assets/srcs/mossy_cobblestone.png");
    texStoneBricks      = loadTexture("assets/srcs/stone_bricks.png");
    texMossyStoneBricks = loadTexture("assets/srcs/mossy_stone_bricks.png");
    texCrackedStoneBricks = loadTexture("assets/srcs/cracked_stone_bricks.png");
    texOakPlanks        = loadTexture("assets/srcs/oak_planks.png");
    texBookshelf        = loadTexture("assets/srcs/bookshelf.png");
    texIronBars         = loadTexture("assets/srcs/iron_bars.png");
    texGlassFM          = loadTexture("assets/srcs/glass.png");
    texGlassPaneTop     = loadTexture("assets/srcs/glass_pane_top.png");
    texLadder           = loadTexture("assets/srcs/ladder.png");
    texIronDoorBot      = loadTexture("assets/srcs/iron_door_bottom.png");
    texIronDoorTop      = loadTexture("assets/srcs/iron_door_top.png");
    texItemOakDoor      = loadTexture("assets/srcs/oak_door.png");
    texItemIronDoor     = loadTexture("assets/srcs/iron_door.png");
    // Tool sprites
    texItemStick           = loadTexture("assets/srcs/stick.png");
    texItemSwordWood       = loadTexture("assets/srcs/wooden_sword.png");
    texItemAxeWood         = loadTexture("assets/srcs/wooden_axe.png");
    texItemPickaxeWood     = loadTexture("assets/srcs/wooden_pickaxe.png");
    texItemShovelWood      = loadTexture("assets/srcs/wooden_shovel.png");
    texItemSwordStone      = loadTexture("assets/srcs/stone_sword.png");
    texItemAxeStone        = loadTexture("assets/srcs/stone_axe.png");
    texItemPickaxeStone    = loadTexture("assets/srcs/stone_pickaxe.png");
    texItemShovelStone     = loadTexture("assets/srcs/stone_shovel.png");
    texItemSwordIron       = loadTexture("assets/srcs/iron_sword.png");
    texItemAxeIron         = loadTexture("assets/srcs/iron_axe.png");
    texItemPickaxeIron     = loadTexture("assets/srcs/iron_pickaxe.png");
    texItemShovelIron      = loadTexture("assets/srcs/iron_shovel.png");
    texItemSwordDiamond    = loadTexture("assets/srcs/diamond_sword.png");
    texItemAxeDiamond      = loadTexture("assets/srcs/diamond_axe.png");
    texItemPickaxeDiamond  = loadTexture("assets/srcs/diamond_pickaxe.png");
    texItemShovelDiamond   = loadTexture("assets/srcs/diamond_shovel.png");
    texItemBow             = loadTexture("assets/srcs/bow.png");
    texTorchBlock       = loadTexture("assets/srcs/torch.png");
    texSeaLantern       = loadTexture("assets/srcs/sea_lantern.png");
    texCraftingTop      = loadTexture("assets/srcs/crafting_table_top.png");
    texCraftingFront    = loadTexture("assets/srcs/crafting_table_front.png");
    texBedrock          = loadTexture("assets/srcs/bedrock.png");
    texCoalOre          = loadTexture("assets/srcs/coal_ore.png");
    texDiamondOre       = loadTexture("assets/srcs/diamond_ore.png");
    texGoldOre          = loadTexture("assets/srcs/gold_ore.png");

    // Gold resource pack textures (blocks not in FM Default)
    texGrassTop     = loadTexture("assets/srcs/grass_block_top.png");
    texSandGold     = loadTexture("assets/srcs/sand.png");
    texOakLog       = loadTexture("assets/srcs/oak_log.png");
    texOakLeaves    = loadTexture("assets/srcs/oak_leaves.png");
    texSnow         = loadTexture("assets/srcs/snow.png");
    texIceGold      = loadTexture("assets/srcs/ice.png");
    texClayGold     = loadTexture("assets/srcs/clay.png");
    texGravelGold   = loadTexture("assets/srcs/gravel.png");
    texGlowstoneGold= loadTexture("assets/srcs/glowstone.png");
    texDiamondBlock = loadTexture("assets/srcs/diamond_block.png");
    texGoldBlock    = loadTexture("assets/srcs/gold_block.png");
    texIronBlockTex = loadTexture("assets/srcs/iron_block.png");
    texObsidian     = loadTexture("assets/srcs/obsidian.png");
    texSandstoneGold= loadTexture("assets/srcs/sandstone.png");
    texCutSandstone = loadTexture("assets/srcs/cut_sandstone.png");
    texQuartzTop    = loadTexture("assets/srcs/quartz_block_top.png");
    texAndesite     = loadTexture("assets/srcs/andesite.png");
    texDiorite      = loadTexture("assets/srcs/diorite.png");
    texGranite      = loadTexture("assets/srcs/granite.png");
    texPolAndesite  = loadTexture("assets/srcs/polished_andesite.png");
    texPolDiorite   = loadTexture("assets/srcs/polished_diorite.png");
    texPolGranite   = loadTexture("assets/srcs/polished_granite.png");
    texOakTrapdoor  = loadTexture("assets/srcs/oak_trapdoor.png");
    texOakDoorBot   = loadTexture("assets/srcs/oak_door_bottom.png");
    texWaterStill   = loadTexture("assets/srcs/water_still.png");

    printf("[World] Generating terrain...\n");
    initSightTable();
    initBlockGrid();
    buildKUETHill();
    // Must run after every structure is built — it snapshots where the ground is
    // so the render loop never has to scan a column again.
    resolveGroundHeights();
    printf("[World] Terrain ready! %dx%d grid, %d height layers\n", GRID_W, GRID_D, GRID_H);

    // Bake tree meshes once — needs the grid (tree list) and a GL context.
    initTreeMeshes();

    // Initialize player on terrain
    int spawnCol, spawnRow;
    findSpawnColumn(spawnCol, spawnRow);
    glm::vec3 spawnGrid = hexGridPos(spawnCol, spawnRow, 0.0f);
    playerWorldPos = glm::vec3(spawnGrid.x, getGroundY(spawnCol, spawnRow), spawnGrid.z);
    printf("[Player] Spawned at (%.1f, %.1f, %.1f)\n", playerWorldPos.x, playerWorldPos.y, playerWorldPos.z);

    // Spawn initial mobs and birds
    spawnInitialMobs();
    initBirds();
    // Initialize Inventory slots
    for (int i=0; i<36; i++) { playerInventory[i].type = BLOCK_AIR; playerInventory[i].count = 0; }
    // Hotbar (slots 0-8)
    playerInventory[0].type = BLOCK_GRASS;        playerInventory[0].count = 64;
    playerInventory[1].type = BLOCK_DIRT;         playerInventory[1].count = 64;
    playerInventory[2].type = BLOCK_SAND;         playerInventory[2].count = 64;
    playerInventory[3].type = BLOCK_STONE;        playerInventory[3].count = 64;
    playerInventory[4].type = BLOCK_WOOD;         playerInventory[4].count = 64;
    playerInventory[5].type = BLOCK_LEAF;         playerInventory[5].count = 64;
    playerInventory[6].type = BLOCK_ORE_DIAMOND;  playerInventory[6].count = 64;
    playerInventory[7].type = BLOCK_ORE_GOLD;     playerInventory[7].count = 64;
    playerInventory[8].type = BLOCK_GLASS;        playerInventory[8].count = 64;
    // Storage (slots 9+) — testing materials for new blocks
    playerInventory[9].type  = BLOCK_PLANKS;      playerInventory[9].count  = 64; // door, ladder, stairs, slab
    playerInventory[10].type = BLOCK_COBBLESTONE; playerInventory[10].count = 64; // fence, fence gate
    playerInventory[11].type = BLOCK_IRON_BLOCK;  playerInventory[11].count = 64; // iron door, iron bars
    playerInventory[12].type = BLOCK_WOOL_WHITE;  playerInventory[12].count = 64; // carpets, other wools
    playerInventory[13].type = BLOCK_WOOL_RED;    playerInventory[13].count = 64;
    playerInventory[14].type = BLOCK_WOOL_BLUE;   playerInventory[14].count = 64;
    playerInventory[15].type = BLOCK_GLOWSTONE;   playerInventory[15].count = 64; // bookshelf, lantern
    playerInventory[16].type = BLOCK_SANDSTONE;   playerInventory[16].count = 64; // sandstone slab
    playerInventory[17].type = BLOCK_BRICKS;      playerInventory[17].count = 64; // brick slab

    // Set camera behind player initially
    camYaw = -90.0f;
    camPitch = -15.0f;
    updateCameraVectors();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        updateBlockTarget();   // must run before processInput so target is current
        processInput(window);
        updateMobs(deltaTime, (float)glfwGetTime());
        updateFluids((float)glfwGetTime());

        // Animate fan: continuous rotation at ~180 deg/s (≈ π rad/s)
        if (fanOn) fanAngle += 3.14159f * deltaTime;

        // Animate door — exponential easing toward 0° (closed) or 90° (open)
        float doorTarget = doorOpen ? 90.0f : 0.0f;
        doorAngle += (doorTarget - doorAngle) * (1.0f - expf(-8.0f * deltaTime));
        if (fabsf(doorAngle - doorTarget) < 0.05f) doorAngle = doorTarget;

        // Animate window — exponential easing toward 0° (closed) or 90° (open)
        float winTarget = windowOpen ? 90.0f : 0.0f;
        windowAngle += (winTarget - windowAngle) * (1.0f - expf(-6.0f * deltaTime));
        if (fabsf(windowAngle - winTarget) < 0.05f) windowAngle = winTarget;

        // Update birds
        updateBirds(deltaTime, (float)glfwGetTime());

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        float curTime = (float)glfwGetTime();
        setFloat(shaderProgram, "time", curTime);

        // Sky color based on day mode (with dawn/dusk orange)
        glm::vec3 sky;
        if (dayMode == 0) { // night
            sky = glm::vec3(0.02f, 0.02f, 0.06f);
        } else if (dayMode == 1) { // dawn
            sky = glm::vec3(0.55f, 0.35f, 0.25f); // warm orange
        } else if (dayMode == 2) { // noon
            sky = glm::vec3(0.45f, 0.65f, 0.95f); // bright blue
        } else { // dusk
            sky = glm::vec3(0.5f, 0.3f, 0.2f); // orange-red
        }
        glClearColor(sky.r, sky.g, sky.b, 1.0f);

        // Fog (blends to sky color). Density is derived from RENDER_DIST rather
        // than hardcoded: the old fixed 0.0002 left terrain 61% visible at the
        // 50-unit cutoff, so blocks visibly popped out of existence at the edge.
        // Fog is held off until fogStart and reaches ~99% right at the cutoff,
        // which hides the edge without hazing over the near field.
        setVec3(shaderProgram, "fogColor", sky);
        const float FOG_START_FRAC = 0.45f;  // clear out to 45% of the render radius
        float fogStart = RENDER_DIST * FOG_START_FRAC;
        float fogSpan  = RENDER_DIST - fogStart;
        // exp(-k) = 0.01 at the cutoff -> k = 4.6
        float fogDens  = 4.6f / (fogSpan * fogSpan);
        // Slightly thicker haze at dawn/dusk. This used to be a flat 2x, which was
        // harmless when the base density was far too low but now saturates the fog
        // by ~35 units and turns dawn into an orange wash.
        if (dayMode == 1 || dayMode == 3) fogDens *= 1.25f;
        setFloat(shaderProgram, "fogStart", fogStart);
        setFloat(shaderProgram, "fogDensity", fogDens);
        currentFogDensity = fogDens;

        // Lighting toggles
        setBool(shaderProgram, "lightOn", lightOn);
        setBool(shaderProgram, "dirLightOn", dirLightOn);
        setBool(shaderProgram, "pointLightOn", pointLightOn);
        setBool(shaderProgram, "spotLightOn", spotLightOn);
        setBool(shaderProgram, "ambientOn", ambientOn);
        setBool(shaderProgram, "diffuseOn", diffuseOn);
        setBool(shaderProgram, "specularOn", specularOn);
        setBool(shaderProgram, "isEmissive", false);
        setBool(shaderProgram, "isHUD", false);
        // dayFactor is what the sky, the mob spawner and the star field key off.
        // It used to be a second global written ONLY by the T key, so anything
        // that changed dayMode by another route (a script, a future day/night
        // timer) left the sky lit for noon over pitch-dark terrain. Derive it
        // from dayMode every frame so the two cannot drift apart.
        {
            static const float DAY_FACTORS[4] = {0.0f, 0.4f, 1.0f, 0.3f}; // night,dawn,noon,dusk
            dayFactor = DAY_FACTORS[((dayMode % 4) + 4) % 4];
        }
        setFloat(shaderProgram, "dayFactor", dayFactor);

        // Texture and shading defaults
        setInt(shaderProgram, "textureMode", 0);    // no texture by default
        setBool(shaderProgram, "useGouraud", useGouraud);

        // Damage tint defaults (reset each frame)
        setVec3(shaderProgram, "colorTint", glm::vec3(0.0f));
        setFloat(shaderProgram, "colorTintStrength", 0.0f);

        // Directional light (sun angle changes with day)
        glm::vec3 sunDir;
        glm::vec3 sunColor;
        if (dayMode == 0) { // night
            sunDir = glm::vec3(0.2f, -0.5f, 0.3f);
            sunColor = glm::vec3(0.15f, 0.15f, 0.25f);
        } else if (dayMode == 1) { // dawn
            sunDir = glm::vec3(-0.8f, -0.3f, -0.2f);
            sunColor = glm::vec3(0.8f, 0.5f, 0.3f);
        } else if (dayMode == 2) { // noon
            sunDir = glm::vec3(-0.3f, -1.0f, -0.5f);
            sunColor = glm::vec3(0.95f, 0.9f, 0.8f);
        } else { // dusk
            sunDir = glm::vec3(0.8f, -0.3f, 0.2f);
            sunColor = glm::vec3(0.8f, 0.35f, 0.2f);
        }
        setVec3(shaderProgram, "dirLightDir", sunDir);
        setVec3(shaderProgram, "dirLightColor", sunColor);

        // Spot light (follows camera like a flashlight)
        setVec3(shaderProgram, "spotLightPos", camPos);
        setVec3(shaderProgram, "spotLightDir", camFront);
        setVec3(shaderProgram, "spotLightColor", glm::vec3(1.0f, 1.0f, 0.9f));
        setFloat(shaderProgram, "spotCutoff", cosf(glm::radians(18.0f)));

        // Point light upload helper (call after torchPositions is populated)
        // Names are fixed literals rather than an sprintf'd buffer so the uniform
        // location cache in shaders.h can key on their (stable) addresses.
        static const char* kPointLightPosNames[8] = {
            "pointLightPos[0]", "pointLightPos[1]", "pointLightPos[2]", "pointLightPos[3]",
            "pointLightPos[4]", "pointLightPos[5]", "pointLightPos[6]", "pointLightPos[7]"
        };
        static const char* kPointLightColorNames[8] = {
            "pointLightColor[0]", "pointLightColor[1]", "pointLightColor[2]", "pointLightColor[3]",
            "pointLightColor[4]", "pointLightColor[5]", "pointLightColor[6]", "pointLightColor[7]"
        };
        auto uploadPointLights = [&]() {
            int count = (int)torchPositions.size();
            if (count > 8) count = 8;
            setInt(shaderProgram, "numPointLights", count);
            for (int i = 0; i < count; i++) {
                setVec3(shaderProgram, kPointLightPosNames[i], torchPositions[i]);
                setVec3(shaderProgram, kPointLightColorNames[i], glm::vec3(1.0f, 0.6f, 0.2f));
            }
        };

        // Helper lambda: render scene with given view/proj into a viewport rect
        auto renderViewport = [&](int vx, int vy, int vw, int vh,
                                   glm::mat4 vMat, glm::mat4 pMat, glm::vec3 eye) {
            glViewport(vx, vy, vw, vh);
            glEnable(GL_SCISSOR_TEST);
            glScissor(vx, vy, vw, vh);
            glClear(GL_DEPTH_BUFFER_BIT);
            // Draw skybox first (behind everything)
            drawSkybox(vMat, pMat, dayFactor, sky);
            glUseProgram(shaderProgram);
            setMat4(shaderProgram, "view", vMat);
            setMat4(shaderProgram, "projection", pMat);
            setVec3(shaderProgram, "viewPos", eye);
            currentVP = pMat * vMat;
            // Lights first: the uniforms must be live before anything is shaded,
            // otherwise the frame is lit by the previous frame's torch set.
            gatherTorchLights();
            uploadPointLights();
            renderSky(curTime, sunDir);
            renderTerrain(curTime);
            renderObjects(curTime);
            if (isBreaking && hasTarget) {
                int _bt = getBlock(targetCol, targetRow, targetHeight);
                float _hard = getBlockHardness(_bt);
                float _speed = getToolSpeedMultiplier(playerInventory[hotbarSlot].type, _bt);
                float _dur = (_hard > 0.0f && _speed > 0.0f) ? _hard / _speed : 0.0f;
                float _prog = (_dur > 0.0f) ? (breakHoldTime / _dur) : 0.0f;
                drawBreakOverlay(_prog);
            }
            glDisable(GL_SCISSOR_TEST);
        };

        if (fourViewport) {
            int hw = w / 2, hh = h / 2;
            float aspect = (float)hw / (float)hh;
            glm::vec3 center = lookAtTarget;

            // Top-left: Free camera / current view
            {
                glm::mat4 v, p;
                if (birdsEye) {
                    glm::vec3 eye = center + glm::vec3(0, 25, 0.01f);
                    v = glm::lookAt(eye, center, glm::vec3(0, 0, -1));
                } else {
                    v = glm::lookAt(camPos, camPos + camFront, camUp);
                }
                p = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 200.0f);
                glm::vec3 eye = birdsEye ? center + glm::vec3(0, 25, 0.01f) : camPos;
                renderViewport(0, hh, hw, hh, v, p, eye);
            }

            // Top-right: Isometric view
            {
                glm::vec3 eye = center + glm::vec3(15, 15, 15);
                glm::mat4 v = glm::lookAt(eye, center, glm::vec3(0, 1, 0));
                float s = 12.0f;
                glm::mat4 p = glm::ortho(-s * aspect, s * aspect, -s, s, 0.1f, 200.0f);
                renderViewport(hw, hh, hw, hh, v, p, eye);
            }

            // Bottom-left: Top-down view
            {
                glm::vec3 eye = center + glm::vec3(0, 30, 0.01f);
                glm::mat4 v = glm::lookAt(eye, center, glm::vec3(0, 0, -1));
                float s = 15.0f;
                glm::mat4 p = glm::ortho(-s * aspect, s * aspect, -s, s, 0.1f, 200.0f);
                renderViewport(0, 0, hw, hh, v, p, eye);
            }

            // Bottom-right: Front view
            {
                glm::vec3 eye = center + glm::vec3(0, 5, 20);
                glm::mat4 v = glm::lookAt(eye, center, glm::vec3(0, 1, 0));
                float s = 10.0f;
                glm::mat4 p = glm::ortho(-s * aspect, s * aspect, -s, s, 0.1f, 200.0f);
                renderViewport(hw, 0, hw, hh, v, p, eye);
            }
        } else {
            // Single viewport
            glViewport(0, 0, w, h);
            float aspect = (float)w / (float)h;
            glm::mat4 view, proj;
            glm::vec3 eye;

            if (birdsEye) {
                eye = lookAtTarget + glm::vec3(0, 25, 0.01f);
                view = glm::lookAt(eye, lookAtTarget, glm::vec3(0, 0, -1));
                float s = 15.0f;
                proj = glm::ortho(-s * aspect, s * aspect, -s, s, 0.1f, 200.0f);
            } else {
                eye = camPos;
                view = glm::lookAt(camPos, camPos + camFront, camUp);
                proj = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 200.0f);
            }

            // Draw skybox first (behind everything)
            drawSkybox(view, proj, dayFactor, sky);
            glUseProgram(shaderProgram);
            setMat4(shaderProgram, "view", view);
            setMat4(shaderProgram, "projection", proj);
            setVec3(shaderProgram, "viewPos", eye);
            currentVP = proj * view;
            // Lights first — see the note in renderViewport above.
            gatherTorchLights();
            uploadPointLights();
            renderSky(curTime, sunDir);
            renderTerrain(curTime);
            renderObjects(curTime);
            if (isBreaking && hasTarget) {
                int _bt = getBlock(targetCol, targetRow, targetHeight);
                float _hard = getBlockHardness(_bt);
                float _speed = getToolSpeedMultiplier(playerInventory[hotbarSlot].type, _bt);
                float _dur = (_hard > 0.0f && _speed > 0.0f) ? _hard / _speed : 0.0f;
                float _prog = (_dur > 0.0f) ? (breakHoldTime / _dur) : 0.0f;
                drawBreakOverlay(_prog);
            }
        }

        // Render HUD (always on top, after all viewports)
        {
            int fw, fh;
            glfwGetFramebufferSize(window, &fw, &fh);
            glViewport(0, 0, fw, fh);
            renderHUD(fw, fh);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroySkybox();
    glDeleteVertexArrays(1, &hudVAO);
    glDeleteBuffers(1, &hudVBO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteVertexArrays(1, &coneVAO);
    glDeleteBuffers(1, &coneVBO);
    glDeleteVertexArrays(1, &bezierVAO);
    glDeleteBuffers(1, &bezierVBO);
    glDeleteVertexArrays(1, &splineVAO);
    glDeleteBuffers(1, &splineVBO);
    glDeleteVertexArrays(1, &ruledVAO);
    glDeleteBuffers(1, &ruledVBO);
    glDeleteVertexArrays(1, &hexVAO);
    glDeleteBuffers(1, &hexVBO);
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &boxVBO);
    destroyTreeMeshes();
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

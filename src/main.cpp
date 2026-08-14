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
#include "horizon.h"
// After world.h (findGroundH) and objects.h (worldToGrid).
#include "weather.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Load a texture from file and return GL texture ID.
//
// `pixelArt` picks the magnification filter. It defaults to true because all 88
// original textures here are 16x16 Minecraft art, where GL_LINEAR turns a block
// face into a smear. The gfx_b3 imports (car paint, road, roof, world map) are
// photographs several hundred pixels across, and NEAREST on those is worse in
// exactly the opposite way: a road stretched over several blocks magnifies past
// one texel per pixel and comes out visibly chunky. So they pass false.
// Minification is trilinear either way — distant blocks must not shimmer.
GLuint loadTexture(const char* path, bool pixelArt = true) {
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    pixelArt ? GL_NEAREST : GL_LINEAR);
    GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    printf("[Texture] Loaded: %s (%dx%d, %d channels)\n", path, w, h, channels);
    return tex;
}

// =====================================================
// Offscreen frame capture (development aid)
// =====================================================
// HEXA_SHOT=<path.ppm> grabs the framebuffer once, after HEXA_SHOT_DELAY seconds
// (default 6, enough for terrain generation to finish), then closes the window.
// Exists because the desktop screenshot tools do not work under this Wayland
// session, and rendering changes cannot be reviewed without seeing a frame.
// PPM because it needs no image library — the project vendors stb_image for
// reading but not stb_image_write, and one more dependency is not worth it for a
// debugging path. Convert with ImageMagick or ffmpeg.
void captureFrame(const char* path, int w, int h) {
    std::vector<unsigned char> px((size_t)w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    FILE* f = fopen(path, "wb");
    if (!f) { printf("[Shot] Cannot write %s\n", path); return; }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    // glReadPixels returns bottom-up; PPM is top-down.
    for (int y = h - 1; y >= 0; y--)
        fwrite(&px[(size_t)y * w * 3], 1, (size_t)w * 3, f);
    fclose(f);
    printf("[Shot] Wrote %s (%dx%d)\n", path, w, h);
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
    printf("    N           - Toggle rain\n");
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
    // These two were the wrong way round: mouse_button_callback places on left
    // and hold-to-breaks on right.
    printf("    Left Click               - Place block / use sculpt tool\n");
    printf("    Right Click (hold)       - Break block (bedrock unbreakable)\n");
    printf("    Hill / Pond tool         - Left Click reshapes terrain in an area\n");
    printf("    K                        - Grab / set down the targeted block\n");
    printf("    Scroll                   - Cycle hotbar slot\n");
    printf("    Ctrl + Scroll            - Zoom in/out\n");
    printf("    Numpad 1-9               - Quick-select hotbar\n");
    printf("\n");
    printf("  Build Tab (inventory screen):\n");
    printf("    House button             - Below the Recipe Book button\n");
    printf("    Click a recipe           - Builds it 3m ahead, facing you\n");
    printf("    House/Tree/Torch/Fireplace - Costs come out of your inventory\n");
    printf("\n");
    printf("  ESC           - Quit\n");
    printf("========================================\n");
}

// =====================================================
// HEXA_BUILD_TEST — assertion harness for plan_2 Step 4
// =====================================================
// Runs in-process after world generation, because everything under test reads
// real terrain: the flatness gate, the ground snap and the light gathering all
// need a generated world, and a stub grid would only prove the stub works.
static int  g_btPass = 0, g_btFail = 0;
static void CHK(bool cond, const char* what) {
    if (cond) { g_btPass++; printf("  ok    %s\n", what); }
    else      { g_btFail++; printf("  FAIL  %s\n", what); }
}

static void setSlot(int i, int type, int count) {
    playerInventory[i].type = type;
    playerInventory[i].count = count;
}
static void clearInventory() {
    for (int i = 0; i < 36; i++) setSlot(i, BLOCK_AIR, 0);
}

static void runBuildTests() {
    printf("\n=== plan_2 Step 4 — craft-and-place structures ===\n");

    // ---- Find genuinely level ground. Spawn is in the mountains, and every
    // ---- placement assertion below is meaningless on a slope.
    glm::vec3 site(0.0f);
    bool found = false;
    for (int c = -70; c <= 70 && !found; c += 2) {
        for (int r = -70; r <= 70 && !found; r += 2) {
            glm::vec3 p = hexGridPos(c, r, 0.0f);
            float lo = 1e9f, hi = -1e9f, gc = getGroundYWorld(p.x, p.z);
            lo = hi = gc;
            for (int i = 0; i < 12; i++) {
                float a = (float)i / 12.0f * 6.2831853f;
                float g = getGroundYWorld(p.x + cosf(a) * 5.0f, p.z + sinf(a) * 5.0f);
                if (g < lo) lo = g;
                if (g > hi) hi = g;
            }
            if (hi - lo < 0.51f) { site = glm::vec3(p.x, gc, p.z); found = true; }
        }
    }
    CHK(found, "found a level test site");
    printf("  [site] %.2f, %.2f, %.2f\n", site.x, site.y, site.z);

    playerWorldPos = site;
    camPos = site + glm::vec3(0, 1.62f, 0);
    camFront = glm::vec3(0.0f, 0.0f, -1.0f);

    // ---- Inventory tally and spend --------------------------------------
    clearInventory();
    setSlot(0, BLOCK_DIRT, 3);
    setSlot(7, BLOCK_DIRT, 4);
    CHK(countInInventory(BLOCK_DIRT) == 7, "countInInventory sums across split stacks");
    CHK(countInInventory(BLOCK_STONE) == 0, "countInInventory reports 0 for an absent type");

    CHK(removeFromInventory(BLOCK_DIRT, 5) == 5, "removeFromInventory takes the full amount");
    CHK(countInInventory(BLOCK_DIRT) == 2, "removeFromInventory leaves the remainder");
    CHK(playerInventory[0].type == BLOCK_AIR && playerInventory[0].count == 0,
        "emptied slot is cleared to AIR, not left as a 0-count ghost");

    CHK(removeFromInventory(BLOCK_DIRT, 5) == 2, "removeFromInventory reports a short take");
    CHK(countInInventory(BLOCK_DIRT) == 0, "a short take still drains what was there");

    // ---- Affordability gate ---------------------------------------------
    craftedHouses.clear(); craftedTrees.clear();
    craftedLights.clear(); craftedFireplaces.clear();

    clearInventory();
    CHK(!craftStructure(0), "house refused with an empty inventory");
    CHK(craftedHouses.empty(), "a refused build places nothing");

    // The no-partial-spend guarantee: removeFromInventory has no rollback, so
    // craftStructure must test BOTH ingredients before removing EITHER.
    clearInventory();
    setSlot(0, BLOCK_DIRT, 5);          // enough dirt, no stone at all
    CHK(!craftStructure(0), "house refused when only the first ingredient is present");
    CHK(countInInventory(BLOCK_DIRT) == 5, "a refused build spends nothing (no partial take)");

    // ---- A successful build ---------------------------------------------
    clearInventory();
    setSlot(0, BLOCK_DIRT, 9);
    setSlot(1, BLOCK_STONE, 4);
    CHK(craftStructure(0), "house built when both ingredients are present");
    CHK(craftedHouses.size() == 1, "house appended to craftedHouses");
    CHK(countInInventory(BLOCK_DIRT) == 4 && countInInventory(BLOCK_STONE) == 1,
        "house cost exactly 5 dirt + 3 stone");

    if (craftedHouses.size() == 1) {
        const PlacedStructure& h = craftedHouses[0];
        float r = HOUSE_D * HOUSE_CS * 0.5f;
        float dxz = sqrtf((h.pos.x - site.x) * (h.pos.x - site.x) +
                          (h.pos.z - site.z) * (h.pos.z - site.z));
        CHK(fabsf(dxz - (3.0f + r)) < 0.01f,
            "house centre offset by 3m + footprint radius, so the near wall is 3m out");
        CHK(h.pos.z < site.z, "house is in front of the player, not behind");

        // Ground snap: the stored y is the LOW sample of the footprint, which is
        // what buildSpotOk returns.
        glm::vec3 c(h.pos.x, 0.0f, h.pos.z);
        float g = 0.0f;
        CHK(buildSpotOk(c, r, g), "the chosen site passes its own flatness gate");
        CHK(fabsf(h.pos.y - g) < 0.001f, "house y snapped to the lowest footprint sample");

        // Yaw: local -Z must come back pointing at the player, i.e. at -fwd.
        // myRotate about +Y sends (0,0,-1) to (-sin, 0, -cos). Looking along -Z
        // puts the player on the house's +Z side, so its front faces +Z.
        glm::vec3 facing(-sinf(h.yaw), 0.0f, -cosf(h.yaw));
        CHK(facing.z > 0.99f, "house door faces back toward a player looking along -Z");
    }

    // Yaw again from a diagonal heading — the case a fixed-axis test would miss.
    craftedHouses.clear();
    clearInventory();
    setSlot(0, BLOCK_DIRT, 5);
    setSlot(1, BLOCK_STONE, 3);
    camFront = glm::normalize(glm::vec3(1.0f, -0.3f, -1.0f));
    if (craftStructure(0) && craftedHouses.size() == 1) {
        glm::vec3 fwd = glm::normalize(glm::vec3(camFront.x, 0.0f, camFront.z));
        float yaw = craftedHouses[0].yaw;
        glm::vec3 facing(-sinf(yaw), 0.0f, -cosf(yaw));
        CHK(glm::length(facing - (-fwd)) < 0.001f,
            "house front faces back toward the player on a diagonal heading");
    } else {
        CHK(false, "diagonal-heading house built");
    }
    camFront = glm::vec3(0.0f, 0.0f, -1.0f);

    // ---- Flatness rejection ----------------------------------------------
    // Raise a stone tower inside the next footprint. gfx_b3 needs no such gate —
    // its world is a flat plate — so this is the port's own failure mode.
    {
        glm::vec3 fwd(0.0f, 0.0f, -1.0f);
        glm::vec3 c = site + fwd * (3.0f + 0.7f);
        int tc, tr;
        worldToColRow(c.x, c.z, tc, tr);
        int base = (int)(site.y / HEX_HEIGHT);
        for (int h = base; h < base + 6; h++) setBlock(tc, tr, h, BLOCK_STONE);

        float g = 0.0f;
        CHK(!buildSpotOk(c, 0.7f, g), "flatness gate rejects a 6-block step in the footprint");

        clearInventory();
        setSlot(0, BLOCK_STONE, 2);
        setSlot(1, BLOCK_WOOD, 1);
        int before = (int)craftedFireplaces.size();
        CHK(!craftStructure(3), "fireplace refused on broken ground");
        CHK((int)craftedFireplaces.size() == before, "a ground-refused build places nothing");
        CHK(countInInventory(BLOCK_STONE) == 2 && countInInventory(BLOCK_WOOD) == 1,
            "a ground-refused build spends nothing");

        for (int h = base; h < base + 6; h++) setBlock(tc, tr, h, BLOCK_AIR);
    }

    // ---- Lights ----------------------------------------------------------
    craftedLights.clear(); craftedFireplaces.clear();
    clearInventory();
    setSlot(0, BLOCK_WOOD, 3);
    setSlot(1, BLOCK_STONE, 3);
    CHK(craftStructure(2), "torch built");
    CHK(craftStructure(3), "fireplace built");
    CHK(craftedLights.size() == 1 && craftedFireplaces.size() == 1,
        "torch and fireplace stored in their own lists");

    if (craftedLights.size() == 1 && craftedFireplaces.size() == 1) {
        // Stand right on top of them so they are unambiguously the nearest two
        // and survive the nearest-8 truncation whatever else is around.
        camPos = craftedLights[0] + glm::vec3(0, 1.0f, 0);
        gatherTorchLights();

        glm::vec3 wantTorch = craftedLights[0] + glm::vec3(0, 0.8f, 0);
        glm::vec3 wantFire  = craftedFireplaces[0].pos + glm::vec3(0, 0.35f, 0);
        int nTorch = 0, nFire = 0, iTorch = -1, iFire = -1;
        for (size_t i = 0; i < torchPositions.size(); i++) {
            const PointLightSrc& l = torchPositions[i];
            if (glm::length(l.pos - wantTorch) < 0.001f &&
                glm::length(l.color - COL_TORCH_FLAME) < 0.001f) { nTorch++; iTorch = (int)i; }
            if (glm::length(l.pos - wantFire) < 0.001f &&
                glm::length(l.color - COL_FIRE_HEARTH) < 0.001f) { nFire++; iFire = (int)i; }
        }
        CHK(nTorch == 1, "crafted torch registers exactly one point light (not zero, not two)");
        CHK(nFire == 1, "crafted fireplace registers exactly one point light");
        // gatherTorchLights does not shrink the list — it partial_sorts so the
        // nearest 8 sit at the front, and uploadPointLights in the render loop
        // takes that prefix. Standing on top of these two, they must be in it or
        // they light nothing.
        CHK(iTorch >= 0 && iTorch < 8, "crafted torch survives into the uploaded 8");
        CHK(iFire  >= 0 && iFire  < 8, "crafted fireplace survives into the uploaded 8");
    }

    // ---- Recipe table sanity ---------------------------------------------
    {
        bool allOk = true;
        for (int i = 0; i < NUM_BUILD_RECIPES; i++) {
            const BuildRecipe& br = buildRecipes[i];
            if (br.c1 <= 0 || br.c2 <= 0) allOk = false;
            if (strcmp(getBlockName(br.t1), "unknown") == 0) allOk = false;
            if (strcmp(getBlockName(br.t2), "unknown") == 0) allOk = false;
        }
        CHK(allOk, "every build recipe costs a positive amount of a real block type");
    }

    printf("=== %d passed, %d failed ===\n", g_btPass, g_btFail);
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
    // 4x MSAA. Must be requested before the window exists — the default
    // framebuffer's sample count is fixed at creation and cannot be changed
    // afterwards. A voxel world is nothing but hard silhouette edges, so this
    // buys more here than it would in a scene made of organic shapes.
    glfwWindowHint(GLFW_SAMPLES, 4);

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
    // The GLFW_SAMPLES hint only asks for a multisampled framebuffer; rasterising
    // into it still has to be switched on. Report what the driver actually gave
    // us — a request for 4 can silently come back as 0 on some configurations,
    // and without this the only symptom is "the edges still look jagged".
    glEnable(GL_MULTISAMPLE);
    {
        GLint samples = 0;
        glGetIntegerv(GL_SAMPLES, &samples);
        printf("[Boot] MSAA: %d samples\n", samples);
    }
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
    initCylinder();
    initBezier();
    initSpline();
    initRuledSurface();
    initWineGlass();
    initSlabMesh();
    initPanelMesh();
    initHUD();
    // After the mesh helpers: the ring is built with the shared Vertex layout.
    initHorizon();

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

    // gfx_b3 imports — photographs, so pixelArt = false (see loadTexture).
    // Four of these shipped as .jpg in gfx_b3 but are actually PNG; renamed on
    // import. stb_image sniffs magic bytes so the wrong name never broke
    // anything there, but a .jpg that is a PNG is a trap for the next reader.
    texCarBody   = loadTexture("assets/srcs/car_body.png", false);
    texCarWindow = loadTexture("assets/srcs/car_window.png", false);
    texRoad      = loadTexture("assets/srcs/road.png", false);
    texRoof      = loadTexture("assets/srcs/roof.png", false);
    texWorldMap  = loadTexture("assets/srcs/world_map_texture.jpg", false);
    texHillIcon  = loadTexture("assets/srcs/double_layer_mud_grass.png", false);

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
    initRain();   // after the spawn point is chosen — the first fill is camera-relative
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
    playerInventory[17].type = BLOCK_BRICKS;      playerInventory[17].count = 64;
    // Sculpt tools (plan_2 Step 3). count = 1 so the HUD draws no stack number
    // — drawSlotCount skips <= 1 — and nothing decrements them, because the
    // left-click handler returns before placeBlock() ever sees the slot.
    playerInventory[18].type = ITEM_TOOL_HILL;    playerInventory[18].count = 1;
    playerInventory[19].type = ITEM_TOOL_POND;    playerInventory[19].count = 1;

// brick slab

    // Set camera behind player initially
    camYaw = -90.0f;
    camPitch = -15.0f;
    updateCameraVectors();

    // HEXA_SHOT_CAM="x,y,z,yaw,pitch" drops the camera into free-fly at a fixed
    // pose. Pairs with HEXA_SHOT: together they make a given view reproducible
    // from the command line, which is the only way to compare a rendering change
    // against a before-shot without a human at the keyboard.
    if (const char* camEnv = getenv("HEXA_SHOT_CAM")) {
        float x, y, z, yaw, pitch;
        if (sscanf(camEnv, "%f,%f,%f,%f,%f", &x, &y, &z, &yaw, &pitch) == 5) {
            cameraMode = 2;             // free-fly
            camPos = glm::vec3(x, y, z);
            camYaw = yaw;
            camPitch = pitch;
            updateCameraVectors();
            printf("[Shot] Camera (%.1f, %.1f, %.1f) yaw %.0f pitch %.0f\n", x, y, z, yaw, pitch);
        }
    }

    // Shading, weather and time overrides. Same reason as HEXA_SHOT_CAM: an A/B
    // pair is only worth reading if both halves share a pose, and pressing H, N
    // or T by hand between two runs cannot guarantee that.
    if (getenv("HEXA_GOURAUD")) {
        useGouraud = true;
        printf("[Shot] Shading: Gouraud\n");
    }
    if (getenv("HEXA_RAIN")) {
        rainOn = true;
        printf("[Shot] Rain: ON\n");
    }
    // 0=Night 1=Dawn 2=Noon 3=Dusk — the same table the T key cycles through.
    if (const char* dayEnv = getenv("HEXA_DAY")) {
        const float factors[4] = {0.0f, 0.4f, 1.0f, 0.3f};
        const char*  names[4]  = {"Night", "Dawn", "Noon", "Dusk"};
        int m = atoi(dayEnv) & 3;
        dayMode = m;
        dayFactor = factors[m];
        printf("[Shot] Time: %s\n", names[m]);
    }

    // HEXA_BUILD_DEMO=1 — build one of each recipe on level ground and park the
    // camera in front of them, so HEXA_SHOT can capture evidence without a human
    // walking there and clicking four buttons. HEXA_BUILD_DEMO_NIGHT=1 does the
    // same at night, which is the only way to see the torch and fireplace light.
    if (getenv("HEXA_BUILD_DEMO")) {
        glm::vec3 site(0.0f);
        bool found = false;
        for (int c = -70; c <= 70 && !found; c += 2) {
            for (int r = -70; r <= 70 && !found; r += 2) {
                glm::vec3 p = hexGridPos(c, r, 0.0f);
                float gc = getGroundYWorld(p.x, p.z), lo = gc, hi = gc;
                for (int i = 0; i < 12; i++) {
                    float a = (float)i / 12.0f * 6.2831853f;
                    float g = getGroundYWorld(p.x + cosf(a) * 9.0f, p.z + sinf(a) * 9.0f);
                    if (g < lo) lo = g;
                    if (g > hi) hi = g;
                }
                if (hi - lo < 0.51f) { site = glm::vec3(p.x, gc, p.z); found = true; }
            }
        }
        if (found) {
            // Each recipe is built from the same spot but a different heading, so
            // they fan out around the site instead of stacking on each other —
            // and each one goes through craftStructure(), the real path.
            const float headings[4] = { 0.0f, 1.9f, 3.4f, 4.6f };  // radians
            for (int i = 0; i < NUM_BUILD_RECIPES; i++) {
                playerWorldPos = site;
                camFront = glm::vec3(sinf(headings[i]), 0.0f, -cosf(headings[i]));
                for (int s = 0; s < 36; s++) { playerInventory[s].type = BLOCK_AIR; playerInventory[s].count = 0; }
                playerInventory[0].type = buildRecipes[i].t1; playerInventory[0].count = buildRecipes[i].c1;
                playerInventory[1].type = buildRecipes[i].t2; playerInventory[1].count = buildRecipes[i].c2;
                craftStructure(i);
            }
            playerWorldPos = site;
            printf("[Demo] Site %.2f, %.2f, %.2f — %zu house, %zu tree, %zu torch, %zu fire\n",
                   site.x, site.y, site.z, craftedHouses.size(), craftedTrees.size(),
                   craftedLights.size(), craftedFireplaces.size());

            // HEXA_SHOT_CAM (handled above) wins if it was given — the demo only
            // supplies a default framing, and close-ups need a hand-picked one.
            if (!getenv("HEXA_SHOT_CAM")) {
                cameraMode = 2;
                camPos = site + glm::vec3(0.0f, 6.0f, 12.0f);
                // Look back at the site from the +Z side.
                glm::vec3 dir = glm::normalize(site + glm::vec3(0, 1.0f, 0) - camPos);
                camPitch = glm::degrees(asinf(dir.y));
                camYaw   = glm::degrees(atan2f(dir.z, dir.x));
                updateCameraVectors();
            }
        } else {
            printf("[Demo] no level site found\n");
        }
        if (getenv("HEXA_BUILD_DEMO_NIGHT")) { dayMode = 0; dayFactor = 0.0f; }

        // HEXA_BUILD_DEMO_UI=1 opens the crafting screen on the Build tab with a
        // deliberately partial inventory, so the shot shows both the affordable
        // and the unaffordable readout.
        if (getenv("HEXA_BUILD_DEMO_UI")) {
            for (int s = 0; s < 36; s++) { playerInventory[s].type = BLOCK_AIR; playerInventory[s].count = 0; }
            playerInventory[0].type = BLOCK_DIRT;  playerInventory[0].count = 12;
            playerInventory[1].type = BLOCK_STONE; playerInventory[1].count = 5;
            playerInventory[2].type = BLOCK_WOOD;  playerInventory[2].count = 1;
            inventoryOpen = true;
            buildTabOpen  = true;
        }
    }

    // HEXA_BUILD_TEST=1 — plan_2 Step 4 assertions. Runs here, after world
    // generation and inventory setup, then exits without entering the render
    // loop. It mutates the inventory and (briefly) the terrain, so it must never
    // run alongside a normal session.
    if (getenv("HEXA_BUILD_TEST")) {
        runBuildTests();
        glfwTerminate();
        return g_btFail == 0 ? 0 : 1;
    }

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        updateBlockTarget();   // must run before processInput so target is current
        processInput(window);
        updateMobs(deltaTime, (float)glfwGetTime());
        updateCarry();   // returns a held block to the world if the player died
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

        // Rain steps once per frame, before any viewport draws — see updateRain().
        updateRain(deltaTime, (float)glfwGetTime());

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
        // Off by default; renderTerrain turns it on for its block pass only.
        setBool(shaderProgram, "proceduralTint", false);
        // Likewise wind: nothing sways unless a draw asks for it. Goes through
        // setSway so the redundant-upload cache stays in step with the uniform.
        setSway(0.0f);
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
        // The moon is now its own directional light rather than the sun wearing a
        // blue tint. Its colour carries the whole day/night fade — the shader does
        // not scale it by dayFactor again — so setting it to black is what turns
        // the moon off at noon. Night's sun drops to almost nothing in exchange,
        // since the moon has taken over lighting the scene.
        glm::vec3 moonDir;
        glm::vec3 moonColor;
        if (dayMode == 0) { // night
            sunDir = glm::vec3(0.2f, -0.5f, 0.3f);
            sunColor = glm::vec3(0.03f, 0.03f, 0.05f);
            moonDir = glm::vec3(-0.35f, -0.8f, 0.25f);
            moonColor = glm::vec3(0.14f, 0.16f, 0.28f);
        } else if (dayMode == 1) { // dawn
            sunDir = glm::vec3(-0.8f, -0.3f, -0.2f);
            sunColor = glm::vec3(0.8f, 0.5f, 0.3f);
            moonDir = glm::vec3(0.7f, -0.5f, 0.2f);
            moonColor = glm::vec3(0.05f, 0.06f, 0.11f);
        } else if (dayMode == 2) { // noon
            sunDir = glm::vec3(-0.3f, -1.0f, -0.5f);
            sunColor = glm::vec3(0.95f, 0.9f, 0.8f);
            moonDir = glm::vec3(0.3f, -1.0f, 0.5f);
            moonColor = glm::vec3(0.0f);
        } else { // dusk
            sunDir = glm::vec3(0.8f, -0.3f, 0.2f);
            sunColor = glm::vec3(0.8f, 0.35f, 0.2f);
            moonDir = glm::vec3(-0.7f, -0.5f, -0.2f);
            moonColor = glm::vec3(0.07f, 0.08f, 0.15f);
        }
        setVec3(shaderProgram, "dirLightDir", sunDir);
        setVec3(shaderProgram, "dirLightColor", sunColor);
        setVec3(shaderProgram, "moonLightDir", moonDir);
        setVec3(shaderProgram, "moonLightColor", moonColor);

        // Specular defaults. bindBlockTexture() overrides these per block family;
        // everything else (mobs, props, HUD-adjacent geometry) inherits the old
        // uniform response, so nothing changes look without opting in.
        // Goes through resetBlockSpecular() rather than setFloat directly so the
        // redundant-upload cache in world.h stays in step with the real uniform.
        resetBlockSpecular();

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
                setVec3(shaderProgram, kPointLightPosNames[i], torchPositions[i].pos);
                setVec3(shaderProgram, kPointLightColorNames[i], torchPositions[i].color);
            }
            // Zero the slots past `count`. The loop in the shader stops at
            // numPointLights so these are not read today, but leaving a previous
            // frame's light sitting in the array is one edit away from becoming a
            // light that follows the player around with no source.
            for (int i = count; i < 8; i++) {
                setVec3(shaderProgram, kPointLightColorNames[i], glm::vec3(0.0f));
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
            drawHorizon(vMat, pMat, eye, dayFactor, sky);
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
            drawClouds(curTime, eye, dayFactor);
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
            // Last in the pass: rain is alpha-blended and has to composite over
            // the solid world.
            drawRain(curTime);
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
            drawHorizon(view, proj, eye, dayFactor, sky);
            glUseProgram(shaderProgram);
            setMat4(shaderProgram, "view", view);
            setMat4(shaderProgram, "projection", proj);
            setVec3(shaderProgram, "viewPos", eye);
            currentVP = proj * view;
            // Lights first — see the note in renderViewport above.
            gatherTorchLights();
            uploadPointLights();
            renderSky(curTime, sunDir);
            drawClouds(curTime, eye, dayFactor);
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
            drawRain(curTime);   // last — see the note in renderViewport
        }

        // Render HUD (always on top, after all viewports)
        {
            int fw, fh;
            glfwGetFramebufferSize(window, &fw, &fh);
            glViewport(0, 0, fw, fh);
            renderHUD(fw, fh);
        }

        // Development capture — see captureFrame(). Grabs before the swap so the
        // back buffer still holds the frame just drawn.
        if (const char* shotPath = getenv("HEXA_SHOT")) {
            const char* delayEnv = getenv("HEXA_SHOT_DELAY");
            float delay = delayEnv ? (float)atof(delayEnv) : 6.0f;
            // Frame rate, measured only after a warm-up. World generation and
            // texture loading take several seconds before the first frame ever
            // draws, and averaging that in hid a real difference when this was
            // used to A/B the cost of a render pass.
            //
            // The warm-up is counted in FRAMES, not wall-clock. An earlier
            // version started timing at delay*0.5, which silently degrades to
            // "measure everything" whenever world gen happens to run past that
            // mark — and gen time varies enough run to run that two A/B samples
            // were not measuring the same thing.
            const int SHOT_WARMUP = 30;
            static int   shotFrames = -SHOT_WARMUP;
            static float warmT = 0.0f;
            shotFrames++;
            if (shotFrames <= 0) warmT = currentTime;   // still warming up
            if (currentTime >= delay) {
                int sw, sh;
                glfwGetFramebufferSize(window, &sw, &sh);
                captureFrame(shotPath, sw, sh);
                float span = currentTime - warmT;
                if (shotFrames > 0 && span > 0.5f)
                    printf("[Shot] %d frames in %.1fs = %.1f fps\n",
                           shotFrames, span, shotFrames / span);
                else
                    printf("[Shot] fps not measured — only %d frames past warm-up; "
                           "raise HEXA_SHOT_DELAY\n", shotFrames);
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroySkybox();
    destroyHorizon();
    destroyRain();
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

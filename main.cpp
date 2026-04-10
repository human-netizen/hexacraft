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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
    glfwSetKeyCallback(window, keyCallback);
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

    shaderProgram = loadShaders("vertexShader.glsl", "fragmentShader.glsl");
    initHexMesh();
    initSphere();
    initCone();
    initBezier();
    initSpline();
    initRuledSurface();
    initWineGlass();
    initHUD();

    // Load textures
    texBrick = loadTexture("textures/brick.png");
    texGrass = loadTexture("textures/grass.png");
    texWood  = loadTexture("textures/wood.png");

    printf("[World] Generating terrain...\n");
    initBlockGrid();
    printf("[World] Terrain ready! %dx%d grid, %d height layers\n", GRID_W, GRID_D, GRID_H);

    // Initialize player on terrain
    glm::vec3 spawnGrid = hexGridPos(3, 5, 0.0f);
    playerWorldPos = glm::vec3(spawnGrid.x, getGroundY(3, 5), spawnGrid.z);
    printf("[Player] Spawned at (%.1f, %.1f, %.1f)\n", playerWorldPos.x, playerWorldPos.y, playerWorldPos.z);

    // Spawn initial mobs and birds
    spawnInitialMobs();
    initBirds();
    memset(inventoryCounts, 0, sizeof(inventoryCounts));

    // Set camera behind player initially
    camYaw = -90.0f;
    camPitch = -15.0f;
    updateCameraVectors();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        processInput(window);
        updateBlockTarget();
        updateMobs(deltaTime, (float)glfwGetTime());

        // Animate fan
        if (fanOn) fanAngle += 5.0f * deltaTime;

        // Animate door (smooth open/close)
        float doorTarget = doorOpen ? 90.0f : 0.0f;
        if (doorAngle < doorTarget) doorAngle += 120.0f * deltaTime;
        if (doorAngle > doorTarget) doorAngle -= 120.0f * deltaTime;
        if (fabsf(doorAngle - doorTarget) < 1.0f) doorAngle = doorTarget;

        // Animate window (smooth open/close)
        float winTarget = windowOpen ? 90.0f : 0.0f;
        if (windowAngle < winTarget) windowAngle += 120.0f * deltaTime;
        if (windowAngle > winTarget) windowAngle -= 120.0f * deltaTime;
        if (fabsf(windowAngle - winTarget) < 1.0f) windowAngle = winTarget;

        // Update birds
        updateBirds(deltaTime, (float)glfwGetTime());

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        float curTime = (float)glfwGetTime();
        setFloat(shaderProgram, "time", curTime);

        // Sky color based on day factor
        glm::vec3 skyDay(0.45f, 0.65f, 0.95f);
        glm::vec3 skyNight(0.02f, 0.02f, 0.06f);
        glm::vec3 sky = skyNight + dayFactor * (skyDay - skyNight);
        glClearColor(sky.r, sky.g, sky.b, 1.0f);

        // Fog (blends to sky color — objects fade before render cutoff at 75 units)
        setVec3(shaderProgram, "fogColor", sky);
        setFloat(shaderProgram, "fogDensity", 0.0007f);

        // Lighting toggles
        setBool(shaderProgram, "lightOn", lightOn);
        setBool(shaderProgram, "dirLightOn", dirLightOn);
        setBool(shaderProgram, "pointLightOn", pointLightOn);
        setBool(shaderProgram, "spotLightOn", spotLightOn);
        setBool(shaderProgram, "ambientOn", ambientOn);
        setBool(shaderProgram, "diffuseOn", diffuseOn);
        setBool(shaderProgram, "specularOn", specularOn);
        setBool(shaderProgram, "isEmissive", false);
        setFloat(shaderProgram, "dayFactor", dayFactor);

        // Texture and shading defaults
        setInt(shaderProgram, "textureMode", 0);    // no texture by default
        setBool(shaderProgram, "useGouraud", useGouraud);

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
        auto uploadPointLights = [&]() {
            int count = (int)torchPositions.size();
            if (count > 8) count = 8;
            setInt(shaderProgram, "numPointLights", count);
            for (int i = 0; i < count; i++) {
                char buf[64];
                sprintf(buf, "pointLightPos[%d]", i);
                setVec3(shaderProgram, buf, torchPositions[i]);
                sprintf(buf, "pointLightColor[%d]", i);
                setVec3(shaderProgram, buf, glm::vec3(1.0f, 0.6f, 0.2f));
            }
        };

        // Helper lambda: render scene with given view/proj into a viewport rect
        auto renderViewport = [&](int vx, int vy, int vw, int vh,
                                   glm::mat4 vMat, glm::mat4 pMat, glm::vec3 eye) {
            glViewport(vx, vy, vw, vh);
            glEnable(GL_SCISSOR_TEST);
            glScissor(vx, vy, vw, vh);
            glClear(GL_DEPTH_BUFFER_BIT);
            setMat4(shaderProgram, "view", vMat);
            setMat4(shaderProgram, "projection", pMat);
            setVec3(shaderProgram, "viewPos", eye);
            currentVP = pMat * vMat;
            renderTerrain(curTime);
            renderObjects(curTime);
            drawBlockHighlight();
            uploadPointLights();
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
                p = glm::perspective(glm::radians(50.0f), aspect, 0.1f, 200.0f);
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
                proj = glm::perspective(glm::radians(50.0f), aspect, 0.1f, 200.0f);
            }

            setMat4(shaderProgram, "view", view);
            setMat4(shaderProgram, "projection", proj);
            setVec3(shaderProgram, "viewPos", eye);
            currentVP = proj * view;
            renderTerrain(curTime);
            renderObjects(curTime);
            drawBlockHighlight();
            uploadPointLights();
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
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

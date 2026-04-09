#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>

// =====================================================
// Constants
// =====================================================
const int WIN_W = 1280, WIN_H = 720;
const float HEX_RADIUS = 0.5f;
const float HEX_HEIGHT = 1.0f;
const float PI = 3.14159265358979f;

// Block grid dimensions
const int GRID_W = 200;   // col range: -100 to 99
const int GRID_D = 200;   // row range: -100 to 99
const int GRID_H = 32;    // height layers 0..31
const int GRID_OFF_X = 100; // offset so col=-100 maps to index 0
const int GRID_OFF_Z = 100;

// Block types
enum BlockType {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_SAND,
    BLOCK_STONE,
    BLOCK_WATER,
    BLOCK_WOOD,
    BLOCK_LEAF,
    BLOCK_ORE_DIAMOND,
    BLOCK_ORE_GOLD,
    BLOCK_GLASS,
    BLOCK_STONE_LIGHT,
    BLOCK_COUNT
};

// 3D voxel grid
int blockGrid[GRID_W][GRID_D][GRID_H];

// Hotbar
int hotbarSlot = 0; // 0-8
const BlockType hotbarBlocks[] = {
    BLOCK_GRASS, BLOCK_DIRT, BLOCK_SAND, BLOCK_STONE, BLOCK_WOOD,
    BLOCK_LEAF, BLOCK_ORE_DIAMOND, BLOCK_ORE_GOLD, BLOCK_GLASS
};
const int HOTBAR_SIZE = 9;

// Block targeting (raycasting)
bool hasTarget = false;
int targetCol, targetRow, targetHeight;
int placeCol, placeRow, placeHeight; // adjacent cell for placement

// =====================================================
// Camera state
// =====================================================
glm::vec3 camPos(4.0f, 10.0f, 16.0f);
glm::vec3 camFront(0.0f, -0.4f, -1.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);
float camSpeed = 5.0f;
float camPitch = -25.0f, camYaw = -90.0f, camRoll = 0.0f;
float mouseSensitivity = 0.1f;
bool firstMouse = true;
double lastMouseX = 640.0, lastMouseY = 360.0;
bool mouseCaptured = false; // right-click to toggle

// =====================================================
// Player state — actual game character
// =====================================================
glm::vec3 playerWorldPos(0.0f, 5.0f, 0.0f); // world position — will snap to ground on first frame
float playerYaw = 0.0f;       // horizontal facing direction (radians)
float playerVelY = 0.0f;      // vertical velocity (for jump/gravity)
bool playerOnGround = true;
bool playerWalking = false;
float playerWalkTime = 0.0f;  // for walk animation cycle

// Camera mode: 0=third-person, 1=first-person, 2=free-fly
int cameraMode = 0;
float thirdPersonDist = 4.0f;  // distance behind player
float thirdPersonHeight = 1.5f; // height above player

// Timing
float deltaTime = 0.0f, lastFrame = 0.0f;

// View modes
bool fourViewport = false;    // V toggle
bool birdsEye = false;        // B toggle
float orbitAngle = 0.0f;      // F key orbit around look-at point
glm::vec3 lookAtTarget(3.0f, 1.0f, 3.0f); // center of terrain

// Lighting state
bool lightOn = true;
bool dirLightOn = true;
bool pointLightOn = true;
bool spotLightOn = true;
bool ambientOn = true;
bool diffuseOn = true;
bool specularOn = true;

// Day-night: 0=night, 1=dawn, 2=noon, 3=dusk
int dayMode = 2;
float dayFactor = 1.0f;

// Interactive objects
bool fanOn = false;          // G toggle
float fanAngle = 0.0f;
bool doorOpen = false;       // O toggle
float doorAngle = 0.0f;     // animates 0 to 90

// MineCar state (arrow keys to drive)
glm::vec3 carPos(4.0f, 0.0f, -4.0f); // Zone C (sand, south)
float carYaw = 0.0f;        // heading in radians
float carSpeed = 0.0f;
float wheelSpin = 0.0f;

// Shader
GLuint shaderProgram;

// Hex mesh
GLuint hexVAO, hexVBO;
int hexVertexCount = 0;

// =====================================================
// Custom myRotate (Rodrigues' rotation formula)
// Replaces glm::rotate
// =====================================================
glm::mat4 myRotate(glm::mat4 m, float angleRad, glm::vec3 axis) {
    axis = glm::normalize(axis);
    float c = cosf(angleRad);
    float s = sinf(angleRad);
    float t = 1.0f - c;
    float x = axis.x, y = axis.y, z = axis.z;

    glm::mat4 rot(1.0f);
    rot[0][0] = t * x * x + c;
    rot[0][1] = t * x * y + s * z;
    rot[0][2] = t * x * z - s * y;

    rot[1][0] = t * x * y - s * z;
    rot[1][1] = t * y * y + c;
    rot[1][2] = t * y * z + s * x;

    rot[2][0] = t * x * z + s * y;
    rot[2][1] = t * y * z - s * x;
    rot[2][2] = t * z * z + c;

    return m * rot;
}

// =====================================================
// Shader utilities
// =====================================================
std::string loadFile(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) { printf("Cannot open: %s\n", path); return ""; }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        printf("Shader error: %s\n", log);
    }
    return s;
}

GLuint loadShaders(const char* vp, const char* fp) {
    std::string vs = loadFile(vp), fs = loadFile(fp);
    GLuint v = compileShader(GL_VERTEX_SHADER, vs.c_str());
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs.c_str());
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    int ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, 512, NULL, log);
        printf("Link error: %s\n", log);
    }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

void setMat4(GLuint prog, const char* name, const glm::mat4& m) {
    glUniformMatrix4fv(glGetUniformLocation(prog, name), 1, GL_FALSE, glm::value_ptr(m));
}
void setVec3(GLuint prog, const char* name, const glm::vec3& v) {
    glUniform3fv(glGetUniformLocation(prog, name), 1, glm::value_ptr(v));
}
void setBool(GLuint prog, const char* name, bool val) {
    glUniform1i(glGetUniformLocation(prog, name), val ? 1 : 0);
}
void setFloat(GLuint prog, const char* name, float val) {
    glUniform1f(glGetUniformLocation(prog, name), val);
}
void setInt(GLuint prog, const char* name, int val) {
    glUniform1i(glGetUniformLocation(prog, name), val);
}

// =====================================================
// Hexagonal Prism Mesh Generation
// 8 faces: 6 rectangular sides + 2 hexagonal caps
// Each vertex: pos(3) + normal(3) + color(3) = 9 floats
// =====================================================
struct Vertex {
    glm::vec3 pos, normal, color;
};

std::vector<Vertex> generateHexPrism() {
    std::vector<Vertex> verts;
    float r = HEX_RADIUS;
    float h = HEX_HEIGHT;
    float halfH = h / 2.0f;

    // Hex corner positions (XZ plane)
    glm::vec3 top[6], bot[6];
    for (int i = 0; i < 6; i++) {
        float angle = PI / 3.0f * i + PI / 6.0f; // offset 30deg for flat-top hex
        float x = r * cosf(angle);
        float z = r * sinf(angle);
        top[i] = glm::vec3(x, halfH, z);
        bot[i] = glm::vec3(x, -halfH, z);
    }

    glm::vec3 white(1.0f); // default color, overridden per-draw

    // --- Top cap (6 triangles, fan from center) ---
    glm::vec3 topCenter(0, halfH, 0);
    glm::vec3 topNorm(0, 1, 0);
    for (int i = 0; i < 6; i++) {
        int next = (i + 1) % 6;
        verts.push_back({topCenter, topNorm, white});
        verts.push_back({top[i],    topNorm, white});
        verts.push_back({top[next], topNorm, white});
    }

    // --- Bottom cap (6 triangles, fan from center) ---
    glm::vec3 botCenter(0, -halfH, 0);
    glm::vec3 botNorm(0, -1, 0);
    for (int i = 0; i < 6; i++) {
        int next = (i + 1) % 6;
        verts.push_back({botCenter, botNorm, white});
        verts.push_back({bot[next], botNorm, white});
        verts.push_back({bot[i],    botNorm, white});
    }

    // --- 6 rectangular side faces (2 triangles each) ---
    for (int i = 0; i < 6; i++) {
        int next = (i + 1) % 6;
        // Outward normal for this side
        glm::vec3 edge = top[next] - top[i];
        glm::vec3 up(0, 1, 0);
        glm::vec3 sideNorm = glm::normalize(glm::cross(edge, up));

        // Two triangles: top[i], bot[i], bot[next] and top[i], bot[next], top[next]
        verts.push_back({top[i],    sideNorm, white});
        verts.push_back({bot[i],    sideNorm, white});
        verts.push_back({bot[next], sideNorm, white});

        verts.push_back({top[i],    sideNorm, white});
        verts.push_back({bot[next], sideNorm, white});
        verts.push_back({top[next], sideNorm, white});
    }

    return verts;
}

void initHexMesh() {
    std::vector<Vertex> verts = generateHexPrism();
    hexVertexCount = (int)verts.size();

    glGenVertexArrays(1, &hexVAO);
    glGenBuffers(1, &hexVBO);
    glBindVertexArray(hexVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hexVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    // color
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);
}

// Draw a hex prism at position with color and scale (uses uniform color — fast)
void drawHex(glm::vec3 pos, glm::vec3 color, glm::vec3 scale = glm::vec3(1.0f)) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, scale);
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(hexVAO);
    glDrawArrays(GL_TRIANGLES, 0, hexVertexCount);
}

// Hex with custom rotation (using myRotate)
void drawHexRotated(glm::vec3 pos, glm::vec3 color, float angle, glm::vec3 axis, glm::vec3 scale = glm::vec3(1.0f)) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = myRotate(model, angle, axis);
    model = glm::scale(model, scale);
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(hexVAO);
    glDrawArrays(GL_TRIANGLES, 0, hexVertexCount);
}

// Draw hex with full custom model matrix
void drawHexModel(glm::mat4 model, glm::vec3 color) {
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(hexVAO);
    glDrawArrays(GL_TRIANGLES, 0, hexVertexCount);
}

// =====================================================
// Curvy Objects: Sphere, Cone, Bezier, Spline, Ruled Surface
// Each generates a VAO on init and has a draw function.
// =====================================================
GLuint sphereVAO, sphereVBO;
int sphereVertCount = 0;
GLuint coneVAO, coneVBO;
int coneVertCount = 0;
GLuint bezierVAO, bezierVBO;
int bezierVertCount = 0;
GLuint splineVAO, splineVBO;
int splineVertCount = 0;
GLuint ruledVAO, ruledVBO;
int ruledVertCount = 0;

// --- Sphere (UV tessellation) ---
void initSphere(int stacks = 16, int slices = 24, float radius = 1.0f) {
    std::vector<Vertex> verts;
    for (int i = 0; i < stacks; i++) {
        float phi1 = PI * i / stacks;
        float phi2 = PI * (i + 1) / stacks;
        for (int j = 0; j < slices; j++) {
            float th1 = 2.0f * PI * j / slices;
            float th2 = 2.0f * PI * (j + 1) / slices;

            glm::vec3 p00(radius * sinf(phi1) * cosf(th1), radius * cosf(phi1), radius * sinf(phi1) * sinf(th1));
            glm::vec3 p10(radius * sinf(phi1) * cosf(th2), radius * cosf(phi1), radius * sinf(phi1) * sinf(th2));
            glm::vec3 p01(radius * sinf(phi2) * cosf(th1), radius * cosf(phi2), radius * sinf(phi2) * sinf(th1));
            glm::vec3 p11(radius * sinf(phi2) * cosf(th2), radius * cosf(phi2), radius * sinf(phi2) * sinf(th2));

            glm::vec3 n00 = glm::normalize(p00), n10 = glm::normalize(p10);
            glm::vec3 n01 = glm::normalize(p01), n11 = glm::normalize(p11);
            glm::vec3 w(1.0f);

            verts.push_back({p00, n00, w}); verts.push_back({p01, n01, w}); verts.push_back({p10, n10, w});
            verts.push_back({p10, n10, w}); verts.push_back({p01, n01, w}); verts.push_back({p11, n11, w});
        }
    }
    sphereVertCount = (int)verts.size();
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);
}

void drawSphere(glm::vec3 pos, float radius, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(radius));
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(sphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, sphereVertCount);
}

// --- Cone (tessellated) ---
void initCone(int slices = 24, float radius = 1.0f, float height = 2.0f) {
    std::vector<Vertex> verts;
    glm::vec3 tip(0, height, 0);
    glm::vec3 w(1.0f);

    for (int j = 0; j < slices; j++) {
        float th1 = 2.0f * PI * j / slices;
        float th2 = 2.0f * PI * (j + 1) / slices;
        glm::vec3 b1(radius * cosf(th1), 0, radius * sinf(th1));
        glm::vec3 b2(radius * cosf(th2), 0, radius * sinf(th2));

        // Side face normal
        glm::vec3 edge1 = tip - b1, edge2 = b2 - b1;
        glm::vec3 n = glm::normalize(glm::cross(edge2, edge1));

        verts.push_back({tip, n, w}); verts.push_back({b1, n, w}); verts.push_back({b2, n, w});

        // Bottom cap
        glm::vec3 dn(0, -1, 0);
        verts.push_back({glm::vec3(0,0,0), dn, w}); verts.push_back({b2, dn, w}); verts.push_back({b1, dn, w});
    }
    coneVertCount = (int)verts.size();
    glGenVertexArrays(1, &coneVAO);
    glGenBuffers(1, &coneVBO);
    glBindVertexArray(coneVAO);
    glBindBuffer(GL_ARRAY_BUFFER, coneVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);
}

void drawCone(glm::vec3 pos, float scale, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(scale));
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(coneVAO);
    glDrawArrays(GL_TRIANGLES, 0, coneVertCount);
}

// --- Bezier Curve (cubic, rendered as tube) ---
glm::vec3 bezierPoint(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    float u = 1.0f - t;
    return u*u*u*p0 + 3.0f*u*u*t*p1 + 3.0f*u*t*t*p2 + t*t*t*p3;
}

void initBezier() {
    // Control points for a decorative arch/bridge shape
    glm::vec3 cp0(0, 0, 0), cp1(1, 3, 0), cp2(3, 3, 0), cp3(4, 0, 0);
    std::vector<Vertex> verts;
    glm::vec3 w(1.0f);
    int segments = 30;
    float tubeR = 0.08f;
    int tubeSlices = 8;

    for (int i = 0; i < segments; i++) {
        float t1 = (float)i / segments;
        float t2 = (float)(i + 1) / segments;
        glm::vec3 c1 = bezierPoint(cp0, cp1, cp2, cp3, t1);
        glm::vec3 c2 = bezierPoint(cp0, cp1, cp2, cp3, t2);
        glm::vec3 dir = glm::normalize(c2 - c1);
        glm::vec3 up(0, 1, 0);
        if (fabsf(glm::dot(dir, up)) > 0.99f) up = glm::vec3(1, 0, 0);
        glm::vec3 right = glm::normalize(glm::cross(dir, up));
        glm::vec3 nup = glm::normalize(glm::cross(right, dir));

        for (int j = 0; j < tubeSlices; j++) {
            float a1 = 2.0f * PI * j / tubeSlices;
            float a2 = 2.0f * PI * (j + 1) / tubeSlices;
            glm::vec3 n1 = cosf(a1) * right + sinf(a1) * nup;
            glm::vec3 n2 = cosf(a2) * right + sinf(a2) * nup;
            glm::vec3 p1a = c1 + tubeR * n1, p1b = c1 + tubeR * n2;
            glm::vec3 p2a = c2 + tubeR * n1, p2b = c2 + tubeR * n2;
            verts.push_back({p1a, n1, w}); verts.push_back({p2a, n1, w}); verts.push_back({p1b, n2, w});
            verts.push_back({p1b, n2, w}); verts.push_back({p2a, n1, w}); verts.push_back({p2b, n2, w});
        }
    }
    bezierVertCount = (int)verts.size();
    glGenVertexArrays(1, &bezierVAO);
    glGenBuffers(1, &bezierVBO);
    glBindVertexArray(bezierVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bezierVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);
}

void drawBezier(glm::vec3 pos, float scale, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(scale));
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(bezierVAO);
    glDrawArrays(GL_TRIANGLES, 0, bezierVertCount);
}

// --- Catmull-Rom Spline Curve (fence/path) ---
glm::vec3 catmullRom(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    float t2 = t * t, t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
                    + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

void initSpline() {
    glm::vec3 pts[] = {
        glm::vec3(0, 0, 0), glm::vec3(1, 0.5f, 1), glm::vec3(2, 0, 2.5f),
        glm::vec3(3.5f, 0.8f, 2), glm::vec3(5, 0, 0.5f), glm::vec3(6, 0.3f, -0.5f)
    };
    int nPts = 6;
    std::vector<Vertex> verts;
    glm::vec3 w(1.0f);
    float tubeR = 0.06f;
    int tubeSlices = 6;

    for (int seg = 0; seg < nPts - 3; seg++) {
        for (int i = 0; i < 20; i++) {
            float t1 = (float)i / 20.0f;
            float t2 = (float)(i + 1) / 20.0f;
            glm::vec3 c1 = catmullRom(pts[seg], pts[seg+1], pts[seg+2], pts[seg+3], t1);
            glm::vec3 c2 = catmullRom(pts[seg], pts[seg+1], pts[seg+2], pts[seg+3], t2);
            glm::vec3 dir = glm::normalize(c2 - c1);
            glm::vec3 up(0, 1, 0);
            if (fabsf(glm::dot(dir, up)) > 0.99f) up = glm::vec3(1, 0, 0);
            glm::vec3 right = glm::normalize(glm::cross(dir, up));
            glm::vec3 nup = glm::normalize(glm::cross(right, dir));
            for (int j = 0; j < tubeSlices; j++) {
                float a1 = 2.0f * PI * j / tubeSlices;
                float a2 = 2.0f * PI * (j + 1) / tubeSlices;
                glm::vec3 n1 = cosf(a1) * right + sinf(a1) * nup;
                glm::vec3 n2 = cosf(a2) * right + sinf(a2) * nup;
                glm::vec3 p1a = c1 + tubeR * n1, p1b = c1 + tubeR * n2;
                glm::vec3 p2a = c2 + tubeR * n1, p2b = c2 + tubeR * n2;
                verts.push_back({p1a, n1, w}); verts.push_back({p2a, n1, w}); verts.push_back({p1b, n2, w});
                verts.push_back({p1b, n2, w}); verts.push_back({p2a, n1, w}); verts.push_back({p2b, n2, w});
            }
        }
    }
    splineVertCount = (int)verts.size();
    glGenVertexArrays(1, &splineVAO);
    glGenBuffers(1, &splineVBO);
    glBindVertexArray(splineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, splineVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);
}

void drawSpline(glm::vec3 pos, float scale, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(scale));
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(splineVAO);
    glDrawArrays(GL_TRIANGLES, 0, splineVertCount);
}

// --- Ruled Surface (a curved ramp between two curves) ---
void initRuledSurface() {
    std::vector<Vertex> verts;
    glm::vec3 w(1.0f);
    int steps = 30;

    for (int i = 0; i < steps; i++) {
        float t1 = (float)i / steps;
        float t2 = (float)(i + 1) / steps;

        // Curve 1 (bottom edge — straight)
        glm::vec3 a1(t1 * 4.0f, 0, 0);
        glm::vec3 a2(t2 * 4.0f, 0, 0);
        // Curve 2 (top edge — sinusoidal)
        glm::vec3 b1(t1 * 4.0f, 1.5f + 0.5f * sinf(t1 * PI * 2.0f), 2.0f);
        glm::vec3 b2(t2 * 4.0f, 1.5f + 0.5f * sinf(t2 * PI * 2.0f), 2.0f);

        glm::vec3 n = glm::normalize(glm::cross(a2 - a1, b1 - a1));

        verts.push_back({a1, n, w}); verts.push_back({b1, n, w}); verts.push_back({a2, n, w});
        verts.push_back({a2, n, w}); verts.push_back({b1, n, w}); verts.push_back({b2, n, w});
    }
    ruledVertCount = (int)verts.size();
    glGenVertexArrays(1, &ruledVAO);
    glGenBuffers(1, &ruledVBO);
    glBindVertexArray(ruledVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ruledVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);
}

void drawRuledSurface(glm::vec3 pos, float scale, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(scale));
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(ruledVAO);
    glDrawArrays(GL_TRIANGLES, 0, ruledVertCount);
}

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
    int n = x * 374761393 + y * 668265263;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
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
    unsigned int s = (unsigned int)(col * 73856093) ^ (unsigned int)(row * 19349663);
    s = (s ^ (s >> 16)) * 0x45d9f3b;
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
    blockGrid[col + GRID_OFF_X][row + GRID_OFF_Z][h] = type;
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
    float nx = col * 0.12f;
    float ny = row * 0.12f;
    float n = fbmNoise(nx, ny, 4);

    switch (biome) {
        case 0: return 0; // sand is flat
        case 1: { // grass: 1-3 steps
            int h = (int)floorf((n + 1.0f) * 1.5f);
            if (h < 1) h = 1;
            if (h > 3) h = 3;
            return h;
        }
        case 2: { // stone: 0-2
            int h = (int)floorf((n + 1.0f) * 1.2f);
            if (h < 0) h = 0;
            if (h > 2) h = 2;
            return h;
        }
        case 3: return -1; // water is below ground
    }
    return 0;
}

// =====================================================
// Trees — BIG, colorful, with thick trunk and large canopy
// =====================================================
void drawTree(glm::vec3 base, glm::vec3 leafColor) {
    // Thick trunk: 2 hex wide, 5 tall
    int trunkH = 5;
    for (int i = 0; i < trunkH; i++) {
        glm::vec3 p = base + glm::vec3(0, i * HEX_HEIGHT, 0);
        float v = 0.9f + 0.1f * hashNoise((int)(base.x * 10), i);
        // Main trunk (thick)
        drawHex(p, COL_WOOD_DARK * v, glm::vec3(0.8f, 1.0f, 0.8f));
        // Trunk buttress at base
        if (i == 0) {
            drawHex(p + glm::vec3(0.3f, 0, 0), COL_WOOD_DARK * v, glm::vec3(0.4f, 0.6f, 0.4f));
            drawHex(p + glm::vec3(-0.3f, 0, 0.2f), COL_WOOD_DARK * v, glm::vec3(0.4f, 0.6f, 0.4f));
        }
    }

    // Large canopy: 4 layers, radius up to 3
    float topY = base.y + trunkH * HEX_HEIGHT;
    unsigned int seed = gridSeed((int)(base.x * 10), (int)(base.z * 10));
    glm::vec3 leafDark = leafColor * 0.7f;

    for (int dy = 0; dy < 4; dy++) {
        int radius;
        if (dy == 0) radius = 2;
        else if (dy == 1 || dy == 2) radius = 3;
        else radius = 2;

        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                // Circular shape
                if (dx * dx + dz * dz > radius * radius + 1) continue;

                seed = seed * 1103515245 + 12345;
                if ((seed >> 16) % 10 < 1) continue; // 10% holes

                float ox = dx * HEX_RADIUS * 1.6f;
                float oz = dz * HEX_RADIUS * 1.6f;
                glm::vec3 lp(base.x + ox, topY + dy * HEX_HEIGHT * 0.7f, base.z + oz);

                glm::vec3 lc = ((seed >> 8) % 3 == 0) ? leafDark : leafColor;
                drawHex(lp, lc, glm::vec3(0.65f, 0.65f, 0.65f));
            }
        }
    }
}

// Standalone large tree (green, even bigger)
void drawLargeTree(glm::vec3 base) {
    int trunkH = 6;
    for (int i = 0; i < trunkH; i++) {
        glm::vec3 p = base + glm::vec3(0, i * HEX_HEIGHT, 0);
        drawHex(p, COL_WOOD_DARK, glm::vec3(1.0f, 1.0f, 1.0f));
        if (i < 2) {
            drawHex(p + glm::vec3(0.4f, 0, 0.2f), COL_WOOD_DARK, glm::vec3(0.5f, 0.7f, 0.5f));
            drawHex(p + glm::vec3(-0.3f, 0, -0.3f), COL_WOOD_DARK, glm::vec3(0.5f, 0.7f, 0.5f));
        }
    }
    float topY = base.y + trunkH * HEX_HEIGHT;
    unsigned int seed = 12345;
    glm::vec3 green(0.15f, 0.55f, 0.12f);
    glm::vec3 greenD(0.1f, 0.4f, 0.08f);
    for (int dy = 0; dy < 5; dy++) {
        int radius = (dy == 0 || dy == 4) ? 2 : (dy == 2 ? 4 : 3);
        for (int dx = -radius; dx <= radius; dx++) {
            for (int dz = -radius; dz <= radius; dz++) {
                if (dx * dx + dz * dz > radius * radius + 1) continue;
                seed = seed * 1103515245 + 12345;
                if ((seed >> 16) % 10 < 1) continue;
                float ox = dx * HEX_RADIUS * 1.5f;
                float oz = dz * HEX_RADIUS * 1.5f;
                glm::vec3 lc = ((seed >> 8) % 3 == 0) ? greenD : green;
                drawHex(glm::vec3(base.x + ox, topY + dy * HEX_HEIGHT * 0.65f, base.z + oz),
                        lc, glm::vec3(0.6f, 0.6f, 0.6f));
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

    // 4 pillars
    float px[] = {-2.0f, 2.0f, -2.0f, 2.0f};
    float pz[] = {-1.5f, -1.5f, 1.5f, 1.5f};
    int pillarH[] = {6, 6, 5, 4}; // some broken
    for (int p = 0; p < 4; p++) {
        for (int h = 0; h < pillarH[p]; h++) {
            glm::vec3 pos = base + glm::vec3(px[p], h * HEX_HEIGHT, pz[p]);
            drawHex(pos, (h % 2 == 0) ? sc : sd, glm::vec3(0.6f, 1.0f, 0.6f));
        }
    }

    // Top beams connecting pillars
    for (float x = -2.0f; x <= 2.0f; x += 0.8f) {
        drawHex(base + glm::vec3(x, 6.0f * HEX_HEIGHT, -1.5f), sc, glm::vec3(0.5f, 0.4f, 0.5f));
        if (x < 1.5f) // partial on back side
            drawHex(base + glm::vec3(x, 5.0f * HEX_HEIGHT, 1.5f), sd, glm::vec3(0.5f, 0.4f, 0.5f));
    }

    // Archway (front face, gap in middle)
    for (float x = -2.0f; x <= 2.0f; x += 0.8f) {
        if (fabsf(x) < 1.0f) continue; // gap for arch
        for (int h = 0; h < 3; h++) {
            drawHex(base + glm::vec3(x, h * HEX_HEIGHT, -1.5f), sd);
        }
    }
    // Arch top
    drawHex(base + glm::vec3(0, 3.0f * HEX_HEIGHT, -1.5f), sc, glm::vec3(1.2f, 0.4f, 0.5f));

    // Side walls (partial, broken)
    for (int h = 0; h < 3; h++) {
        for (float z = -1.5f; z <= 1.5f; z += 0.8f) {
            unsigned int ws = gridSeed((int)(z * 10), h);
            if (ws % 5 == 0) continue; // missing blocks for ruined look
            drawHex(base + glm::vec3(-2.0f, h * HEX_HEIGHT, z), sd);
        }
    }

    // Scattered rubble
    drawHex(base + glm::vec3(3.0f, 0, 0), sd, glm::vec3(0.5f, 0.5f, 0.5f));
    drawHex(base + glm::vec3(-3.0f, 0, 1.0f), sc, glm::vec3(0.4f, 0.4f, 0.4f));
    drawHex(base + glm::vec3(1.5f, 0, 2.5f), sd, glm::vec3(0.6f, 0.3f, 0.6f));
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
    return (col >= -5 && col <= 10 && row >= 18 && row <= 28);
}

// Check if position is a water pond center
bool isWaterPond(int col, int row) {
    // Pond 1 around (8, 12)
    int d1 = (col - 8) * (col - 8) + (row - 12) * (row - 12);
    if (d1 <= 6) return true;
    // Pond 2 around (-8, 8)
    int d2 = (col + 8) * (col + 8) + (row - 8) * (row - 8);
    if (d2 <= 4) return true;
    // Pond 3 around (15, 20) inside forest
    int d3 = (col - 15) * (col - 15) + (row - 20) * (row - 20);
    if (d3 <= 3) return true;
    return false;
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

            // Water ponds
            if (isWaterPond(col, row)) {
                setBlock(col, row, 0, (seed % 3 == 0) ? BLOCK_WATER : BLOCK_WATER);
                continue;
            }

            int biome = getBiome(col, row);
            int height = getTerrainHeightBiome(col, row, biome);

            // Fill blocks from bottom to top
            for (int h = 0; h <= height + 1; h++) {
                int blockType;
                if (h == height + 1) {
                    // Top surface block
                    switch (biome) {
                        case 0: blockType = BLOCK_SAND; break;
                        case 1: blockType = BLOCK_GRASS; break;
                        case 2: blockType = (seed % 2) ? BLOCK_STONE : BLOCK_STONE_LIGHT; break;
                        default: blockType = BLOCK_SAND; break;
                    }
                } else if (h == height && biome == 1) {
                    blockType = BLOCK_DIRT;
                } else {
                    if ((seed + h) % 47 == 0)
                        blockType = BLOCK_ORE_DIAMOND;
                    else if ((seed + h) % 37 == 0)
                        blockType = BLOCK_ORE_GOLD;
                    else
                        blockType = BLOCK_STONE;
                }
                setBlock(col, row, h, blockType);
            }

            // Record tree locations (will still draw as multi-hex objects)
            if (isForestZone(col, row) && biome == 1 && height >= 1 && (seed % 7 == 0)) {
                TreeInfo ti;
                ti.col = col; ti.row = row;
                ti.leafColor = TREE_COLORS[seed % NUM_TREE_COLORS];
                ti.large = false;
                treeLocations.push_back(ti);
            }

            // Record torch locations
            if (biome == 1 && height >= 1 && (seed % 61 == 0)) {
                TorchInfo t; t.col = col; t.row = row; t.height = height + 2;
                torchLocations.push_back(t);
            }
        }
    }

    // Standalone large tree
    {
        TreeInfo ti; ti.col = 5; ti.row = 10; ti.leafColor = glm::vec3(0); ti.large = true;
        treeLocations.push_back(ti);
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
        const int C0 = -5, C1 = 40;    // col range (46 wide)
        const int R0 = -20, R1 = 25;   // row range (46 deep)
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
            // Path outside
            for (int r = R0 - 3; r <= R0 - 1; r++)
                for (int c = dm - 3; c <= dm + 3; c++)
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

        printf("[Castle] Built at col %d..%d, row %d..%d (%dx%d), walls=%d, towers=%d\n",
               C0, C1, R0, R1, C1 - C0 + 1, R1 - R0 + 1, wallH, twrH);
    }
}

// =====================================================
// Terrain rendering — reads from blockGrid
// =====================================================
void renderTerrain() {
    torchPositions.clear();

    for (int row = TERRAIN_MIN; row <= TERRAIN_MAX; row++) {
        for (int col = TERRAIN_MIN; col <= TERRAIN_MAX; col++) {
            glm::vec3 pos = hexGridPos(col, row, 0.0f);

            for (int h = 0; h < GRID_H; h++) {
                int bt = getBlock(col, row, h);
                if (bt == BLOCK_AIR) continue;

                glm::vec3 p = pos + glm::vec3(0, h * HEX_HEIGHT, 0);

                if (bt == BLOCK_ORE_DIAMOND || bt == BLOCK_ORE_GOLD) {
                    drawHexEmissive(p, getBlockColor(bt));
                } else {
                    drawHex(p, getBlockColor(bt));
                }
            }
        }
    }

    // Draw trees (non-grid decorative objects)
    for (auto& ti : treeLocations) {
        glm::vec3 pos = hexGridPos(ti.col, ti.row, 0.0f);
        // Find top block
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

    // Draw torches
    for (auto& t : torchLocations) {
        glm::vec3 pos = hexGridPos(t.col, t.row, 0.0f);
        int topH = 0;
        for (int h = GRID_H - 1; h >= 0; h--) {
            if (getBlock(t.col, t.row, h) != BLOCK_AIR) { topH = h; break; }
        }
        drawTorch(pos + glm::vec3(0, (topH + 1) * HEX_HEIGHT, 0));
    }

    // Stone Ruins (Zone D)
    {
        int rc = 20, rr = 10;
        glm::vec3 rp = hexGridPos(rc, rr, 0.0f);
        int topH = 0;
        for (int h = GRID_H - 1; h >= 0; h--) {
            if (getBlock(rc, rr, h) != BLOCK_AIR) { topH = h; break; }
        }
        drawRuins(rp + glm::vec3(0, (topH + 1) * HEX_HEIGHT, 0));
    }
}

// =====================================================
// Player Character (hierarchical hexagonal model)
// Body parts have individual transforms relative to parent
// =====================================================
void drawPlayer(glm::vec3 pos, float time, float yaw = 0.0f, bool walking = false, float walkT = 0.0f) {
    float walkCycle = walking ? sinf(walkT * 3.0f) : 0.0f; // no movement when idle

    // Base transform: translate to pos, then rotate to face yaw
    auto makePartModel = [&](glm::vec3 offset, glm::vec3 scale) -> glm::mat4 {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, offset);
        m = glm::scale(m, scale);
        return m;
    };

    glm::vec3 bodyColor(0.55f, 0.45f, 0.3f);
    glm::vec3 skinColor(0.8f, 0.65f, 0.5f);

    // Body (torso)
    drawHexModel(makePartModel(glm::vec3(0, 1.0f, 0), glm::vec3(0.7f, 0.9f, 0.5f)), bodyColor);

    // Head
    drawHexModel(makePartModel(glm::vec3(0, 1.85f, 0), glm::vec3(0.5f, 0.5f, 0.5f)), skinColor);

    // Helmet
    drawHexModel(makePartModel(glm::vec3(0, 2.1f, 0), glm::vec3(0.55f, 0.2f, 0.55f)),
                 glm::vec3(0.2f, 0.5f, 0.7f));

    // Left arm (swings with walk)
    float armSwing = walkCycle * 0.35f;
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(-0.55f, 1.1f, 0));
        m = myRotate(m, armSwing, glm::vec3(1, 0, 0));
        m = glm::scale(m, glm::vec3(0.25f, 0.7f, 0.25f));
        drawHexModel(m, skinColor);
    }
    // Right arm
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(0.55f, 1.1f, 0));
        m = myRotate(m, -armSwing, glm::vec3(1, 0, 0));
        m = glm::scale(m, glm::vec3(0.25f, 0.7f, 0.25f));
        drawHexModel(m, skinColor);
    }

    // Left leg
    float legSwing = walkCycle * 0.3f;
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(-0.2f, 0.25f, 0));
        m = myRotate(m, legSwing, glm::vec3(1, 0, 0));
        m = glm::scale(m, glm::vec3(0.25f, 0.55f, 0.25f));
        drawHexModel(m, glm::vec3(0.35f, 0.25f, 0.15f));
    }
    // Right leg
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(0.2f, 0.25f, 0));
        m = myRotate(m, -legSwing, glm::vec3(1, 0, 0));
        m = glm::scale(m, glm::vec3(0.25f, 0.55f, 0.25f));
        drawHexModel(m, glm::vec3(0.35f, 0.25f, 0.15f));
    }
}

// =====================================================
// Rotating Fan (G toggle)
// Hierarchical: base + pole + 4 blades rotating around pole
// =====================================================
void drawFan(glm::vec3 pos) {
    // Base
    drawHex(pos, glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.8f, 0.2f, 0.8f));
    // Pole
    drawHex(pos + glm::vec3(0, 1.0f, 0), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.15f, 1.5f, 0.15f));
    // Hub
    drawHex(pos + glm::vec3(0, 2.0f, 0), glm::vec3(0.6f, 0.6f, 0.6f), glm::vec3(0.25f, 0.15f, 0.25f));

    // 4 blades
    glm::vec3 bladeColor(0.7f, 0.7f, 0.75f);
    for (int i = 0; i < 4; i++) {
        float bladeAngle = fanAngle + i * (PI / 2.0f);
        float bx = cosf(bladeAngle) * 0.8f;
        float bz = sinf(bladeAngle) * 0.8f;
        glm::vec3 bladePos = pos + glm::vec3(bx, 2.0f, bz);
        drawHexRotated(bladePos, bladeColor, bladeAngle, glm::vec3(0, 1, 0),
                       glm::vec3(0.6f, 0.08f, 0.2f));
    }
}

// =====================================================
// Door (O toggle, animates open/close)
// Pivot on left edge using myRotate
// =====================================================
void drawDoor(glm::vec3 pos) {
    glm::vec3 frameColor(0.5f, 0.35f, 0.2f);
    glm::vec3 doorColor(0.4f, 0.25f, 0.12f);

    // Door frame (left post, right post, top beam)
    drawHex(pos + glm::vec3(-0.6f, 0.75f, 0), frameColor, glm::vec3(0.12f, 1.5f, 0.15f));
    drawHex(pos + glm::vec3(0.6f, 0.75f, 0), frameColor, glm::vec3(0.12f, 1.5f, 0.15f));
    drawHex(pos + glm::vec3(0, 1.55f, 0), frameColor, glm::vec3(1.3f, 0.12f, 0.15f));

    // Door panel (pivots around left edge)
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos + glm::vec3(-0.5f, 0.75f, 0));
    model = myRotate(model, glm::radians(doorAngle), glm::vec3(0, 1, 0));
    model = glm::translate(model, glm::vec3(0.5f, 0, 0));
    model = glm::scale(model, glm::vec3(0.9f, 1.4f, 0.08f));
    drawHexModel(model, doorColor);
}

// =====================================================
// Clock (analog face with hour/minute hands)
// Uses myRotate for hand rotations
// =====================================================
void drawClock(glm::vec3 pos, float time) {
    // Clock is VERTICAL (face in XY plane, facing +Z)
    // Back plate (dark wood frame)
    drawHexRotated(pos, glm::vec3(0.35f, 0.22f, 0.12f),
                   PI / 2.0f, glm::vec3(1, 0, 0), glm::vec3(1.8f, 0.1f, 1.8f));

    // Clock face (white, slightly in front)
    drawHexRotated(pos + glm::vec3(0, 0, 0.06f), glm::vec3(0.9f, 0.9f, 0.85f),
                   PI / 2.0f, glm::vec3(1, 0, 0), glm::vec3(1.6f, 0.05f, 1.6f));

    // Hour markers (12 around the face, in XY plane)
    for (int i = 0; i < 12; i++) {
        float angle = i * (PI / 6.0f);
        float mx = cosf(angle) * 0.65f;
        float my = sinf(angle) * 0.65f;
        glm::vec3 markerScale = (i % 3 == 0) ? glm::vec3(0.1f, 0.1f, 0.08f) : glm::vec3(0.06f, 0.06f, 0.06f);
        drawHex(pos + glm::vec3(mx, my, 0.1f), glm::vec3(0.15f, 0.15f, 0.15f), markerScale);
    }

    // Center hub
    drawHex(pos + glm::vec3(0, 0, 0.12f), glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(0.1f, 0.1f, 0.1f));

    // Hour hand (short, thick)
    float hourAngle = -fmodf(time * 0.05f, 2.0f * PI);
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0, 0, 0.13f));
        model = myRotate(model, hourAngle, glm::vec3(0, 0, 1));
        model = glm::translate(model, glm::vec3(0.2f, 0, 0));
        model = glm::scale(model, glm::vec3(0.4f, 0.07f, 0.05f));
        drawHexModel(model, glm::vec3(0.12f, 0.12f, 0.12f));
    }

    // Minute hand (longer, thinner)
    float minAngle = -fmodf(time * 0.6f, 2.0f * PI);
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0, 0, 0.15f));
        model = myRotate(model, minAngle, glm::vec3(0, 0, 1));
        model = glm::translate(model, glm::vec3(0.25f, 0, 0));
        model = glm::scale(model, glm::vec3(0.55f, 0.04f, 0.04f));
        drawHexModel(model, glm::vec3(0.08f, 0.08f, 0.08f));
    }
}

// =====================================================
// MineCar — drivable vehicle (arrow keys)
// Chassis + 4 wheels + roll bar + cockpit
// =====================================================
void drawWheel(glm::mat4 parent, float spin) {
    // Wheel is a flat hex rotated on its side, spinning
    glm::mat4 m = parent;
    m = myRotate(m, spin, glm::vec3(0, 0, 1));           // spin
    m = myRotate(m, PI / 2.0f, glm::vec3(0, 0, 1));      // lay flat
    m = glm::scale(m, glm::vec3(0.45f, 0.2f, 0.45f));
    drawHexModel(m, glm::vec3(0.4f, 0.4f, 0.42f));

    // Hub cap
    glm::mat4 hub = parent;
    hub = myRotate(hub, spin, glm::vec3(0, 0, 1));
    hub = myRotate(hub, PI / 2.0f, glm::vec3(0, 0, 1));
    hub = glm::scale(hub, glm::vec3(0.2f, 0.25f, 0.2f));
    drawHexModel(hub, glm::vec3(0.55f, 0.55f, 0.57f));
}

void drawCar(glm::vec3 pos, float yaw, float wSpin) {
    // Base transform: position + rotation
    glm::mat4 base = glm::translate(glm::mat4(1.0f), pos);
    base = myRotate(base, yaw, glm::vec3(0, 1, 0));

    // Chassis — main body (yellow)
    glm::mat4 chassis = glm::translate(base, glm::vec3(0, 0.45f, 0));
    drawHexModel(glm::scale(chassis, glm::vec3(1.4f, 0.35f, 0.8f)),
                 glm::vec3(0.9f, 0.75f, 0.15f));

    // Hood (front, slightly lower)
    glm::mat4 hood = glm::translate(base, glm::vec3(0.7f, 0.35f, 0));
    drawHexModel(glm::scale(hood, glm::vec3(0.6f, 0.25f, 0.75f)),
                 glm::vec3(0.85f, 0.7f, 0.1f));

    // Cockpit back (red)
    glm::mat4 cockpit = glm::translate(base, glm::vec3(-0.3f, 0.7f, 0));
    drawHexModel(glm::scale(cockpit, glm::vec3(0.7f, 0.3f, 0.7f)),
                 glm::vec3(0.8f, 0.15f, 0.1f));

    // Roll bar frame (red arches)
    glm::mat4 barL = glm::translate(base, glm::vec3(-0.2f, 0.95f, -0.35f));
    drawHexModel(glm::scale(barL, glm::vec3(0.08f, 0.3f, 0.08f)),
                 glm::vec3(0.75f, 0.12f, 0.08f));
    glm::mat4 barR = glm::translate(base, glm::vec3(-0.2f, 0.95f, 0.35f));
    drawHexModel(glm::scale(barR, glm::vec3(0.08f, 0.3f, 0.08f)),
                 glm::vec3(0.75f, 0.12f, 0.08f));
    // Top bar
    glm::mat4 barTop = glm::translate(base, glm::vec3(-0.2f, 1.12f, 0));
    drawHexModel(glm::scale(barTop, glm::vec3(0.08f, 0.06f, 0.7f)),
                 glm::vec3(0.75f, 0.12f, 0.08f));

    // Bumper (front, light blue)
    glm::mat4 bumper = glm::translate(base, glm::vec3(1.05f, 0.3f, 0));
    drawHexModel(glm::scale(bumper, glm::vec3(0.15f, 0.15f, 0.85f)),
                 glm::vec3(0.3f, 0.7f, 0.85f));

    // 4 wheels
    float wx = 0.6f, wz = 0.55f, wy = 0.2f;
    drawWheel(glm::translate(base, glm::vec3( wx, wy,  wz)), wSpin);  // front-right
    drawWheel(glm::translate(base, glm::vec3( wx, wy, -wz)), wSpin);  // front-left
    drawWheel(glm::translate(base, glm::vec3(-wx, wy,  wz)), wSpin);  // rear-right
    drawWheel(glm::translate(base, glm::vec3(-wx, wy, -wz)), wSpin);  // rear-left
}

// =====================================================
// Render all interactive objects
// =====================================================
// Helper: get ground Y at a grid position
float getGroundY(int col, int row) {
    // Find highest non-air block
    for (int h = GRID_H - 1; h >= 0; h--) {
        if (getBlock(col, row, h) != BLOCK_AIR)
            return (float)(h + 1) * HEX_HEIGHT;
    }
    // Fallback: use biome calculation
    int biome = getBiome(col, row);
    return (float)(getTerrainHeightBiome(col, row, biome) + 1) * HEX_HEIGHT;
}

// =====================================================
// World-to-grid conversion (from world pos to grid col/row/h)
// =====================================================
void worldToGrid(glm::vec3 wpos, int &col, int &row, int &h) {
    float zSpacing = HEX_RADIUS * 1.5f;
    float xSpacing = HEX_RADIUS * 2.0f * 0.866f;
    row = (int)roundf(wpos.z / zSpacing);
    // Offset for odd rows (use abs to handle negative rows correctly)
    float xOff = (abs(row) % 2 == 1) ? (xSpacing * 0.5f) : 0.0f;
    col = (int)roundf((wpos.x - xOff) / xSpacing);
    h = (int)floorf(wpos.y / HEX_HEIGHT + 0.5f);
    if (h < 0) h = 0;
    if (h >= GRID_H) h = GRID_H - 1;
}

// =====================================================
// Block raycasting — find which block camera is looking at
// =====================================================
// Debug: print ray info once per second
static float lastDebugPrint = 0.0f;

void updateBlockTarget() {
    hasTarget = false;
    float step = 0.1f;
    float maxDist = 30.0f;
    int prevCol = -99999, prevRow = -99999, prevH = -99999;
    bool hasPrev = false;

    float now = (float)glfwGetTime();
    bool doPrint = (now - lastDebugPrint > 2.0f);

    if (doPrint) {
        lastDebugPrint = now;
        printf("[Ray] camPos=(%.1f,%.1f,%.1f) camFront=(%.2f,%.2f,%.2f)\n",
               camPos.x, camPos.y, camPos.z, camFront.x, camFront.y, camFront.z);
    }

    for (float d = 0.2f; d < maxDist; d += step) {
        glm::vec3 p = camPos + camFront * d;
        int col, row, h;
        worldToGrid(p, col, row, h);

        if (col == prevCol && row == prevRow && h == prevH) continue;

        int bt = getBlock(col, row, h);

        if (doPrint && d < 6.0f) {
            printf("  d=%.1f world=(%.1f,%.1f,%.1f) grid=(%d,%d,%d) block=%d\n",
                   d, p.x, p.y, p.z, col, row, h, bt);
        }

        if (bt != BLOCK_AIR) {
            hasTarget = true;
            targetCol = col;
            targetRow = row;
            targetHeight = h;
            if (hasPrev) {
                placeCol = prevCol;
                placeRow = prevRow;
                placeHeight = prevH;
            } else {
                placeCol = col;
                placeRow = row;
                placeHeight = h + 1;
            }
            if (doPrint) printf("  >> HIT block %d at (%d,%d,%d)\n", bt, col, row, h);
            return;
        }
        prevCol = col; prevRow = row; prevH = h;
        hasPrev = true;
    }
    if (doPrint) printf("  >> NO HIT\n");
}

// =====================================================
// Draw wireframe hex outline around targeted block
// =====================================================
void drawBlockHighlight() {
    if (!hasTarget) return;
    glm::vec3 pos = hexGridPos(targetCol, targetRow, 0.0f);
    pos.y = targetHeight * HEX_HEIGHT;

    // Draw slightly larger wireframe hex
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0f);
    setBool(shaderProgram, "isEmissive", true);
    setVec3(shaderProgram, "emissiveColor", glm::vec3(1.0f, 1.0f, 1.0f));
    drawHex(pos, glm::vec3(1, 1, 1), glm::vec3(1.05f, 1.05f, 1.05f));
    setBool(shaderProgram, "isEmissive", false);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
}

// =====================================================
// Crosshair + HUD VAOs
// =====================================================
GLuint hudVAO = 0, hudVBO = 0;

void initHUD() {
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    // Will be filled dynamically
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    // pos(3) + normal(3) + color(3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

// Helper: push a colored quad (two triangles) into vertex buffer
void pushQuad(std::vector<float>& verts, float x1, float y1, float x2, float y2, glm::vec3 col) {
    // normal = (0,0,1), all same color
    float n[] = {0,0,1};
    // Triangle 1
    float tri1[] = {x1,y1,0, n[0],n[1],n[2], col.r,col.g,col.b,
                    x2,y1,0, n[0],n[1],n[2], col.r,col.g,col.b,
                    x2,y2,0, n[0],n[1],n[2], col.r,col.g,col.b};
    // Triangle 2
    float tri2[] = {x1,y1,0, n[0],n[1],n[2], col.r,col.g,col.b,
                    x2,y2,0, n[0],n[1],n[2], col.r,col.g,col.b,
                    x1,y2,0, n[0],n[1],n[2], col.r,col.g,col.b};
    for (int i = 0; i < 27; i++) verts.push_back(tri1[i]);
    for (int i = 0; i < 27; i++) verts.push_back(tri2[i]);
}

void renderHUD(int screenW, int screenH) {
    // Set up orthographic projection for screen-space rendering
    glm::mat4 ortho = glm::ortho(0.0f, (float)screenW, 0.0f, (float)screenH, -1.0f, 1.0f);
    setMat4(shaderProgram, "projection", ortho);
    setMat4(shaderProgram, "view", glm::mat4(1.0f));
    glDisable(GL_DEPTH_TEST);

    // Force emissive so lighting doesn't affect HUD
    setBool(shaderProgram, "isEmissive", true);

    std::vector<float> verts;

    // --- Crosshair (+) at screen center ---
    float cx = screenW / 2.0f, cy = screenH / 2.0f;
    float cLen = 12.0f, cThick = 2.0f;
    glm::vec3 white(1.0f, 1.0f, 1.0f);
    pushQuad(verts, cx - cLen, cy - cThick, cx + cLen, cy + cThick, white); // horizontal
    pushQuad(verts, cx - cThick, cy - cLen, cx + cThick, cy + cLen, white); // vertical

    // --- Hotbar (9 slots at bottom center) ---
    float slotSize = 40.0f;
    float slotGap = 4.0f;
    float totalW = HOTBAR_SIZE * slotSize + (HOTBAR_SIZE - 1) * slotGap;
    float hbX = (screenW - totalW) / 2.0f;
    float hbY = 10.0f;

    for (int i = 0; i < HOTBAR_SIZE; i++) {
        float sx = hbX + i * (slotSize + slotGap);
        // Slot background
        glm::vec3 bgCol = (i == hotbarSlot) ? glm::vec3(0.9f, 0.9f, 0.3f) : glm::vec3(0.3f, 0.3f, 0.3f);
        pushQuad(verts, sx, hbY, sx + slotSize, hbY + slotSize, bgCol);
        // Inner (block color preview)
        float pad = 6.0f;
        glm::vec3 bc = getBlockColor(hotbarBlocks[i]);
        pushQuad(verts, sx + pad, hbY + pad, sx + slotSize - pad, hbY + slotSize - pad, bc);
    }

    // --- Health bar (top left) ---
    float barW = 150.0f, barH = 12.0f;
    float barX = 15.0f, barY = screenH - 30.0f;
    pushQuad(verts, barX, barY, barX + barW, barY + barH, glm::vec3(0.3f, 0.0f, 0.0f)); // bg
    pushQuad(verts, barX, barY, barX + barW, barY + barH, glm::vec3(0.9f, 0.15f, 0.1f)); // full health

    // --- Stamina bar ---
    float stY = barY - 18.0f;
    pushQuad(verts, barX, stY, barX + barW, stY + barH, glm::vec3(0.0f, 0.3f, 0.0f)); // bg
    pushQuad(verts, barX, stY, barX + barW, stY + barH, glm::vec3(0.1f, 0.8f, 0.2f)); // full stamina

    // Upload and draw
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    int vertCount = (int)verts.size() / 9;
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(float), verts.data());
    setVec3(shaderProgram, "emissiveColor", glm::vec3(1.0f));
    glm::mat4 identity(1.0f);
    setMat4(shaderProgram, "model", identity);

    // Draw each element with its color via objectColor uniform
    // Actually since we're in emissive mode, the emissiveColor is what matters
    // But we set vertex colors in the buffer — we need to use objectColor per quad
    // Simpler: draw all at once with emissive=true, the color is baked into emissiveColor
    // Let's just iterate and draw quads

    // Actually the simplest approach: draw each quad group separately
    // But for efficiency, let's use a single draw call and rely on objectColor
    // Since emissive mode uses emissiveColor directly, we need a different approach.

    // Simplest fix: temporarily disable the custom emissive and just set objectColor per vertex
    // But fragment shader uses emissiveColor, not objectColor, in emissive mode.
    // Solution: draw each element separately with the right emissiveColor.

    // Let's redraw using drawHex calls positioned in screen space instead.
    // Actually, let me just draw each element with separate draw calls.

    // Reset verts and draw groups
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);

    // Crosshair
    {
        std::vector<float> cv;
        pushQuad(cv, cx - cLen, cy - cThick, cx + cLen, cy + cThick, white);
        pushQuad(cv, cx - cThick, cy - cLen, cx + cThick, cy + cLen, white);
        glBufferSubData(GL_ARRAY_BUFFER, 0, cv.size() * sizeof(float), cv.data());
        setVec3(shaderProgram, "emissiveColor", white);
        setMat4(shaderProgram, "model", identity);
        glDrawArrays(GL_TRIANGLES, 0, (int)cv.size() / 9);
    }

    // Hotbar slots
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        float sx = hbX + i * (slotSize + slotGap);
        glm::vec3 bgCol = (i == hotbarSlot) ? glm::vec3(0.9f, 0.9f, 0.3f) : glm::vec3(0.3f, 0.3f, 0.3f);

        // Background
        std::vector<float> sv;
        pushQuad(sv, sx, hbY, sx + slotSize, hbY + slotSize, bgCol);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
        setVec3(shaderProgram, "emissiveColor", bgCol);
        glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 9);

        // Block color inner
        std::vector<float> iv;
        float pad = 6.0f;
        glm::vec3 bc = getBlockColor(hotbarBlocks[i]);
        pushQuad(iv, sx + pad, hbY + pad, sx + slotSize - pad, hbY + slotSize - pad, bc);
        glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size() * sizeof(float), iv.data());
        setVec3(shaderProgram, "emissiveColor", bc);
        glDrawArrays(GL_TRIANGLES, 0, (int)iv.size() / 9);
    }

    // Health bar
    {
        std::vector<float> hv;
        pushQuad(hv, barX - 1, barY - 1, barX + barW + 1, barY + barH + 1, glm::vec3(0.15f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, hv.size() * sizeof(float), hv.data());
        setVec3(shaderProgram, "emissiveColor", glm::vec3(0.15f));
        glDrawArrays(GL_TRIANGLES, 0, (int)hv.size() / 9);

        hv.clear();
        pushQuad(hv, barX, barY, barX + barW, barY + barH, glm::vec3(0.9f, 0.15f, 0.1f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, hv.size() * sizeof(float), hv.data());
        setVec3(shaderProgram, "emissiveColor", glm::vec3(0.9f, 0.15f, 0.1f));
        glDrawArrays(GL_TRIANGLES, 0, (int)hv.size() / 9);
    }

    // Stamina bar
    {
        std::vector<float> sv;
        pushQuad(sv, barX - 1, stY - 1, barX + barW + 1, stY + barH + 1, glm::vec3(0.15f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
        setVec3(shaderProgram, "emissiveColor", glm::vec3(0.15f));
        glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 9);

        sv.clear();
        pushQuad(sv, barX, stY, barX + barW, stY + barH, glm::vec3(0.1f, 0.8f, 0.2f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
        setVec3(shaderProgram, "emissiveColor", glm::vec3(0.1f, 0.8f, 0.2f));
        glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 9);
    }

    glBindVertexArray(0);
    setBool(shaderProgram, "isEmissive", false);
    glEnable(GL_DEPTH_TEST);
}

// =====================================================
// Block break / place actions
// =====================================================
void breakBlock() {
    if (!hasTarget) return;
    int bt = getBlock(targetCol, targetRow, targetHeight);
    if (bt == BLOCK_WATER) return; // can't break water for now
    setBlock(targetCol, targetRow, targetHeight, BLOCK_AIR);
    printf("[Block] Broke block at (%d, %d, %d)\n", targetCol, targetRow, targetHeight);
}

void placeBlock() {
    if (!hasTarget) return;
    if (!gridInBounds(placeCol, placeRow, placeHeight)) return;
    if (getBlock(placeCol, placeRow, placeHeight) != BLOCK_AIR) return;
    setBlock(placeCol, placeRow, placeHeight, hotbarBlocks[hotbarSlot]);
    printf("[Block] Placed %d at (%d, %d, %d)\n", hotbarBlocks[hotbarSlot], placeCol, placeRow, placeHeight);
}

void renderObjects(float time) {
    // Player — draw at actual player position (only in third-person)
    if (cameraMode == 0) {
        drawPlayer(playerWorldPos, time, playerYaw, playerWalking, playerWalkTime);
    }

    // Fan — Zone E (west)
    int fCol = -15, fRow = 5;
    glm::vec3 fanPos = hexGridPos(fCol, fRow, 0.0f);
    fanPos.y = getGroundY(fCol, fRow);
    drawFan(fanPos);

    // Door — Zone E (west, far from fan)
    int dCol = -18, dRow = 10;
    glm::vec3 doorPos = hexGridPos(dCol, dRow, 0.0f);
    doorPos.y = getGroundY(dCol, dRow);
    drawDoor(doorPos);

    // Clock — Zone E (west, between fan and door)
    int cCol = -15, cRow = 15;
    glm::vec3 clockBase = hexGridPos(cCol, cRow, 0.0f);
    clockBase.y = getGroundY(cCol, cRow);
    drawHex(clockBase + glm::vec3(0, 1.0f, 0), COL_STONE, glm::vec3(0.3f, 2.0f, 0.3f));
    glm::vec3 clockPos = clockBase + glm::vec3(0, 2.8f, 0);
    drawClock(clockPos, time);

    // MineCar — Zone C (sand, south)
    int carCol, carRow;
    float zSp = HEX_RADIUS * 1.5f;
    float xSp = HEX_RADIUS * 2.0f * 0.866f;
    carRow = (int)roundf(carPos.z / zSp);
    float xOff = (carRow % 2) * (xSp * 0.5f);
    carCol = (int)roundf((carPos.x - xOff) / xSp);
    float carGroundY = getGroundY(carCol, carRow);
    drawCar(glm::vec3(carPos.x, carGroundY, carPos.z), carYaw, wheelSpin);

    // --- Phase 6: Curvy Objects ---

    // Sphere — crystal ball on pedestal near ruins (Zone D)
    int sphCol = 18, sphRow = 12;
    glm::vec3 sphBase = hexGridPos(sphCol, sphRow, 0.0f);
    sphBase.y = getGroundY(sphCol, sphRow);
    // Stone pedestal
    drawHex(sphBase + glm::vec3(0, 0.5f, 0), COL_STONE, glm::vec3(0.4f, 1.0f, 0.4f));
    // Glowing sphere on top
    drawSphere(sphBase + glm::vec3(0, 1.8f, 0), 0.6f, glm::vec3(0.3f, 0.8f, 1.0f));

    // Cone — tent in sand zone (Zone C)
    int coneCol = 8, coneRow = -7;
    glm::vec3 coneBase = hexGridPos(coneCol, coneRow, 0.0f);
    coneBase.y = getGroundY(coneCol, coneRow);
    drawCone(coneBase + glm::vec3(0, 0.1f, 0), 1.5f, glm::vec3(0.85f, 0.55f, 0.25f));

    // Bezier curve — decorative arch near ruins (Zone D)
    int bezCol = 22, bezRow = 8;
    glm::vec3 bezBase = hexGridPos(bezCol, bezRow, 0.0f);
    bezBase.y = getGroundY(bezCol, bezRow);
    drawBezier(bezBase + glm::vec3(0, 0.5f, 0), 2.0f, glm::vec3(0.7f, 0.5f, 0.9f));

    // Spline — winding fence in plains (Zone A)
    int splCol = 0, splRow = 8;
    glm::vec3 splBase = hexGridPos(splCol, splRow, 0.0f);
    splBase.y = getGroundY(splCol, splRow);
    drawSpline(splBase + glm::vec3(0, 0.3f, 0), 2.5f, glm::vec3(0.6f, 0.4f, 0.2f));

    // Ruled surface — ramp/slide near interactive zone (Zone E)
    int rmpCol = -12, rmpRow = 12;
    glm::vec3 rmpBase = hexGridPos(rmpCol, rmpRow, 0.0f);
    rmpBase.y = getGroundY(rmpCol, rmpRow);
    drawRuledSurface(rmpBase + glm::vec3(0, 0.3f, 0), 2.0f, glm::vec3(0.5f, 0.7f, 0.5f));
}

// =====================================================
// Update camera direction from pitch/yaw
// =====================================================
void updateCameraVectors() {
    glm::vec3 front;
    front.x = cosf(glm::radians(camYaw)) * cosf(glm::radians(camPitch));
    front.y = sinf(glm::radians(camPitch));
    front.z = sinf(glm::radians(camYaw)) * cosf(glm::radians(camPitch));
    camFront = glm::normalize(front);

    // Apply roll using myRotate
    glm::vec3 worldUp(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(camFront, worldUp));

    // Roll rotates the up vector around the front axis
    glm::mat4 rollMat = myRotate(glm::mat4(1.0f), glm::radians(camRoll), camFront);
    glm::vec4 upRolled = rollMat * glm::vec4(glm::cross(right, camFront), 1.0f);
    camUp = glm::normalize(glm::vec3(upRolled));
}

// =====================================================
// Key handling
// =====================================================
// Helper: convert world XZ to grid col/row
void worldToColRow(float wx, float wz, int &col, int &row) {
    float zSpacing = HEX_RADIUS * 1.5f;
    float xSpacing = HEX_RADIUS * 2.0f * 0.866f;
    row = (int)roundf(wz / zSpacing);
    float xOff = (abs(row) % 2 == 1) ? (xSpacing * 0.5f) : 0.0f;
    col = (int)roundf((wx - xOff) / xSpacing);
}

// Y-aware ground: find highest solid block BELOW the player's feet
// This lets the player walk inside buildings instead of being teleported to the roof
float getGroundYAtHeight(int col, int row, float currentY) {
    int playerH = (int)floorf(currentY / HEX_HEIGHT); // block height at player's feet
    // Search downward from player's feet to find the first solid block below
    for (int h = playerH; h >= 0; h--) {
        if (getBlock(col, row, h) != BLOCK_AIR) {
            return (float)(h + 1) * HEX_HEIGHT;
        }
    }
    // Fallback: nothing below, return 0
    return 0.0f;
}

// Check if a block at (col, row, h) is solid (not air, not water)
bool isSolid(int col, int row, int h) {
    int bt = getBlock(col, row, h);
    return bt != BLOCK_AIR && bt != BLOCK_WATER;
}

// Check if player can stand at world position (wx, wz) at current height
// Player occupies 2 blocks vertically (feet + head)
bool canMoveTo(float wx, float wz, float currentY) {
    int col, row;
    worldToColRow(wx, wz, col, row);
    int feetH = (int)floorf(currentY / HEX_HEIGHT);
    int headH = feetH + 1;
    // If there's a solid block at feet or head height, can't move there
    if (isSolid(col, row, feetH)) return false;
    if (isSolid(col, row, headH)) return false;
    return true;
}

// Legacy: get ground Y at a world XZ (uses highest block — for objects, not player)
float getGroundYWorld(float wx, float wz) {
    int col, row;
    worldToColRow(wx, wz, col, row);
    return getGroundY(col, row);
}

void processInput(GLFWwindow* window) {
    float speed = camSpeed * deltaTime;
    playerWalking = false;

    if (cameraMode == 2) {
        // --- Free-fly mode: WASD moves camera directly ---
        glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += camFront * speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= camFront * speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= right * speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += right * speed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camPos += camUp * speed;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) camPos -= camUp * speed;
    } else {
        // --- Player mode (first/third person): WASD moves the player ---
        // Movement direction based on camera yaw (horizontal only)
        float moveYaw = glm::radians(camYaw);
        glm::vec3 forward(cosf(moveYaw), 0.0f, sinf(moveYaw));
        glm::vec3 right(-sinf(moveYaw), 0.0f, cosf(moveYaw));
        float pSpeed = 4.5f * deltaTime;

        glm::vec3 moveDir(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { moveDir += forward; playerWalking = true; }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { moveDir -= forward; playerWalking = true; }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { moveDir -= right; playerWalking = true; }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { moveDir += right; playerWalking = true; }

        if (glm::length(moveDir) > 0.001f) {
            moveDir = glm::normalize(moveDir);
            float newX = playerWorldPos.x + moveDir.x * pSpeed;
            float newZ = playerWorldPos.z + moveDir.z * pSpeed;
            // Horizontal collision: check if destination is walkable
            // Try full move first, then axis-separated (wall sliding)
            if (canMoveTo(newX, newZ, playerWorldPos.y)) {
                playerWorldPos.x = newX;
                playerWorldPos.z = newZ;
            } else if (canMoveTo(newX, playerWorldPos.z, playerWorldPos.y)) {
                playerWorldPos.x = newX; // slide along Z wall
            } else if (canMoveTo(playerWorldPos.x, newZ, playerWorldPos.y)) {
                playerWorldPos.z = newZ; // slide along X wall
            }
            // Face movement direction
            playerYaw = atan2f(moveDir.z, moveDir.x);
        }

        // Jump
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && playerOnGround) {
            playerVelY = 6.0f;
            playerOnGround = false;
        }

        // Gravity
        playerVelY -= 15.0f * deltaTime;
        playerWorldPos.y += playerVelY * deltaTime;

        // Ground collision — Y-aware: finds ground below player, not roof above
        {
            int col, row;
            worldToColRow(playerWorldPos.x, playerWorldPos.z, col, row);
            float groundY = getGroundYAtHeight(col, row, playerWorldPos.y);
            if (playerWorldPos.y <= groundY) {
                playerWorldPos.y = groundY;
                playerVelY = 0.0f;
                playerOnGround = true;
            }
        }

        // Walk animation timer
        if (playerWalking) {
            playerWalkTime += deltaTime * 6.0f;
        }

        // Update camera to follow player
        if (cameraMode == 0) {
            // Third-person: camera behind and above
            float camYawRad = glm::radians(camYaw);
            float camPitchRad = glm::radians(camPitch);
            glm::vec3 offset;
            offset.x = -cosf(camYawRad) * cosf(camPitchRad) * thirdPersonDist;
            offset.y = -sinf(camPitchRad) * thirdPersonDist + thirdPersonHeight;
            offset.z = -sinf(camYawRad) * cosf(camPitchRad) * thirdPersonDist;
            camPos = playerWorldPos + glm::vec3(0, 2.0f, 0) + offset;
        } else {
            // First-person: camera at player head
            camPos = playerWorldPos + glm::vec3(0, 2.2f, 0);
        }

        // E/R for vertical fly (useful in player modes too)
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) playerWorldPos.y += speed;
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) playerWorldPos.y -= speed;
    }

    // Arrow keys = drive car
    float carAccel = 8.0f * deltaTime;
    float carTurnSpeed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) carSpeed += carAccel;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) carSpeed -= carAccel;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) carYaw += carTurnSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) carYaw -= carTurnSpeed;

    // Pitch (X), Yaw (Y), Roll (Z) — always available
    float rotSpeed = 60.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camPitch -= rotSpeed;
        else
            camPitch += rotSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camYaw -= rotSpeed;
        else
            camYaw += rotSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camRoll -= rotSpeed;
        else
            camRoll += rotSpeed;
    }

    // Friction
    carSpeed *= 0.97f;
    if (fabsf(carSpeed) < 0.01f) carSpeed = 0.0f;

    // Move car
    carPos.x += sinf(carYaw) * carSpeed * deltaTime * 5.0f;
    carPos.z += cosf(carYaw) * carSpeed * deltaTime * 5.0f;

    // Wheel spin based on speed
    wheelSpin += carSpeed * deltaTime * 8.0f;

    // Rotate around look-at point (F) — only in free-fly mode
    if (cameraMode == 2 && glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        float orbitSpeed = 40.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            orbitAngle -= orbitSpeed;
        else
            orbitAngle += orbitSpeed;

        float orbitRadius = glm::length(camPos - lookAtTarget);
        camPos.x = lookAtTarget.x + orbitRadius * cosf(glm::radians(orbitAngle));
        camPos.z = lookAtTarget.z + orbitRadius * sinf(glm::radians(orbitAngle));
        // Point camera at target
        camFront = glm::normalize(lookAtTarget - camPos);
        camYaw = glm::degrees(atan2f(camFront.z, camFront.x));
        camPitch = glm::degrees(asinf(camFront.y));
    }

    // Clamp pitch
    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;

    updateCameraVectors();
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
        return;
    }

    float dx = (float)(xpos - lastMouseX) * mouseSensitivity;
    float dy = (float)(lastMouseY - ypos) * mouseSensitivity;
    lastMouseX = xpos;
    lastMouseY = ypos;

    // Always look around with mouse movement (like Minecraft)
    camYaw += dx;
    camPitch += dy;
    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;
    updateCameraVectors();
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action != GLFW_PRESS) return;

    // Left-click = break block
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (hasTarget) {
            breakBlock();
            printf("[Debug] Break at (%d,%d,%d)\n", targetCol, targetRow, targetHeight);
        } else {
            printf("[Debug] No block targeted\n");
        }
        return;
    }

    // Right-click = place block
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        placeBlock();
        return;
    }

    // Middle click = place block (alternative)
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        placeBlock();
        return;
    }
}

void scrollCallback(GLFWwindow* window, double xoff, double yoff) {
    // Scroll always cycles hotbar
    // Hold Ctrl + Scroll for zoom
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        camPos += camFront * (float)yoff * 1.5f;
    } else {
        hotbarSlot = ((hotbarSlot - (int)yoff) % HOTBAR_SIZE + HOTBAR_SIZE) % HOTBAR_SIZE;
        printf("[Hotbar] Selected: slot %d (%s)\n", hotbarSlot + 1,
               (const char*[]){"Grass","Dirt","Sand","Stone","Wood","Leaf","Diamond","Gold","Glass"}[hotbarSlot]);
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_B:
            birdsEye = !birdsEye;
            if (birdsEye) {
                printf("[Camera] Bird's Eye View ON\n");
            } else {
                printf("[Camera] Bird's Eye View OFF (free cam)\n");
            }
            break;
        case GLFW_KEY_V:
            fourViewport = !fourViewport;
            printf("[Camera] 4-Viewport: %s\n", fourViewport ? "ON" : "OFF");
            break;

        // Lighting type toggles
        case GLFW_KEY_1:
            dirLightOn = !dirLightOn;
            printf("[Light] Directional: %s\n", dirLightOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_2:
            pointLightOn = !pointLightOn;
            printf("[Light] Point lights: %s\n", pointLightOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_3:
            spotLightOn = !spotLightOn;
            printf("[Light] Spot light: %s\n", spotLightOn ? "ON" : "OFF");
            break;

        // Lighting component toggles
        case GLFW_KEY_5:
            ambientOn = !ambientOn;
            printf("[Light] Ambient: %s\n", ambientOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_6:
            diffuseOn = !diffuseOn;
            printf("[Light] Diffuse: %s\n", diffuseOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_7:
            specularOn = !specularOn;
            printf("[Light] Specular: %s\n", specularOn ? "ON" : "OFF");
            break;

        // Master light toggle
        case GLFW_KEY_L:
            lightOn = !lightOn;
            printf("[Light] Master: %s\n", lightOn ? "ON" : "OFF");
            break;

        // Day-night cycle
        case GLFW_KEY_T: {
            dayMode = (dayMode + 1) % 4;
            const char* names[] = {"Night", "Dawn", "Noon", "Dusk"};
            float factors[] = {0.0f, 0.4f, 1.0f, 0.3f};
            dayFactor = factors[dayMode];
            printf("[Time] %s (dayFactor=%.1f)\n", names[dayMode], dayFactor);
            break;
        }

        // Interactive objects
        case GLFW_KEY_G:
            fanOn = !fanOn;
            printf("[Fan] %s\n", fanOn ? "ON" : "OFF");
            break;
        case GLFW_KEY_O:
            doorOpen = !doorOpen;
            printf("[Door] %s\n", doorOpen ? "Opening" : "Closing");
            break;

        // Camera mode cycle
        case GLFW_KEY_C:
            cameraMode = (cameraMode + 1) % 3;
            {
                const char* modeNames[] = {"Third-Person", "First-Person", "Free-Fly"};
                printf("[Camera] Mode: %s\n", modeNames[cameraMode]);
            }
            break;

        // Hotbar quick-select (keys 1-9 conflict with lighting, use F1-F9 or numpad)
        // Using KP_1..KP_9 (numpad)
        case GLFW_KEY_KP_1: hotbarSlot = 0; printf("[Hotbar] Slot 1\n"); break;
        case GLFW_KEY_KP_2: hotbarSlot = 1; printf("[Hotbar] Slot 2\n"); break;
        case GLFW_KEY_KP_3: hotbarSlot = 2; printf("[Hotbar] Slot 3\n"); break;
        case GLFW_KEY_KP_4: hotbarSlot = 3; printf("[Hotbar] Slot 4\n"); break;
        case GLFW_KEY_KP_5: hotbarSlot = 4; printf("[Hotbar] Slot 5\n"); break;
        case GLFW_KEY_KP_6: hotbarSlot = 5; printf("[Hotbar] Slot 6\n"); break;
        case GLFW_KEY_KP_7: hotbarSlot = 6; printf("[Hotbar] Slot 7\n"); break;
        case GLFW_KEY_KP_8: hotbarSlot = 7; printf("[Hotbar] Slot 8\n"); break;
        case GLFW_KEY_KP_9: hotbarSlot = 8; printf("[Hotbar] Slot 9\n"); break;
    }
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
    printf("    E / R       - Fly Up / Down\n");
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
    printf("\n");
    printf("  Day-Night:\n");
    printf("    T           - Cycle: Night > Dawn > Noon > Dusk\n");
    printf("\n");
    printf("  Interactive Objects:\n");
    printf("    G           - Toggle rotating fan\n");
    printf("    O           - Toggle door open/close\n");
    printf("\n");
    printf("  MineCar:\n");
    printf("    Arrow Keys  - Drive (UP/DOWN = accel, LEFT/RIGHT = steer)\n");
    printf("\n");
    printf("  Block Building:\n");
    printf("    Left Click               - Break block\n");
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
    initHUD();
    printf("[World] Generating terrain...\n");
    initBlockGrid();
    printf("[World] Terrain ready! %dx%d grid, %d height layers\n", GRID_W, GRID_D, GRID_H);

    // Initialize player on terrain
    glm::vec3 spawnGrid = hexGridPos(3, 5, 0.0f);
    playerWorldPos = glm::vec3(spawnGrid.x, getGroundY(3, 5), spawnGrid.z);
    printf("[Player] Spawned at (%.1f, %.1f, %.1f)\n", playerWorldPos.x, playerWorldPos.y, playerWorldPos.z);

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

        // Animate fan
        if (fanOn) fanAngle += 5.0f * deltaTime;

        // Animate door (smooth open/close)
        float doorTarget = doorOpen ? 90.0f : 0.0f;
        if (doorAngle < doorTarget) doorAngle += 120.0f * deltaTime;
        if (doorAngle > doorTarget) doorAngle -= 120.0f * deltaTime;
        if (fabsf(doorAngle - doorTarget) < 1.0f) doorAngle = doorTarget;

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
            renderTerrain();
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
            renderTerrain();
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

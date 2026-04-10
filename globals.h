#pragma once

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
    BLOCK_BEDROCK,
    BLOCK_COAL_ORE,
    BLOCK_COUNT
};

// Underground depth: all terrain raised by this many blocks to create minable underground
const int UNDERGROUND_DEPTH = 10;

// 3D voxel grid
int blockGrid[GRID_W][GRID_D][GRID_H];
// Cache: highest non-air block per column (avoids iterating empty air in render)
int columnMaxH[GRID_W][GRID_D];

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

// Item drops
struct ItemDrop {
    glm::vec3 pos;
    glm::vec3 vel;      // for initial bounce
    BlockType type;
    float lifetime;      // despawn after 60s
    float bobPhase;      // for bobbing animation
};
std::vector<ItemDrop> itemDrops;
const float ITEM_PICKUP_RADIUS = 1.5f;
const float ITEM_DESPAWN_TIME = 60.0f;

// Simple inventory (slot -> count) beyond hotbar
int inventoryCounts[BLOCK_COUNT]; // how many of each block type the player has

// =====================================================
// Mobs
// =====================================================
enum MobType { MOB_CHICKEN = 0, MOB_PIG, MOB_ZOMBIE };
enum MobState { MOB_IDLE, MOB_WANDER, MOB_CHASE, MOB_ATTACK };

struct Mob {
    glm::vec3 pos;
    float yaw;           // facing direction
    MobType type;
    MobState state;
    float health;
    float maxHealth;
    float stateTimer;    // time in current state
    float attackCooldown; // for hostile mobs
    float walkTime;      // for walk animation
    bool alive;
};
std::vector<Mob> mobs;
const int MAX_MOBS = 30;
float mobSpawnTimer = 0.0f;

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
bool playerSprinting = false;

// Health, stamina, hunger
float playerHealth = 20.0f;   // max 20 (10 hearts)
float playerMaxHealth = 20.0f;
float playerStamina = 100.0f; // max 100
float playerMaxStamina = 100.0f;
float playerHunger = 20.0f;   // max 20 (10 drumsticks)
float playerMaxHunger = 20.0f;
float hungerTimer = 0.0f;     // accumulates time for hunger depletion
bool playerDead = false;
float fallStartY = 0.0f;      // Y when player left ground (for fall damage)
bool trackingFall = false;

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
glm::vec3 lookAtTarget(3.0f, 3.0f, 8.0f); // castle entrance area

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
bool windowOpen = false;     // P toggle (when not dead)
float windowAngle = 0.0f;   // animates 0 to 90

// MineCar state (arrow keys to drive)
glm::vec3 carPos(-4.0f, 0.0f, 5.0f); // parked near castle entrance
float carYaw = 0.0f;        // heading in radians
float carSpeed = 0.0f;
float wheelSpin = 0.0f;

// Flying birds
struct Bird {
    glm::vec3 pos;
    glm::vec3 waypoints[6]; // spline control points
    int currentWP;           // current waypoint index
    float t;                 // parameter along current spline segment
    float wingPhase;         // wing flap phase
    float speed;
    glm::vec3 color;
};
std::vector<Bird> birds;

// Gouraud/Phong toggle
bool useGouraud = false;     // H toggle

// Shader
GLuint shaderProgram;

// Textures
GLuint texBrick = 0, texGrass = 0, texWood = 0;

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

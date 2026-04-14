#pragma once
// =====================================================
// Helper: convert world XZ to grid col/row
// =====================================================
void worldToColRow(float wx, float wz, int &col, int &row) {
    float zSpacing = HEX_RADIUS * 1.5f;
    float xSpacing = HEX_RADIUS * 2.0f * 0.866f;
    row = (int)roundf(wz / zSpacing);
    float xOff = (abs(row) % 2 == 1) ? (xSpacing * 0.5f) : 0.0f;
    col = (int)roundf((wx - xOff) / xSpacing);
}

// Y-aware ground: find highest solid block BELOW the player's feet
float getGroundYAtHeight(int col, int row, float currentY) {
    int playerH = (int)floorf(currentY / HEX_HEIGHT);
    for (int h = playerH; h >= 0; h--) {
        if (getBlock(col, row, h) != BLOCK_AIR) {
            return (float)(h + 1) * HEX_HEIGHT;
        }
    }
    return 0.0f;
}

// Check if a block at (col, row, h) is solid for collision
bool isSolid(int col, int row, int h) {
    int bt = getBlock(col, row, h);
    if (bt == BLOCK_AIR || bt == BLOCK_WATER) return false;
    BlockProperties props = getBlockProps(bt);
    if (!props.isSolid) return false;
    // Open interactive blocks (doors, trapdoors, fence gates) are passable
    if (props.isInteractive) {
        uint16_t state = getBlockState(col, row, h);
        if ((state >> 2) & 1) return false; // open = not solid
    }
    return true;
}

// Check if player can stand at world position (wx, wz) at current height.
// Samples 9 points around the player's horizontal radius so blocks that
// are adjacent (but not exactly under the center) are also tested.
bool canMoveTo(float wx, float wz, float currentY) {
    int feetH = (int)floorf(currentY / HEX_HEIGHT);
    int headH = feetH + 1;

    // Player physical half-width — keep slightly under hex half-width (0.43)
    // so a 1-hex-wide corridor is still passable.
    static const float PR = 0.28f;
    static const float SX[9] = { 0,  PR, -PR,  0,    0,  PR*0.707f, -PR*0.707f, -PR*0.707f,  PR*0.707f };
    static const float SZ[9] = { 0,   0,    0, PR, -PR,  PR*0.707f,  PR*0.707f, -PR*0.707f, -PR*0.707f };

    int seenC[9], seenR[9];
    int nSeen = 0;
    for (int i = 0; i < 9; i++) {
        int col, row;
        worldToColRow(wx + SX[i], wz + SZ[i], col, row);
        // skip if we already checked this cell
        bool dup = false;
        for (int j = 0; j < nSeen; j++) {
            if (seenC[j] == col && seenR[j] == row) { dup = true; break; }
        }
        if (dup) continue;
        seenC[nSeen] = col; seenR[nSeen] = row; nSeen++;

        if (isSolid(col, row, feetH)) return false;
        if (isSolid(col, row, headH)) return false;
    }
    return true;
}

// Try to move with auto-step (climb 1-block ledges)
bool tryMoveWithStep(float& px, float& py, float& pz, float nx, float nz) {
    if (canMoveTo(nx, nz, py)) {
        px = nx; pz = nz;
        return true;
    }
    float steppedY = py + HEX_HEIGHT;
    if (canMoveTo(nx, nz, steppedY)) {
        int col, row;
        worldToColRow(nx, nz, col, row);
        float groundY = getGroundYAtHeight(col, row, steppedY);
        if (groundY > py && groundY <= py + HEX_HEIGHT + 0.1f) {
            px = nx; pz = nz;
            py = groundY;
            return true;
        }
    }
    return false;
}

// Legacy: get ground Y at a world XZ (uses highest block — for objects, not player)
float getGroundYWorld(float wx, float wz) {
    int col, row;
    worldToColRow(wx, wz, col, row);
    return getGroundY(col, row);
}

// =====================================================
// Block break / place actions
// =====================================================
void spawnItemDrop(int col, int row, int h, BlockType bt) {
    glm::vec3 pos = hexGridPos(col, row, 0.0f) + glm::vec3(0, h * HEX_HEIGHT + 0.5f, 0);
    ItemDrop drop;
    drop.pos = pos;
    drop.vel = glm::vec3(
        (float)((gridSeed(col, row) % 100) - 50) * 0.02f,
        3.0f,
        (float)((gridSeed(row, col) % 100) - 50) * 0.02f
    );
    drop.type = bt;
    drop.lifetime = 0.0f;
    drop.bobPhase = (float)(gridSeed(col + h, row) % 100) * 0.06f;
    itemDrops.push_back(drop);
}

void breakBlock() {
    if (!hasTarget) return;
    int bt = getBlock(targetCol, targetRow, targetHeight);
    if (bt == BLOCK_AIR) return;
    setBlock(targetCol, targetRow, targetHeight, BLOCK_AIR);
    int dropType = getBlockDrop(bt);
    if (dropType != BLOCK_AIR) {
        addToInventory(dropType, 1);
        printf("[Decompose] %s -> +1 %s added to inventory\n",
               getBlockName(bt), getBlockName(dropType));
    } else {
        printf("[Decompose] %s decomposed (no drop)\n", getBlockName(bt));
    }

    // Degrade tool durability
    InventorySlot& held = playerInventory[hotbarSlot];
    if (held.durability > 0) {
        held.durability--;
        if (held.durability <= 0) {
            printf("[Tool] %s broke!\n", getBlockName(held.type));
            held = {BLOCK_AIR, 0, -1};
        }
    }
}

// Convert player yaw to 4-direction facing (0=+Z, 1=+X, 2=-Z, 3=-X)
int yawToFacing(float yaw) {
    // yaw is atan2(z, x); normalize to [0, 2PI)
    float a = fmodf(yaw + 2.0f * PI, 2.0f * PI);
    // split into 4 quadrants
    if (a < PI * 0.25f || a >= PI * 1.75f) return 0;
    if (a < PI * 0.75f) return 1;
    if (a < PI * 1.25f) return 2;
    return 3;
}

void placeBlock() {
    if (!hasTarget) return;
    if (!gridInBounds(placeCol, placeRow, placeHeight)) return;
    if (getBlock(placeCol, placeRow, placeHeight) != BLOCK_AIR) return;
    int bt = playerInventory[hotbarSlot].type;
    if (bt == BLOCK_AIR || playerInventory[hotbarSlot].count <= 0) return;

    // Items (tools etc.) can't be placed in the world
    if (getBlockProps(bt).isItem) return;

    setBlock(placeCol, placeRow, placeHeight, bt);
    playerInventory[hotbarSlot].count--;
    if (playerInventory[hotbarSlot].count <= 0) playerInventory[hotbarSlot].type = BLOCK_AIR;

    // Set initial facing state for directional blocks
    BlockProperties props = getBlockProps(bt);
    if (props.shape == SHAPE_DOOR || props.shape == SHAPE_TRAPDOOR
        || props.shape == SHAPE_STAIR || props.shape == SHAPE_FENCE
        || props.shape == SHAPE_FLAT_PANEL) {
        uint16_t state = yawToFacing(playerYaw) & 3;
        setBlockState(placeCol, placeRow, placeHeight, state);
    }
    printf("[Block] Placed %d at (%d, %d, %d)\n", bt, placeCol, placeRow, placeHeight);
}

// =====================================================
// Mob system — Phase 15: Mobs & Combat
// =====================================================

// --- 15D: A* Pathfinding on hex grid ---
struct AStarNode {
    int col, row, h;
    float g, f;
    int parentIdx;
};

float hexDistance(int c1, int r1, int c2, int r2) {
    // Axial hex distance
    float dx = (float)(c2 - c1);
    float dz = (float)(r2 - r1);
    return sqrtf(dx * dx + dz * dz);
}

// Get 6 hex neighbors for a given col/row
void getHexNeighbors(int col, int row, int out[][2], int& count) {
    count = 0;
    bool odd = (abs(row) % 2 == 1);
    // 6 hex directions (flat-top): depends on odd/even row
    int dirs[6][2];
    if (odd) {
        dirs[0][0]= 1; dirs[0][1]= 0;
        dirs[1][0]=-1; dirs[1][1]= 0;
        dirs[2][0]= 0; dirs[2][1]= 1;
        dirs[3][0]= 1; dirs[3][1]= 1;
        dirs[4][0]= 0; dirs[4][1]=-1;
        dirs[5][0]= 1; dirs[5][1]=-1;
    } else {
        dirs[0][0]= 1; dirs[0][1]= 0;
        dirs[1][0]=-1; dirs[1][1]= 0;
        dirs[2][0]=-1; dirs[2][1]= 1;
        dirs[3][0]= 0; dirs[3][1]= 1;
        dirs[4][0]=-1; dirs[4][1]=-1;
        dirs[5][0]= 0; dirs[5][1]=-1;
    }
    for (int i = 0; i < 6; i++) {
        out[count][0] = col + dirs[i][0];
        out[count][1] = row + dirs[i][1];
        count++;
    }
}

// A* pathfinding: returns world-space waypoints from start to goal
std::vector<glm::vec3> findPath(glm::vec3 startWorld, glm::vec3 goalWorld) {
    std::vector<glm::vec3> result;
    int sc, sr, gc, gr;
    worldToColRow(startWorld.x, startWorld.z, sc, sr);
    worldToColRow(goalWorld.x, goalWorld.z, gc, gr);

    if (sc == gc && sr == gr) return result;

    const int MAX_NODES = 200;
    std::vector<AStarNode> open;
    std::vector<AStarNode> closed;

    // Key for visited set: pack col/row
    auto packKey = [](int c, int r) -> int { return (c + GRID_OFF_X) * 1000 + (r + GRID_OFF_Z); };
    std::vector<int> closedKeys;

    AStarNode start;
    start.col = sc; start.row = sr;
    start.h = (int)floorf(startWorld.y / HEX_HEIGHT);
    start.g = 0.0f;
    start.f = hexDistance(sc, sr, gc, gr);
    start.parentIdx = -1;
    open.push_back(start);

    int totalExpanded = 0;
    while (!open.empty() && totalExpanded < MAX_NODES) {
        // Find lowest f in open
        int bestI = 0;
        for (int i = 1; i < (int)open.size(); i++) {
            if (open[i].f < open[bestI].f) bestI = i;
        }
        AStarNode cur = open[bestI];
        open.erase(open.begin() + bestI);

        // Check if reached goal
        if (cur.col == gc && cur.row == gr) {
            // Reconstruct path
            std::vector<glm::vec3> path;
            AStarNode n = cur;
            while (n.parentIdx >= 0) {
                glm::vec3 wp = hexGridPos(n.col, n.row, 0.0f);
                wp.y = getGroundY(n.col, n.row);
                path.push_back(wp);
                n = closed[n.parentIdx];
            }
            // Reverse (goal->start -> start->goal)
            for (int i = (int)path.size() - 1; i >= 0; i--)
                result.push_back(path[i]);
            return result;
        }

        int closedIdx = (int)closed.size();
        closed.push_back(cur);
        closedKeys.push_back(packKey(cur.col, cur.row));
        totalExpanded++;

        // Expand neighbors
        int neighbors[6][2];
        int ncount;
        getHexNeighbors(cur.col, cur.row, neighbors, ncount);
        for (int i = 0; i < ncount; i++) {
            int nc = neighbors[i][0], nr = neighbors[i][1];
            if (!gridInBounds(nc, nr, 0)) continue;

            // Check walkable: need solid ground below and air at feet+head
            int groundH = -1;
            for (int h = cur.h + 1; h >= cur.h - 2 && h >= 0; h--) {
                if (h < GRID_H && getBlock(nc, nr, h) != BLOCK_AIR &&
                    getBlock(nc, nr, h) != BLOCK_WATER) {
                    groundH = h;
                    break;
                }
            }
            if (groundH < 0) continue;
            int feetH = groundH + 1;
            int headH = groundH + 2;
            if (feetH >= GRID_H) continue;
            if (getBlock(nc, nr, feetH) != BLOCK_AIR) continue;
            if (headH < GRID_H && getBlock(nc, nr, headH) != BLOCK_AIR) continue;

            // Skip if in closed
            int key = packKey(nc, nr);
            bool inClosed = false;
            for (auto& ck : closedKeys) if (ck == key) { inClosed = true; break; }
            if (inClosed) continue;

            float newG = cur.g + 1.0f;
            // Check if in open with better g
            bool inOpen = false;
            for (auto& o : open) {
                if (o.col == nc && o.row == nr) {
                    inOpen = true;
                    if (newG < o.g) {
                        o.g = newG;
                        o.f = newG + hexDistance(nc, nr, gc, gr);
                        o.parentIdx = closedIdx;
                        o.h = feetH;
                    }
                    break;
                }
            }
            if (!inOpen) {
                AStarNode nn;
                nn.col = nc; nn.row = nr; nn.h = feetH;
                nn.g = newG;
                nn.f = newG + hexDistance(nc, nr, gc, gr);
                nn.parentIdx = closedIdx;
                open.push_back(nn);
            }
        }
    }
    return result; // empty = no path found
}

// Move mob along A* path, returns true if still following
bool followPath(Mob& m, float dt) {
    if (m.path.empty() || m.pathIndex >= (int)m.path.size()) return false;
    glm::vec3 target = m.path[m.pathIndex];
    glm::vec3 dir = target - m.pos;
    dir.y = 0.0f;
    float dist = glm::length(dir);
    if (dist < 0.5f) {
        m.pathIndex++;
        if (m.pathIndex >= (int)m.path.size()) return false;
        target = m.path[m.pathIndex];
        dir = target - m.pos;
        dir.y = 0.0f;
        dist = glm::length(dir);
    }
    if (dist > 0.01f) {
        dir = glm::normalize(dir);
        m.yaw = atan2f(dir.z, dir.x);
        bool isHostile = (m.type == MOB_ZOMBIE || m.type == MOB_SKELETON);
        float speed = (isHostile ? 2.5f : 1.2f) * dt;
        m.pos.x += dir.x * speed;
        m.pos.z += dir.z * speed;
        m.walkTime += dt * (isHostile ? 4.0f : 3.0f);
    }
    return true;
}

void spawnMob(MobType type, glm::vec3 pos) {
    if ((int)mobs.size() >= MAX_MOBS) return;
    Mob m;
    m.pos = pos;
    m.yaw = (float)(gridSeed((int)(pos.x * 7), (int)(pos.z * 13)) % 628) / 100.0f;
    m.type = type;
    m.state = MOB_IDLE;
    if (type == MOB_ZOMBIE || type == MOB_SKELETON)
        m.health = 20.0f;
    else if (type == MOB_SHEEP)
        m.health = 8.0f;
    else
        m.health = 8.0f; // chicken, pig
    m.maxHealth = m.health;
    m.stateTimer = 0.0f;
    m.attackCooldown = 0.0f;
    m.walkTime = 0.0f;
    m.alive = true;
    m.hitFlash = 0.0f;
    m.deathTimer = -1.0f; // alive
    m.pathIndex = 0;
    m.pathTimer = 0.0f;
    mobs.push_back(m);
}

void spawnInitialMobs() {
    // Passive mobs in grass zones: chickens, pigs, sheep
    for (int i = 0; i < 12; i++) {
        unsigned int s = gridSeed(i * 17, i * 31 + 5);
        int col = (int)(s % 40) - 20;
        int row = (int)((s >> 8) % 40) - 20;
        glm::vec3 pos = hexGridPos(col, row, 0.0f);
        pos.y = getGroundY(col, row);
        MobType t;
        int r = s % 3;
        if (r == 0) t = MOB_CHICKEN;
        else if (r == 1) t = MOB_PIG;
        else t = MOB_SHEEP;
        spawnMob(t, pos);
    }
}

// Helper: get mob name for printing
const char* getMobName(MobType t) {
    switch (t) {
        case MOB_CHICKEN: return "Chicken";
        case MOB_PIG: return "Pig";
        case MOB_SHEEP: return "Sheep";
        case MOB_ZOMBIE: return "Zombie";
        case MOB_SKELETON: return "Skeleton";
        default: return "Mob";
    }
}

void drawMob(const Mob& m, float time) {
    // Death animation: mob is dying (falling over + fading)
    if (m.deathTimer >= 0.0f) {
        float t = m.deathTimer;
        float fadeAlpha = 1.0f - (t / 1.0f); // fade over 1 second
        if (fadeAlpha <= 0.0f) return;
        setFloat(shaderProgram, "alpha", fadeAlpha);
        // Tilt the mob to fall over (rotate around Z axis)
        float tiltAngle = t * 1.57f; // 90 degrees over ~1s
        if (tiltAngle > 1.57f) tiltAngle = 1.57f;
        glm::vec3 p = m.pos;
        // Draw a simplified falling body
        glm::vec3 col;
        if (m.type == MOB_ZOMBIE) col = glm::vec3(0.3f, 0.5f, 0.2f);
        else if (m.type == MOB_SKELETON) col = glm::vec3(0.85f, 0.85f, 0.8f);
        else if (m.type == MOB_PIG) col = glm::vec3(0.9f, 0.6f, 0.6f);
        else if (m.type == MOB_SHEEP) col = glm::vec3(0.95f, 0.95f, 0.9f);
        else col = glm::vec3(0.9f, 0.9f, 0.85f);
        drawHexRotated(p + glm::vec3(0, 0.5f, 0), col, tiltAngle, glm::vec3(0, 0, 1), glm::vec3(0.4f, 0.6f, 0.3f));
        setFloat(shaderProgram, "alpha", 1.0f);
        return;
    }

    if (!m.alive) return;

    // Set damage tint if hit
    if (m.hitFlash > 0.0f) {
        setVec3(shaderProgram, "colorTint", glm::vec3(1.0f, 0.15f, 0.1f));
        setFloat(shaderProgram, "colorTintStrength", m.hitFlash / 0.3f);
    }

    glm::vec3 p = m.pos;

    if (m.type == MOB_CHICKEN) {
        float bob = sinf(m.walkTime * 4.0f) * 0.05f;
        drawHex(p + glm::vec3(0, 0.3f + bob, 0), glm::vec3(0.9f, 0.9f, 0.85f), glm::vec3(0.3f, 0.3f, 0.3f));
        drawHex(p + glm::vec3(0, 0.6f + bob, 0), glm::vec3(0.9f, 0.9f, 0.85f), glm::vec3(0.2f, 0.2f, 0.2f));
        drawHex(p + glm::vec3(0.15f, 0.6f + bob, 0), glm::vec3(0.9f, 0.7f, 0.1f), glm::vec3(0.08f, 0.05f, 0.05f));
        drawHex(p + glm::vec3(0, 0.75f + bob, 0), glm::vec3(0.8f, 0.1f, 0.1f), glm::vec3(0.08f, 0.1f, 0.08f));
        drawHex(p + glm::vec3(0.08f, 0.1f, 0), glm::vec3(0.9f, 0.7f, 0.1f), glm::vec3(0.04f, 0.15f, 0.04f));
        drawHex(p + glm::vec3(-0.08f, 0.1f, 0), glm::vec3(0.9f, 0.7f, 0.1f), glm::vec3(0.04f, 0.15f, 0.04f));
    } else if (m.type == MOB_PIG) {
        float bob = sinf(m.walkTime * 3.0f) * 0.03f;
        glm::vec3 pink(0.9f, 0.6f, 0.6f);
        glm::vec3 darkPink(0.8f, 0.5f, 0.5f);
        drawHex(p + glm::vec3(0, 0.35f + bob, 0), pink, glm::vec3(0.4f, 0.35f, 0.35f));
        drawHex(p + glm::vec3(0.3f, 0.4f + bob, 0), pink, glm::vec3(0.25f, 0.25f, 0.25f));
        drawHex(p + glm::vec3(0.45f, 0.38f + bob, 0), darkPink, glm::vec3(0.08f, 0.08f, 0.08f));
        drawHex(p + glm::vec3(0.15f, 0.1f, 0.15f), darkPink, glm::vec3(0.06f, 0.15f, 0.06f));
        drawHex(p + glm::vec3(0.15f, 0.1f, -0.15f), darkPink, glm::vec3(0.06f, 0.15f, 0.06f));
        drawHex(p + glm::vec3(-0.15f, 0.1f, 0.15f), darkPink, glm::vec3(0.06f, 0.15f, 0.06f));
        drawHex(p + glm::vec3(-0.15f, 0.1f, -0.15f), darkPink, glm::vec3(0.06f, 0.15f, 0.06f));
    } else if (m.type == MOB_SHEEP) {
        // White woolly body, dark face and legs
        float bob = sinf(m.walkTime * 3.0f) * 0.03f;
        glm::vec3 wool(0.95f, 0.95f, 0.9f);
        glm::vec3 face(0.35f, 0.3f, 0.25f);
        // Body (big woolly block)
        drawHex(p + glm::vec3(0, 0.4f + bob, 0), wool, glm::vec3(0.45f, 0.4f, 0.4f));
        // Head (dark)
        drawHex(p + glm::vec3(0.35f, 0.5f + bob, 0), face, glm::vec3(0.2f, 0.22f, 0.2f));
        // Ears
        drawHex(p + glm::vec3(0.35f, 0.65f + bob, 0.1f), face, glm::vec3(0.06f, 0.06f, 0.06f));
        drawHex(p + glm::vec3(0.35f, 0.65f + bob, -0.1f), face, glm::vec3(0.06f, 0.06f, 0.06f));
        // Legs (dark)
        float legSwing = sinf(m.walkTime * 3.0f) * 0.06f;
        drawHex(p + glm::vec3(0.15f, 0.1f, 0.15f + legSwing), face, glm::vec3(0.06f, 0.15f, 0.06f));
        drawHex(p + glm::vec3(0.15f, 0.1f, -0.15f - legSwing), face, glm::vec3(0.06f, 0.15f, 0.06f));
        drawHex(p + glm::vec3(-0.15f, 0.1f, 0.15f - legSwing), face, glm::vec3(0.06f, 0.15f, 0.06f));
        drawHex(p + glm::vec3(-0.15f, 0.1f, -0.15f + legSwing), face, glm::vec3(0.06f, 0.15f, 0.06f));
    } else if (m.type == MOB_ZOMBIE) {
        float bob = sinf(m.walkTime * 3.5f) * 0.04f;
        glm::vec3 zGreen(0.3f, 0.5f, 0.2f);
        glm::vec3 zDark(0.2f, 0.35f, 0.15f);
        glm::vec3 zShirt(0.25f, 0.4f, 0.3f);
        drawHex(p + glm::vec3(0, 0.975f + bob, 0), zShirt, glm::vec3(0.40f, 0.65f, 0.25f));
        drawHex(p + glm::vec3(0, 1.55f + bob, 0), zDark, glm::vec3(0.35f, 0.45f, 0.35f));
        float armSwing = sinf(m.walkTime * 3.5f) * 0.15f;
        drawHex(p + glm::vec3(0.35f, 0.85f + bob, 0.2f + armSwing), zGreen, glm::vec3(0.14f, 0.55f, 0.14f));
        drawHex(p + glm::vec3(-0.35f, 0.85f + bob, 0.2f - armSwing), zGreen, glm::vec3(0.14f, 0.55f, 0.14f));
        float legSwing = sinf(m.walkTime * 3.5f) * 0.12f;
        drawHex(p + glm::vec3(0.12f, 0.325f, legSwing), zDark, glm::vec3(0.15f, 0.55f, 0.15f));
        drawHex(p + glm::vec3(-0.12f, 0.325f, -legSwing), zDark, glm::vec3(0.15f, 0.55f, 0.15f));
    } else if (m.type == MOB_SKELETON) {
        // Gray/white bony humanoid with bow
        float bob = sinf(m.walkTime * 3.5f) * 0.04f;
        glm::vec3 bone(0.85f, 0.85f, 0.8f);
        glm::vec3 dark(0.15f, 0.15f, 0.15f); // eye sockets
        glm::vec3 bowCol(0.5f, 0.3f, 0.15f);
        // Body (ribcage — thinner than zombie)
        drawHex(p + glm::vec3(0, 0.975f + bob, 0), bone, glm::vec3(0.30f, 0.60f, 0.18f));
        // Head (skull)
        drawHex(p + glm::vec3(0, 1.55f + bob, 0), bone, glm::vec3(0.30f, 0.38f, 0.30f));
        // Eye sockets
        drawHex(p + glm::vec3(0.12f, 1.6f + bob, 0.15f), dark, glm::vec3(0.06f, 0.06f, 0.04f));
        drawHex(p + glm::vec3(0.12f, 1.6f + bob, -0.15f), dark, glm::vec3(0.06f, 0.06f, 0.04f));
        // Arms (one holds bow)
        float armSwing = sinf(m.walkTime * 3.5f) * 0.1f;
        drawHex(p + glm::vec3(0.28f, 0.85f + bob, 0.15f + armSwing), bone, glm::vec3(0.08f, 0.50f, 0.08f));
        drawHex(p + glm::vec3(-0.28f, 0.85f + bob, -0.15f - armSwing), bone, glm::vec3(0.08f, 0.50f, 0.08f));
        // Bow in right hand
        drawHex(p + glm::vec3(0.35f, 0.7f + bob, 0.15f + armSwing), bowCol, glm::vec3(0.04f, 0.25f, 0.04f));
        // Legs (thin)
        float legSwing = sinf(m.walkTime * 3.5f) * 0.12f;
        drawHex(p + glm::vec3(0.08f, 0.325f, legSwing), bone, glm::vec3(0.08f, 0.50f, 0.08f));
        drawHex(p + glm::vec3(-0.08f, 0.325f, -legSwing), bone, glm::vec3(0.08f, 0.50f, 0.08f));
    }

    // Health bar above head if damaged (all mobs)
    if (m.health < m.maxHealth && m.alive) {
        float hFrac = m.health / m.maxHealth;
        float headY = (m.type == MOB_ZOMBIE || m.type == MOB_SKELETON) ? 2.0f :
                       (m.type == MOB_SHEEP || m.type == MOB_PIG) ? 0.9f : 0.9f;
        drawHex(p + glm::vec3(0, headY, 0), glm::vec3(0.8f, 0.1f, 0.1f), glm::vec3(0.3f * hFrac, 0.04f, 0.04f));
    }

    // Reset tint
    if (m.hitFlash > 0.0f) {
        setVec3(shaderProgram, "colorTint", glm::vec3(0.0f));
        setFloat(shaderProgram, "colorTintStrength", 0.0f);
    }
}

// --- Arrow projectiles (skeleton ranged attack) ---
void spawnArrow(glm::vec3 from, glm::vec3 target) {
    Arrow a;
    a.pos = from + glm::vec3(0, 1.2f, 0); // shoot from chest height
    glm::vec3 dir = target - a.pos;
    dir.y += 1.0f; // arc upward slightly
    if (glm::length(dir) > 0.01f) dir = glm::normalize(dir);
    a.vel = dir * 12.0f; // arrow speed
    a.lifetime = 0.0f;
    a.active = true;
    arrows.push_back(a);
}

void updateArrows(float dt) {
    for (int i = (int)arrows.size() - 1; i >= 0; i--) {
        Arrow& a = arrows[i];
        if (!a.active) { arrows.erase(arrows.begin() + i); continue; }
        a.lifetime += dt;
        a.vel.y -= 9.8f * dt; // gravity
        a.pos += a.vel * dt;

        // Hit player?
        float distToPlayer = glm::length(a.pos - (playerWorldPos + glm::vec3(0, 0.9f, 0)));
        if (distToPlayer < 1.0f) {
            playerHealth -= 3.0f;
            if (playerHealth < 0.0f) playerHealth = 0.0f;
            printf("[Combat] Arrow hit you! Health: %.0f\n", playerHealth);
            if (playerHealth <= 0.0f && !playerDead) {
                playerDead = true;
                printf("[Player] YOU DIED! Press P to respawn.\n");
            }
            a.active = false;
            continue;
        }

        // Hit ground or timeout?
        int ac, ar;
        worldToColRow(a.pos.x, a.pos.z, ac, ar);
        float groundY = getGroundYWorld(a.pos.x, a.pos.z);
        if (a.pos.y <= groundY || a.lifetime > 5.0f) {
            a.active = false;
        }
    }
}

void drawArrows() {
    for (auto& a : arrows) {
        if (!a.active) continue;
        // Arrow: thin brown stick
        glm::vec3 dir = glm::normalize(a.vel);
        float yaw = atan2f(dir.z, dir.x);
        float pitch = asinf(dir.y);
        drawHexRotated(a.pos, glm::vec3(0.5f, 0.35f, 0.15f), yaw, glm::vec3(0, 1, 0), glm::vec3(0.03f, 0.03f, 0.4f));
    }
}

void updateMobs(float dt, float time) {
    // Update player attack cooldown
    playerAttackCooldown -= dt;
    if (playerAttackCooldown < 0.0f) playerAttackCooldown = 0.0f;

    // Spawn hostile mobs at night
    mobSpawnTimer += dt;
    if (dayFactor < 0.3f && mobSpawnTimer > 5.0f) {
        mobSpawnTimer = 0.0f;
        int hostileCount = 0;
        for (auto& m : mobs) if ((m.type == MOB_ZOMBIE || m.type == MOB_SKELETON) && m.alive) hostileCount++;
        if (hostileCount < 10) {
            unsigned int s = gridSeed((int)(time * 100), (int)(playerWorldPos.x * 7));
            float angle = (float)(s % 628) / 100.0f;
            float dist = 15.0f + (float)(s % 10);
            glm::vec3 spawnPos = playerWorldPos + glm::vec3(cosf(angle) * dist, 0, sinf(angle) * dist);
            int sc, sr;
            worldToColRow(spawnPos.x, spawnPos.z, sc, sr);
            spawnPos.y = getGroundY(sc, sr);
            // Torch safe zone check
            bool nearTorch = false;
            for (auto& tp : torchPositions) {
                if (glm::length(tp - spawnPos) < 5.0f) { nearTorch = true; break; }
            }
            if (!nearTorch && glm::length(spawnPos - playerWorldPos) > 8.0f) {
                // Alternate zombie and skeleton
                MobType spawnType = (s % 3 == 0) ? MOB_SKELETON : MOB_ZOMBIE;
                spawnMob(spawnType, spawnPos);
                printf("[Mob] %s spawned!\n", getMobName(spawnType));
            }
        }
    }

    // Despawn hostile mobs during day
    if (dayFactor > 0.5f) {
        for (int i = (int)mobs.size() - 1; i >= 0; i--) {
            if (mobs[i].type == MOB_ZOMBIE || mobs[i].type == MOB_SKELETON) {
                mobs.erase(mobs.begin() + i);
            }
        }
    }

    // Update death animations + remove finished deaths
    for (int i = (int)mobs.size() - 1; i >= 0; i--) {
        if (mobs[i].deathTimer >= 0.0f) {
            mobs[i].deathTimer += dt;
            if (mobs[i].deathTimer > 1.0f) {
                mobs.erase(mobs.begin() + i);
            }
            continue; // skip normal update for dying mobs
        }
    }

    for (auto& m : mobs) {
        if (!m.alive || m.deathTimer >= 0.0f) continue;
        m.stateTimer += dt;
        m.attackCooldown -= dt;
        if (m.attackCooldown < 0.0f) m.attackCooldown = 0.0f;
        m.hitFlash -= dt;
        if (m.hitFlash < 0.0f) m.hitFlash = 0.0f;
        m.pathTimer += dt;

        float distToPlayer = glm::length(m.pos - playerWorldPos);

        if (m.type == MOB_ZOMBIE) {
            // Hostile: chase player within 16 blocks, use A* pathfinding
            if (distToPlayer < 16.0f && !playerDead) {
                m.state = MOB_CHASE;
                // Recalculate path every 1.5 seconds
                if (m.pathTimer > 1.5f) {
                    m.pathTimer = 0.0f;
                    m.path = findPath(m.pos, playerWorldPos);
                    m.pathIndex = 0;
                }
                if (!followPath(m, dt)) {
                    // Direct approach if no path
                    glm::vec3 dir = playerWorldPos - m.pos;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.5f) {
                        dir = glm::normalize(dir);
                        m.yaw = atan2f(dir.z, dir.x);
                        m.pos.x += dir.x * 2.5f * dt;
                        m.pos.z += dir.z * 2.5f * dt;
                        m.walkTime += dt * 4.0f;
                    }
                }
                // Attack player when close
                if (distToPlayer < 1.5f && m.attackCooldown <= 0.0f) {
                    playerHealth -= 3.0f;
                    if (playerHealth < 0.0f) playerHealth = 0.0f;
                    m.attackCooldown = 1.0f;
                    printf("[Mob] Zombie hit you! Health: %.0f\n", playerHealth);
                    if (playerHealth <= 0.0f && !playerDead) {
                        playerDead = true;
                        printf("[Player] YOU DIED! Press P to respawn.\n");
                    }
                }
            } else {
                m.state = MOB_WANDER;
                m.path.clear();
                if (m.stateTimer > 3.0f) {
                    m.stateTimer = 0.0f;
                    unsigned int s = gridSeed((int)(m.pos.x * 10 + time * 100), (int)(m.pos.z * 10));
                    m.state = (s % 3 == 0) ? MOB_WANDER : MOB_IDLE;
                    if (m.state == MOB_WANDER) m.yaw = (float)(s % 628) / 100.0f;
                }
                if (m.state == MOB_WANDER) {
                    m.pos.x += cosf(m.yaw) * 1.5f * dt;
                    m.pos.z += sinf(m.yaw) * 1.5f * dt;
                    m.walkTime += dt * 3.0f;
                }
            }
        } else if (m.type == MOB_SKELETON) {
            // Ranged hostile: approach to 8-15 range, shoot arrows
            if (distToPlayer < 20.0f && !playerDead) {
                m.state = MOB_CHASE;
                // Recalculate path every 2 seconds
                if (m.pathTimer > 2.0f) {
                    m.pathTimer = 0.0f;
                    // If too far, path toward player; if in range, stay put
                    if (distToPlayer > 15.0f) {
                        m.path = findPath(m.pos, playerWorldPos);
                        m.pathIndex = 0;
                    } else {
                        m.path.clear(); // in range, stop moving
                    }
                }

                if (distToPlayer > 15.0f) {
                    if (!followPath(m, dt)) {
                        glm::vec3 dir = playerWorldPos - m.pos;
                        dir.y = 0.0f;
                        if (glm::length(dir) > 0.5f) {
                            dir = glm::normalize(dir);
                            m.yaw = atan2f(dir.z, dir.x);
                            m.pos.x += dir.x * 2.0f * dt;
                            m.pos.z += dir.z * 2.0f * dt;
                            m.walkTime += dt * 3.5f;
                        }
                    }
                } else if (distToPlayer < 6.0f) {
                    // Too close — back away
                    glm::vec3 dir = m.pos - playerWorldPos;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.1f) {
                        dir = glm::normalize(dir);
                        m.yaw = atan2f(-dir.z, -dir.x); // face player while retreating
                        m.pos.x += dir.x * 1.5f * dt;
                        m.pos.z += dir.z * 1.5f * dt;
                        m.walkTime += dt * 3.0f;
                    }
                } else {
                    // In range: face player
                    glm::vec3 dir = playerWorldPos - m.pos;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.1f) m.yaw = atan2f(dir.z, dir.x);
                }

                // Shoot arrows at 8-15 block range
                if (distToPlayer >= 5.0f && distToPlayer <= 18.0f && m.attackCooldown <= 0.0f) {
                    spawnArrow(m.pos, playerWorldPos + glm::vec3(0, 0.9f, 0));
                    m.attackCooldown = 2.0f; // slower than zombie melee
                    printf("[Mob] Skeleton shoots an arrow!\n");
                }
            } else {
                m.state = MOB_WANDER;
                m.path.clear();
                if (m.stateTimer > 3.0f) {
                    m.stateTimer = 0.0f;
                    unsigned int s = gridSeed((int)(m.pos.x * 10 + time * 100), (int)(m.pos.z * 10));
                    m.state = (s % 3 == 0) ? MOB_WANDER : MOB_IDLE;
                    if (m.state == MOB_WANDER) m.yaw = (float)(s % 628) / 100.0f;
                }
                if (m.state == MOB_WANDER) {
                    m.pos.x += cosf(m.yaw) * 1.5f * dt;
                    m.pos.z += sinf(m.yaw) * 1.5f * dt;
                    m.walkTime += dt * 3.0f;
                }
            }
        } else {
            // Passive mobs: wander randomly, flee when attacked
            if (m.state == MOB_FLEE) {
                // Run away from player for 3 seconds
                if (m.stateTimer > 3.0f) {
                    m.state = MOB_IDLE;
                    m.stateTimer = 0.0f;
                } else {
                    glm::vec3 dir = m.pos - playerWorldPos;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.1f) {
                        dir = glm::normalize(dir);
                        m.yaw = atan2f(dir.z, dir.x);
                        float fleeSpeed = 3.0f * dt;
                        m.pos.x += dir.x * fleeSpeed;
                        m.pos.z += dir.z * fleeSpeed;
                        m.walkTime += dt * 5.0f;
                    }
                }
            } else {
                if (m.stateTimer > 3.0f) {
                    m.stateTimer = 0.0f;
                    unsigned int s = gridSeed((int)(m.pos.x * 10 + time * 100), (int)(m.pos.z * 10));
                    m.state = (s % 3 == 0) ? MOB_WANDER : MOB_IDLE;
                    if (m.state == MOB_WANDER) {
                        m.yaw = (float)(s % 628) / 100.0f;
                    }
                }
                if (m.state == MOB_WANDER) {
                    float speed = 1.2f * dt;
                    m.pos.x += cosf(m.yaw) * speed;
                    m.pos.z += sinf(m.yaw) * speed;
                    m.walkTime += dt * 3.0f;
                }
            }
        }

        // Snap to ground
        m.pos.y = getGroundYWorld(m.pos.x, m.pos.z);
    }

    // Update arrows
    updateArrows(dt);

    // Handle dying mobs (deathTimer started but not yet removed — handled above)
    // Drop loot for newly killed mobs (alive=false, deathTimer not started)
    for (auto& m : mobs) {
        if (!m.alive && m.deathTimer < 0.0f) {
            m.deathTimer = 0.0f; // start death animation
            // Drop loot
            if (m.type == MOB_PIG) {
                playerHunger += 4.0f;
                if (playerHunger > playerMaxHunger) playerHunger = playerMaxHunger;
                printf("[Mob] Pig dropped food! Hunger restored.\n");
            } else if (m.type == MOB_CHICKEN) {
                playerHunger += 2.0f;
                if (playerHunger > playerMaxHunger) playerHunger = playerMaxHunger;
                printf("[Mob] Chicken dropped food! Hunger restored.\n");
            } else if (m.type == MOB_SHEEP) {
                // Drop wool (white wool block)
                int sc, sr;
                worldToColRow(m.pos.x, m.pos.z, sc, sr);
                int h = (int)(m.pos.y / HEX_HEIGHT);
                spawnItemDrop(sc, sr, h, BLOCK_WOOL_WHITE);
                printf("[Mob] Sheep dropped wool!\n");
            } else if (m.type == MOB_ZOMBIE) {
                // Drop rotten flesh (represented as dirt for now)
                int sc, sr;
                worldToColRow(m.pos.x, m.pos.z, sc, sr);
                int h = (int)(m.pos.y / HEX_HEIGHT);
                spawnItemDrop(sc, sr, h, BLOCK_DIRT);
                printf("[Mob] Zombie dropped rotten flesh!\n");
            } else if (m.type == MOB_SKELETON) {
                // Drop bones (sticks) and arrows (sticks)
                int sc, sr;
                worldToColRow(m.pos.x, m.pos.z, sc, sr);
                int h = (int)(m.pos.y / HEX_HEIGHT);
                spawnItemDrop(sc, sr, h, BLOCK_STONE); // bones proxy
                printf("[Mob] Skeleton dropped bones!\n");
            }
        }
    }
}

// 15C: Combat — left-click on mob to attack (with cooldown)
void attackMob(glm::vec3 camOrigin, glm::vec3 camDir) {
    // Check player attack cooldown
    if (playerAttackCooldown > 0.0f) return;

    float bestDist = 4.0f; // melee range
    int bestIdx = -1;
    for (int i = 0; i < (int)mobs.size(); i++) {
        if (!mobs[i].alive || mobs[i].deathTimer >= 0.0f) continue;
        glm::vec3 toMob = mobs[i].pos + glm::vec3(0, 0.5f, 0) - camOrigin;
        float d = glm::length(toMob);
        if (d > bestDist) continue;
        float dot = glm::dot(glm::normalize(toMob), camDir);
        if (dot > 0.85f && d < bestDist) {
            bestDist = d;
            bestIdx = i;
        }
    }
    if (bestIdx >= 0) {
        int heldType = playerInventory[hotbarSlot].type;
        float damage = getToolDamage(heldType);
        mobs[bestIdx].health -= damage;
        mobs[bestIdx].hitFlash = 0.3f; // red tint for 0.3s

        // Player attack cooldown (swords: 0.625s, axes: 1.0s, others: 0.4s)
        if (isSword(heldType)) playerAttackCooldown = 0.625f;
        else if (isAxe(heldType)) playerAttackCooldown = 1.0f;
        else playerAttackCooldown = 0.4f;

        // Knockback
        float kbStr = isSword(heldType) ? 2.0f : 1.5f;
        glm::vec3 kb = glm::normalize(mobs[bestIdx].pos - camOrigin);
        kb.y = 0.2f;
        mobs[bestIdx].pos += kb * kbStr;

        printf("[Combat] Hit %s with %s for %.0f damage! Health: %.0f\n",
            getMobName(mobs[bestIdx].type),
            getBlockName(heldType),
            damage,
            mobs[bestIdx].health);

        // Degrade tool durability on hit
        InventorySlot& held = playerInventory[hotbarSlot];
        if (held.durability > 0) {
            held.durability--;
            if (held.durability <= 0) {
                printf("[Tool] %s broke!\n", getBlockName(held.type));
                held = {BLOCK_AIR, 0, -1};
            }
        }

        // Passive mobs flee when attacked
        if (mobs[bestIdx].type == MOB_CHICKEN || mobs[bestIdx].type == MOB_PIG ||
            mobs[bestIdx].type == MOB_SHEEP) {
            mobs[bestIdx].state = MOB_FLEE;
            mobs[bestIdx].stateTimer = 0.0f;
            // Alert nearby passive mobs too
            for (auto& m2 : mobs) {
                if (&m2 == &mobs[bestIdx]) continue;
                if (m2.type == MOB_CHICKEN || m2.type == MOB_PIG || m2.type == MOB_SHEEP) {
                    if (glm::length(m2.pos - mobs[bestIdx].pos) < 8.0f) {
                        m2.state = MOB_FLEE;
                        m2.stateTimer = 0.0f;
                    }
                }
            }
        }

        if (mobs[bestIdx].health <= 0.0f) {
            mobs[bestIdx].alive = false;
            printf("[Combat] Killed!\n");
        }
    }
}

// =====================================================
// Birds — init, update, draw
// =====================================================
void initBirds() {
    birds.clear();
    srand(12345);
    for (int i = 0; i < 6; i++) {
        Bird b;
        b.pos = glm::vec3(
            (rand() % 100 - 50) * 0.5f,
            15.0f + (rand() % 10),
            (rand() % 100 - 50) * 0.5f
        );
        // Generate random waypoints
        for (int w = 0; w < 6; w++) {
            b.waypoints[w] = glm::vec3(
                (rand() % 120 - 60) * 0.5f,
                12.0f + (rand() % 8),
                (rand() % 120 - 60) * 0.5f
            );
        }
        b.currentWP = 0;
        b.t = 0.0f;
        b.wingPhase = (float)(rand() % 1000) / 1000.0f * 6.28f;
        b.speed = 3.0f + (rand() % 20) * 0.1f;
        b.color = glm::vec3(0.15f + (rand()%30)*0.01f, 0.12f + (rand()%20)*0.01f, 0.1f + (rand()%15)*0.01f);
        birds.push_back(b);
    }
    printf("[Birds] Spawned %d birds\n", (int)birds.size());
}

void updateBirds(float dt, float time) {
    for (auto& b : birds) {
        b.t += b.speed * dt * 0.3f;
        if (b.t >= 1.0f) {
            b.t -= 1.0f;
            b.currentWP = (b.currentWP + 1) % 4; // cycle through waypoints
            // Regenerate far waypoints for variety
            int futureWP = (b.currentWP + 3) % 6;
            b.waypoints[futureWP] = glm::vec3(
                (rand() % 120 - 60) * 0.5f,
                12.0f + (rand() % 8),
                (rand() % 120 - 60) * 0.5f
            );
        }
        // Catmull-Rom spline interpolation between waypoints
        int i0 = b.currentWP % 6;
        int i1 = (b.currentWP + 1) % 6;
        int i2 = (b.currentWP + 2) % 6;
        int i3 = (b.currentWP + 3) % 6;
        b.pos = catmullRom(b.waypoints[i0], b.waypoints[i1], b.waypoints[i2], b.waypoints[i3], b.t);
        b.wingPhase += dt * 12.0f; // wing flap speed
    }
}

void drawBird(const Bird& b, float time) {
    glm::vec3 p = b.pos;
    // Body
    drawHex(p, b.color, glm::vec3(0.15f, 0.1f, 0.3f));
    // Head
    drawHex(p + glm::vec3(0.0f, 0.05f, -0.25f), b.color * 1.1f, glm::vec3(0.08f, 0.08f, 0.08f));
    // Wings — flap using sin(wingPhase), rotate with myRotate
    float wingAngle = sinf(b.wingPhase) * 0.7f; // radians
    glm::vec3 wingColor = b.color + glm::vec3(0.1f, 0.1f, 0.15f);
    // Left wing
    glm::mat4 modelL = glm::translate(glm::mat4(1.0f), p + glm::vec3(-0.15f, 0.0f, 0.0f));
    modelL = myRotate(modelL, wingAngle, glm::vec3(0, 0, 1));
    modelL = glm::translate(modelL, glm::vec3(-0.15f, 0, 0));
    modelL = glm::scale(modelL, glm::vec3(0.25f, 0.03f, 0.15f));
    drawHexModel(modelL, wingColor);
    // Right wing
    glm::mat4 modelR = glm::translate(glm::mat4(1.0f), p + glm::vec3(0.15f, 0.0f, 0.0f));
    modelR = myRotate(modelR, -wingAngle, glm::vec3(0, 0, 1));
    modelR = glm::translate(modelR, glm::vec3(0.15f, 0, 0));
    modelR = glm::scale(modelR, glm::vec3(0.25f, 0.03f, 0.15f));
    drawHexModel(modelR, wingColor);
}

// =====================================================
// Window — frame + two panes that swing open
// =====================================================
void drawWindow(glm::vec3 pos, float angle, float facing = 0.0f) {
    glm::vec3 frame(0.35f, 0.3f, 0.2f); // brown wood
    glm::vec3 glass(0.5f, 0.7f, 0.9f);  // light blue glass

    // Frame (4 sides)
    drawHex(pos + glm::vec3(0, 0.6f, 0), frame, glm::vec3(0.7f, 0.05f, 0.05f));  // top
    drawHex(pos + glm::vec3(0, -0.6f, 0), frame, glm::vec3(0.7f, 0.05f, 0.05f)); // bottom
    drawHex(pos + glm::vec3(-0.35f, 0, 0), frame, glm::vec3(0.05f, 1.2f, 0.05f)); // left
    drawHex(pos + glm::vec3(0.35f, 0, 0), frame, glm::vec3(0.05f, 1.2f, 0.05f));  // right
    drawHex(pos, frame, glm::vec3(0.05f, 1.2f, 0.05f)); // center divider

    // Left pane (pivots on left frame edge using myRotate)
    float rad = glm::radians(angle);
    glm::mat4 leftPane = glm::translate(glm::mat4(1.0f), pos + glm::vec3(-0.17f, 0, 0));
    leftPane = myRotate(leftPane, glm::radians(facing), glm::vec3(0, 1, 0));
    leftPane = myRotate(leftPane, -rad, glm::vec3(0, 1, 0)); // swing open
    leftPane = glm::translate(leftPane, glm::vec3(-0.085f, 0, 0));
    leftPane = glm::scale(leftPane, glm::vec3(0.15f, 1.0f, 0.02f));
    // Semi-transparent glass
    setFloat(shaderProgram, "alpha", 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawHexModel(leftPane, glass);

    // Right pane (pivots on right frame edge)
    glm::mat4 rightPane = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0.17f, 0, 0));
    rightPane = myRotate(rightPane, glm::radians(facing), glm::vec3(0, 1, 0));
    rightPane = myRotate(rightPane, rad, glm::vec3(0, 1, 0)); // swing open other way
    rightPane = glm::translate(rightPane, glm::vec3(0.085f, 0, 0));
    rightPane = glm::scale(rightPane, glm::vec3(0.15f, 1.0f, 0.02f));
    drawHexModel(rightPane, glass);
    glDisable(GL_BLEND);
    setFloat(shaderProgram, "alpha", 1.0f);
}

// =====================================================
// Fractal Tree — recursive branching
// =====================================================
void drawFractalBranch(glm::vec3 pos, glm::vec3 dir, float length, float thickness, int depth, int maxDepth) {
    if (depth > maxDepth || length < 0.05f) return;

    // Draw this branch segment as a hex
    glm::vec3 endPos = pos + dir * length;
    glm::vec3 midPos = (pos + endPos) * 0.5f;

    // Branch color: brown for trunk, green for leaves
    glm::vec3 color = (depth < maxDepth - 1) ?
        glm::vec3(0.35f, 0.22f, 0.1f) : // wood
        glm::vec3(0.1f + (float)(depth % 3) * 0.1f, 0.5f + (float)(depth % 2) * 0.2f, 0.15f); // leaf green

    drawHex(midPos, color, glm::vec3(thickness, length * 0.5f, thickness));

    // At leaf nodes, draw a cluster of leaf hexes
    if (depth >= maxDepth - 1) {
        glm::vec3 leafGreen(0.2f, 0.55f, 0.15f);
        drawHex(endPos, leafGreen, glm::vec3(thickness * 3.0f, thickness * 1.5f, thickness * 3.0f));
        return;
    }

    // Branch into 2-3 sub-branches using myRotate
    float branchAngle = 0.45f + 0.1f * (float)(depth % 3); // radians
    float childLen = length * 0.65f;
    float childThick = thickness * 0.6f;

    // Main continuation (slightly deviated)
    glm::vec3 up(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(dir, up));
    if (glm::length(right) < 0.01f) right = glm::vec3(1, 0, 0);
    glm::vec3 nup = glm::normalize(glm::cross(right, dir));

    // Use myRotate to rotate direction vectors for branches
    glm::mat4 rot1 = myRotate(glm::mat4(1.0f), branchAngle, right);
    glm::vec3 d1 = glm::vec3(rot1 * glm::vec4(dir, 0.0f));
    drawFractalBranch(endPos, glm::normalize(d1), childLen, childThick, depth + 1, maxDepth);

    glm::mat4 rot2 = myRotate(glm::mat4(1.0f), -branchAngle, right);
    glm::vec3 d2 = glm::vec3(rot2 * glm::vec4(dir, 0.0f));
    drawFractalBranch(endPos, glm::normalize(d2), childLen, childThick, depth + 1, maxDepth);

    // Third branch (angled in perpendicular direction)
    if (depth < 2) {
        glm::mat4 rot3 = myRotate(glm::mat4(1.0f), branchAngle * 0.8f, nup);
        glm::vec3 d3 = glm::vec3(rot3 * glm::vec4(dir, 0.0f));
        drawFractalBranch(endPos, glm::normalize(d3), childLen * 0.8f, childThick, depth + 1, maxDepth);
    }
}

void drawFractalTree(glm::vec3 base) {
    drawFractalBranch(base, glm::vec3(0, 1, 0), 1.5f, 0.15f, 0, 3);
}

// =====================================================
// Render all scene objects
// =====================================================
// Helper: check if a world position is within render range of camera
bool nearCamera(glm::vec3 objPos, float range = 45.0f) {
    glm::vec3 d = objPos - camPos;
    return (d.x*d.x + d.z*d.z) < range * range;
}

void renderObjects(float time) {
    // Player — draw at actual player position (only in third-person)
    if (cameraMode == 0) {
        drawPlayer(playerWorldPos, time, playerYaw, playerWalking, playerWalkTime);
    }

    // Fan — inside castle keep (great hall area)
    int fCol = -15, fRow = 5;
    glm::vec3 fanPos = hexGridPos(fCol, fRow, 0.0f);
    fanPos.y = getGroundY(fCol, fRow);
    if (nearCamera(fanPos)) drawFan(fanPos);

    // Door — castle keep entrance doorway
    int dCol = -11, dRow = 13;
    glm::vec3 doorPos = hexGridPos(dCol, dRow, 0.0f);
    doorPos.y = getGroundY(dCol, dRow);
    drawDoor(doorPos);

    // Windows — on castle walls (P key toggles)
    {
        int wCol1 = -24, wRow1 = 6;
        glm::vec3 wPos1 = hexGridPos(wCol1, wRow1, 0.0f);
        wPos1.y = getGroundY(wCol1, wRow1) + 1.5f;
        drawWindow(wPos1, windowAngle, 0.0f);

        int wCol2 = 3, wRow2 = 6;
        glm::vec3 wPos2 = hexGridPos(wCol2, wRow2, 0.0f);
        wPos2.y = getGroundY(wCol2, wRow2) + 1.5f;
        drawWindow(wPos2, windowAngle, 180.0f);

        int wCol3 = -10, wRow3 = -1;
        glm::vec3 wPos3 = hexGridPos(wCol3, wRow3, 0.0f);
        wPos3.y = getGroundY(wCol3, wRow3) + 1.5f;
        drawWindow(wPos3, windowAngle, 90.0f);
    }

    // Clock — in castle courtyard on a pillar
    int cCol = -5, cRow = 20;
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

    // --- Curvy Objects (with textures applied, distance-culled) ---

    // Sphere — crystal ball with BLENDED grass texture
    glm::vec3 sphBase = hexGridPos(-20, 8, 0.0f);
    sphBase.y = getGroundY(-20, 8);
    if (nearCamera(sphBase)) {
        drawHex(sphBase + glm::vec3(0, 0.5f, 0), COL_STONE, glm::vec3(0.4f, 1.0f, 0.4f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texGrass);
        setInt(shaderProgram, "texture1", 0);
        setInt(shaderProgram, "textureMode", 2);
        drawSphere(sphBase + glm::vec3(0, 1.8f, 0), 0.6f, glm::vec3(0.3f, 0.8f, 1.0f));
        setInt(shaderProgram, "textureMode", 0);
    }

    // Cone — tent with SIMPLE wood texture
    glm::vec3 coneBase = hexGridPos(12, 5, 0.0f);
    coneBase.y = getGroundY(12, 5);
    if (nearCamera(coneBase)) {
        glBindTexture(GL_TEXTURE_2D, texWood);
        setInt(shaderProgram, "textureMode", 1);
        drawCone(coneBase + glm::vec3(0, 0.1f, 0), 1.5f, glm::vec3(0.85f, 0.55f, 0.25f));
        setInt(shaderProgram, "textureMode", 0);
    }

    // Brick wall panel
    {
        glm::vec3 bwBase = hexGridPos(-12, 15, 0.0f);
        bwBase.y = getGroundY(-12, 15);
        if (nearCamera(bwBase)) {
            glBindTexture(GL_TEXTURE_2D, texBrick);
            setInt(shaderProgram, "textureMode", 1);
            drawHex(bwBase + glm::vec3(0, 1.0f, 0), glm::vec3(0.7f), glm::vec3(2.0f, 2.0f, 0.15f));
            setInt(shaderProgram, "textureMode", 0);
        }
    }

    // Wine glass — surface of revolution on castle table
    {
        glm::vec3 wgBase = hexGridPos(-8, 5, 0.0f);
        wgBase.y = getGroundY(-8, 5);
        if (nearCamera(wgBase)) {
            drawHex(wgBase + glm::vec3(0, 0.5f, 0), glm::vec3(0.4f, 0.25f, 0.15f), glm::vec3(0.6f, 1.0f, 0.6f));
            drawHex(wgBase + glm::vec3(0, 1.1f, 0), glm::vec3(0.5f, 0.3f, 0.18f), glm::vec3(1.0f, 0.1f, 1.0f));
            setFloat(shaderProgram, "alpha", 0.6f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            drawWineGlass(wgBase + glm::vec3(0, 1.2f, 0), 0.5f, glm::vec3(0.85f, 0.92f, 0.95f));
            drawWineGlass(wgBase + glm::vec3(0.6f, 1.2f, 0.3f), 0.4f, glm::vec3(0.9f, 0.85f, 0.95f));
            glDisable(GL_BLEND);
            setFloat(shaderProgram, "alpha", 1.0f);
        }
    }

    // Bezier curve — decorative arch near ruins
    glm::vec3 bezBase = hexGridPos(57, -12, 0.0f);
    bezBase.y = getGroundY(57, -12);
    if (nearCamera(bezBase)) {
        drawBezier(bezBase + glm::vec3(0, 0.5f, 0), 2.0f, glm::vec3(0.7f, 0.5f, 0.9f));
    }

    // Spline — winding fence along path to castle
    glm::vec3 splBase = hexGridPos(-5, 8, 0.0f);
    splBase.y = getGroundY(-5, 8);
    if (nearCamera(splBase)) {
        drawSpline(splBase + glm::vec3(0, 0.3f, 0), 2.5f, glm::vec3(0.6f, 0.4f, 0.2f));
    }

    // Ruled surface — ramp outside castle west wall
    glm::vec3 rmpBase = hexGridPos(-25, 30, 0.0f);
    rmpBase.y = getGroundY(-25, 30);
    if (nearCamera(rmpBase)) {
        drawRuledSurface(rmpBase + glm::vec3(0, 0.3f, 0), 2.0f, glm::vec3(0.5f, 0.7f, 0.5f));
    }

    // --- Decorative: Grass Tufts (textured with texGrass, blended) ---
    {
        static const int gPos[][2] = {
            {5,3},{7,4},{2,6},{10,2},{6,8},{-3,5},{4,10},{8,1},{1,8},{-5,2},
            {12,6},{-1,4},{3,9},{9,-1},{0,7},{-4,9},{11,4},{7,-2},{-6,6},{5,12}
        };
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texGrass);
        setInt(shaderProgram, "texture1", 0);
        setInt(shaderProgram, "textureMode", 2);
        for (auto& gp : gPos) {
            glm::vec3 base = hexGridPos(gp[0], gp[1], 0.0f);
            base.y = getGroundY(gp[0], gp[1]) + HEX_HEIGHT * 0.52f;
            if (!nearCamera(base)) continue;
            unsigned int s = gridSeed(gp[0], gp[1]);
            float sz = 0.45f + (s & 0xF) * 0.04f;
            glm::vec3 gc = COL_GRASS * (0.75f + (s & 7) * 0.035f);
            // Main tuft
            drawHex(base, gc, glm::vec3(sz, 0.05f, sz));
            // Second smaller tuft offset
            drawHex(base + glm::vec3(0.22f, 0.02f, 0.15f), gc * 0.9f, glm::vec3(sz * 0.65f, 0.05f, sz * 0.65f));
        }
        setInt(shaderProgram, "textureMode", 0);
    }

    // --- Decorative: Stone Boulders (textured with texBrick as stone) ---
    {
        static const int bPos[][2] = {
            {15,5},{-8,12},{20,-3},{-18,8},{12,-10},
            {25,15},{-5,-8},{30,5},{18,-8},{-22,3}
        };
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texBrick);
        setInt(shaderProgram, "texture1", 0);
        setInt(shaderProgram, "textureMode", 1);
        for (auto& bp : bPos) {
            glm::vec3 base = hexGridPos(bp[0], bp[1], 0.0f);
            base.y = getGroundY(bp[0], bp[1]);
            if (!nearCamera(base)) continue;
            unsigned int s = gridSeed(bp[0], bp[1]);
            float w = 0.5f + (s & 0xF) * 0.05f;
            float h = 0.3f + ((s >> 4) & 0xF) * 0.04f;
            glm::vec3 sc = COL_STONE * (0.75f + (s & 7) * 0.04f);
            // Main boulder
            drawHex(base + glm::vec3(0, h * HEX_HEIGHT * 0.5f, 0), sc, glm::vec3(w, h, w));
            // Smaller rock beside it
            float w2 = w * 0.45f;
            drawHex(base + glm::vec3(0.38f, w2 * HEX_HEIGHT * 0.3f, 0.22f), sc * 0.88f, glm::vec3(w2, w2 * 0.6f, w2));
            // Pebble
            float w3 = w * 0.2f;
            drawHex(base + glm::vec3(-0.3f, w3 * HEX_HEIGHT * 0.3f, 0.3f), sc * 0.82f, glm::vec3(w3, w3 * 0.5f, w3));
        }
        setInt(shaderProgram, "textureMode", 0);
    }

    // --- Decorative: Flowers (color only, no texture) ---
    {
        struct FlowerSpec { int col, row; glm::vec3 color; };
        static const FlowerSpec flowers[] = {
            {3,4,  glm::vec3(0.9f,0.2f,0.2f)},  // red
            {6,3,  glm::vec3(0.9f,0.8f,0.1f)},  // yellow
            {-2,7, glm::vec3(0.7f,0.2f,0.8f)},  // purple
            {9,6,  glm::vec3(1.0f,0.4f,0.0f)},  // orange
            {4,-2, glm::vec3(1.0f,0.75f,0.8f)}, // pink
            {-7,3, glm::vec3(0.4f,0.6f,1.0f)},  // blue
            {11,9, glm::vec3(1.0f,0.3f,0.4f)},  // rose
            {-3,11,glm::vec3(0.9f,0.9f,0.3f)},  // sunflower yellow
            {13,3, glm::vec3(0.5f,0.2f,0.9f)},  // violet
        };
        setInt(shaderProgram, "textureMode", 0);
        for (auto& f : flowers) {
            glm::vec3 base = hexGridPos(f.col, f.row, 0.0f);
            base.y = getGroundY(f.col, f.row);
            if (!nearCamera(base)) continue;
            // Stem
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.3f, 0),
                    glm::vec3(0.2f, 0.65f, 0.15f), glm::vec3(0.07f, 0.6f, 0.07f));
            // Petal cap
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.72f, 0),
                    f.color, glm::vec3(0.35f, 0.04f, 0.35f));
            // Small centre dot (slightly lighter)
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.74f, 0),
                    f.color * 1.3f + glm::vec3(0.1f), glm::vec3(0.12f, 0.025f, 0.12f));
        }
    }

    // --- Decorative: Mushrooms ---
    {
        static const int mPos[][2] = {
            {-10,6},{14,12},{-20,-5},{8,18},{-12,18},{22,8}
        };
        setInt(shaderProgram, "textureMode", 0);
        for (auto& mp : mPos) {
            glm::vec3 base = hexGridPos(mp[0], mp[1], 0.0f);
            base.y = getGroundY(mp[0], mp[1]);
            if (!nearCamera(base)) continue;
            unsigned int s = gridSeed(mp[0], mp[1]);
            bool redMush = (s & 1);
            glm::vec3 capCol = redMush
                ? glm::vec3(0.75f, 0.12f, 0.08f)
                : glm::vec3(0.55f, 0.38f, 0.12f);
            float scale = 0.7f + (s & 7) * 0.07f;
            // Stalk
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.22f * scale, 0),
                    glm::vec3(0.85f, 0.82f, 0.78f), glm::vec3(0.12f * scale, 0.45f * scale, 0.12f * scale));
            // Cap
            drawHex(base + glm::vec3(0, HEX_HEIGHT * 0.58f * scale, 0),
                    capCol, glm::vec3(0.52f * scale, 0.07f * scale, 0.52f * scale));
            // White spots on red mushroom
            if (redMush) {
                drawHex(base + glm::vec3(0.09f, HEX_HEIGHT * 0.61f * scale, 0.05f),
                        glm::vec3(0.95f, 0.95f, 0.9f), glm::vec3(0.09f, 0.02f, 0.09f));
                drawHex(base + glm::vec3(-0.06f, HEX_HEIGHT * 0.60f * scale, 0.1f),
                        glm::vec3(0.95f, 0.95f, 0.9f), glm::vec3(0.06f, 0.015f, 0.06f));
            }
        }
    }

    // Fractal Trees — distance-culled
    {
        glm::vec3 ftBase1 = hexGridPos(20, 15, 0.0f); ftBase1.y = getGroundY(20, 15);
        if (nearCamera(ftBase1, 35.0f)) drawFractalTree(ftBase1);

        glm::vec3 ftBase2 = hexGridPos(-15, 25, 0.0f); ftBase2.y = getGroundY(-15, 25);
        if (nearCamera(ftBase2, 35.0f)) drawFractalTree(ftBase2);

        glm::vec3 ftBase3 = hexGridPos(30, -20, 0.0f); ftBase3.y = getGroundY(30, -20);
        if (nearCamera(ftBase3, 35.0f)) drawFractalTree(ftBase3);
    }

    // --- Flying Birds ---
    for (auto& b : birds) {
        if (nearCamera(b.pos)) drawBird(b, time);
    }

    // --- Mobs (render alive + dying mobs) ---
    for (auto& m : mobs) {
        if ((m.alive || m.deathTimer >= 0.0f) && nearCamera(m.pos)) drawMob(m, time);
    }

    // --- Arrow projectiles ---
    drawArrows();

    // --- Item drops: update physics, render, pickup ---
    for (int i = (int)itemDrops.size() - 1; i >= 0; i--) {
        ItemDrop& d = itemDrops[i];
        d.lifetime += deltaTime;

        // Despawn
        if (d.lifetime > ITEM_DESPAWN_TIME) {
            itemDrops.erase(itemDrops.begin() + i);
            continue;
        }

        // Physics: gravity + bounce
        d.vel.y -= 10.0f * deltaTime;
        d.pos += d.vel * deltaTime;

        // Ground collision for drop
        int dc, dr;
        worldToColRow(d.pos.x, d.pos.z, dc, dr);
        float groundY = getGroundYWorld(d.pos.x, d.pos.z);
        if (d.pos.y < groundY + 0.3f) {
            d.pos.y = groundY + 0.3f;
            d.vel = glm::vec3(0.0f); // stop bouncing
        }

        // Bobbing + spinning animation
        float bob = sinf(time * 2.0f + d.bobPhase) * 0.15f;
        float spin = time * 2.0f + d.bobPhase;
        glm::vec3 drawPos = d.pos + glm::vec3(0, bob, 0);

        // Draw as small rotating hex
        glm::vec3 color = getBlockColor(d.type);
        drawHexRotated(drawPos, color, spin, glm::vec3(0, 1, 0), glm::vec3(0.3f, 0.3f, 0.3f));

        // Pickup: check distance to player
        float dist = glm::length(d.pos - playerWorldPos);
        if (dist < ITEM_PICKUP_RADIUS) {
            bool pickedUp = false;
            // 1. Try to stack in existing slot
            for (int j = 0; j < 36; j++) {
                if (playerInventory[j].type == d.type && playerInventory[j].count < 64) {
                    playerInventory[j].count++;
                    pickedUp = true;
                    break;
                }
            }
            // 2. Try empty slot
            if (!pickedUp) {
                for (int j = 0; j < 36; j++) {
                    if (playerInventory[j].type == BLOCK_AIR) {
                        playerInventory[j].type = d.type;
                        playerInventory[j].count = 1;
                        pickedUp = true;
                        break;
                    }
                }
            }
            if (pickedUp) {
                printf("[Pickup] Got %d\n", d.type);
                itemDrops.erase(itemDrops.begin() + i);
            }
        }
    }

    // --- Break particles: update physics + render ---
    for (int i = (int)breakParticles.size() - 1; i >= 0; i--) {
        BreakParticle& bp = breakParticles[i];
        bp.lifetime += deltaTime;
        if (bp.lifetime >= bp.maxLifetime) {
            breakParticles.erase(breakParticles.begin() + i);
            continue;
        }
        // Physics: gravity pulls them down, no bounce
        bp.vel.y -= 14.0f * deltaTime;
        bp.pos   += bp.vel * deltaTime;
        bp.spin  += bp.spinSpeed * deltaTime;

        // Fade out in last 20% of lifetime
        float t = bp.lifetime / bp.maxLifetime;
        float alpha = (t > 0.8f) ? (1.0f - (t - 0.8f) / 0.2f) : 1.0f;

        // Draw as tiny rotating hex chip
        float sz = 0.07f * (1.0f - t * 0.4f); // shrinks slightly
        glm::vec3 col = bp.color * alpha;
        drawHexRotated(bp.pos, col, bp.spin, glm::vec3(0.577f, 0.577f, 0.577f),
                       glm::vec3(sz, sz * 0.5f, sz));
    }
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
void processInput(GLFWwindow* window) {
    // Block all movement/gameplay input while inventory is open
    if (inventoryOpen) return;

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
        // Q key: hold to fly up, release to fall (gravity)
        bool flying = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) && !playerDead;

        // Movement direction
        float moveYaw = glm::radians(camYaw);

        // Sprinting (Shift held + moving forward + has stamina)
        playerSprinting = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                          && playerStamina > 0.0f && !playerDead;
        float speedMult = playerSprinting ? 1.5f : 1.0f;
        float pSpeed = 4.5f * speedMult * deltaTime;

        if (flying) {
            // --- Flying mode: WASD moves in full 3D camera direction ---
            playerOnGround = false;

            glm::vec3 flyForward = camFront; // full 3D direction (where you look)
            glm::vec3 flyRight = glm::normalize(glm::cross(camFront, glm::vec3(0, 1, 0)));

            glm::vec3 moveDir(0.0f);
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { moveDir += flyForward; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { moveDir -= flyForward; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { moveDir -= flyRight; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { moveDir += flyRight; playerWalking = true; }

            if (glm::length(moveDir) > 0.001f) {
                moveDir = glm::normalize(moveDir);
                playerWorldPos += moveDir * pSpeed * 1.5f; // slightly faster in flight
                playerYaw = atan2f(moveDir.z, moveDir.x);
            }

            // Q held: fly upward steadily
            playerVelY = 5.0f;

        } else {
            // --- Ground mode: WASD moves horizontally only ---
            glm::vec3 forward(cosf(moveYaw), 0.0f, sinf(moveYaw));
            glm::vec3 right(-sinf(moveYaw), 0.0f, cosf(moveYaw));

            glm::vec3 moveDir(0.0f);
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { moveDir += forward; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { moveDir -= forward; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { moveDir -= right; playerWalking = true; }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { moveDir += right; playerWalking = true; }

            if (glm::length(moveDir) > 0.001f) {
                moveDir = glm::normalize(moveDir);
                float newX = playerWorldPos.x + moveDir.x * pSpeed;
                float newZ = playerWorldPos.z + moveDir.z * pSpeed;
                // Horizontal collision with auto-step (climb 1-block ledges)
                // Try full move first, then axis-separated (wall sliding)
                if (!tryMoveWithStep(playerWorldPos.x, playerWorldPos.y, playerWorldPos.z, newX, newZ)) {
                    // Wall sliding: try X-only, then Z-only
                    if (!tryMoveWithStep(playerWorldPos.x, playerWorldPos.y, playerWorldPos.z, newX, playerWorldPos.z)) {
                        tryMoveWithStep(playerWorldPos.x, playerWorldPos.y, playerWorldPos.z, playerWorldPos.x, newZ);
                    }
                }
                // Face movement direction
                playerYaw = atan2f(moveDir.z, moveDir.x);
            }

            // Jump
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && playerOnGround && !playerDead) {
                playerVelY = 6.0f;
                playerOnGround = false;
            }
        }

        // E/R for vertical fly (alternative controls, override gravity)
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            playerVelY = 8.0f;
            playerOnGround = false;
            flying = true;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            playerVelY = -6.0f;
            flying = true;
        }

        // Ladder climbing: detect if player is overlapping a ladder block
        {
            int lCol, lRow;
            worldToColRow(playerWorldPos.x, playerWorldPos.z, lCol, lRow);
            int lH = (int)floorf(playerWorldPos.y / HEX_HEIGHT);
            int btFeet = getBlock(lCol, lRow, lH);
            int btBody = getBlock(lCol, lRow, lH + 1);
            playerOnLadder = (btFeet == BLOCK_LADDER || btBody == BLOCK_LADDER);
        }
        if (playerOnLadder && !flying) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                playerVelY = 4.0f;
                playerOnGround = false;
            } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                playerVelY = -4.0f;
            } else {
                playerVelY = 0.0f; // hold position on ladder
            }
            flying = true; // suppress gravity while on ladder
        }

        // Track fall start
        if (!playerOnGround && !trackingFall && !flying) {
            fallStartY = playerWorldPos.y;
            trackingFall = true;
        }

        // Gravity (disabled while flying)
        if (!flying) {
            playerVelY -= 15.0f * deltaTime;
        }
        playerWorldPos.y += playerVelY * deltaTime;

        // Ground collision — Y-aware: finds ground below player, not roof above
        {
            int col, row;
            worldToColRow(playerWorldPos.x, playerWorldPos.z, col, row);
            float groundY = getGroundYAtHeight(col, row, playerWorldPos.y);
            if (playerWorldPos.y <= groundY) {
                playerWorldPos.y = groundY;

                // Fall damage: damage if fell more than 3 blocks
                if (trackingFall) {
                    float fallDist = fallStartY - groundY;
                    if (fallDist > 3.0f * HEX_HEIGHT) {
                        float dmg = (fallDist / HEX_HEIGHT) - 3.0f; // 1 damage per block above 3
                        playerHealth -= dmg;
                        if (playerHealth < 0.0f) playerHealth = 0.0f;
                        printf("[Player] Fall damage: %.1f (fell %.1f blocks)\n", dmg, fallDist / HEX_HEIGHT);
                        if (playerHealth <= 0.0f && !playerDead) {
                            playerDead = true;
                            printf("[Player] YOU DIED! Press R to respawn.\n");
                        }
                    }
                    trackingFall = false;
                }

                playerVelY = 0.0f;
                playerOnGround = true;
            }
        }

        // Sprinting drains stamina
        if (playerSprinting && playerWalking) {
            playerStamina -= 15.0f * deltaTime;
            if (playerStamina < 0.0f) playerStamina = 0.0f;
        } else {
            // Stamina regenerates when not sprinting
            playerStamina += 10.0f * deltaTime;
            if (playerStamina > playerMaxStamina) playerStamina = playerMaxStamina;
        }

        // Hunger depletes over time
        hungerTimer += deltaTime;
        float hungerRate = playerSprinting ? 0.3f : 0.1f; // faster when sprinting
        if (hungerTimer >= 5.0f) { // every 5 seconds
            playerHunger -= hungerRate;
            if (playerHunger < 0.0f) playerHunger = 0.0f;
            hungerTimer = 0.0f;
        }

        // Health regen when hunger > 14 (70%)
        if (playerHunger > 14.0f && playerHealth < playerMaxHealth) {
            playerHealth += 0.5f * deltaTime;
            if (playerHealth > playerMaxHealth) playerHealth = playerMaxHealth;
        }
        // Starving: lose health when hunger is 0
        if (playerHunger <= 0.0f) {
            playerHealth -= 0.5f * deltaTime;
            if (playerHealth < 0.0f) playerHealth = 0.0f;
            if (playerHealth <= 0.0f && !playerDead) {
                playerDead = true;
                printf("[Player] YOU DIED from starvation! Press R to respawn.\n");
            }
        }

        // Walk animation timer
        if (playerWalking) {
            playerWalkTime += deltaTime * 6.0f;
        }

        // Update camera to follow player (Minecraft-style)
        // Eye height: 1.62 blocks from feet (Minecraft standard)
        float eyeHeight = 1.62f;
        if (cameraMode == 0) {
            // Third-person: camera behind and above player's head
            float camYawRad = glm::radians(camYaw);
            float camPitchRad = glm::radians(camPitch);
            glm::vec3 offset;
            offset.x = -cosf(camYawRad) * cosf(camPitchRad) * thirdPersonDist;
            offset.y = -sinf(camPitchRad) * thirdPersonDist + thirdPersonHeight;
            offset.z = -sinf(camYawRad) * cosf(camPitchRad) * thirdPersonDist;
            camPos = playerWorldPos + glm::vec3(0, eyeHeight, 0) + offset;
        } else {
            // First-person: camera at player eye level
            camPos = playerWorldPos + glm::vec3(0, eyeHeight, 0);
        }

        // E/R fly is now handled above with proper gravity override
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

    // --- Hold-to-break tick ---
    static float breakDebugTimer = 0.0f;
    breakDebugTimer -= deltaTime;
    if (isBreaking && breakDebugTimer <= 0.0f) {
        breakDebugTimer = 0.5f;
        printf("[Break] isBreaking=%d hasTarget=%d inventoryOpen=%d target=(%d,%d,%d) hand=%s\n",
               (int)isBreaking, (int)hasTarget, (int)inventoryOpen,
               targetCol, targetRow, targetHeight,
               getBlockName(playerInventory[hotbarSlot].type));
        if (hasTarget) {
            int _dbt = getBlock(targetCol, targetRow, targetHeight);
            printf("[Break]   block=%d hardness=%.2f speed=%.2f holdTime=%.3f\n",
                   _dbt, getBlockHardness(_dbt),
                   getToolSpeedMultiplier(playerInventory[hotbarSlot].type, _dbt),
                   breakHoldTime);
        }
    }
    if (isBreaking && hasTarget && !inventoryOpen) {
        // If player looked away to a different block, reset progress
        if (targetCol != breakTargetCol || targetRow != breakTargetRow || targetHeight != breakTargetH) {
            breakHoldTime = 0.0f;
            breakTargetCol = targetCol;
            breakTargetRow = targetRow;
            breakTargetH   = targetHeight;
        }

        int bt = getBlock(targetCol, targetRow, targetHeight);
        float hardness = getBlockHardness(bt);

        if (bt == BLOCK_AIR) {
            // Nothing to break — reset progress but keep isBreaking so it resumes when crosshair returns to a solid block
            breakHoldTime = 0.0f;
        } else if (hardness < 0.0f) {
            // Unbreakable (bedrock)
        } else {
            int heldType = playerInventory[hotbarSlot].type;
            float speed = getToolSpeedMultiplier(heldType, bt);
            if (speed <= 0.0f) {
                // Can't break (e.g. obsidian without diamond pick)
            } else {
                float duration = hardness / speed;
                breakHoldTime += deltaTime;

                // Emit break particles — throttled to ~12 per second
                breakParticleTimer -= deltaTime;
                if (breakParticleTimer <= 0.0f && (int)breakParticles.size() < MAX_BREAK_PARTICLES) {
                    breakParticleTimer = 0.083f; // ~12 Hz

                    glm::vec3 blockCenter = hexGridPos(targetCol, targetRow, 0.0f);
                    blockCenter.y = targetHeight * HEX_HEIGHT + HEX_HEIGHT * 0.5f;
                    glm::vec3 blockColor = getBlockColor(bt);

                    // Emit 3 chips per burst
                    for (int p = 0; p < 3; p++) {
                        if ((int)breakParticles.size() >= MAX_BREAK_PARTICLES) break;

                        // Random offset on block surface
                        float rx = ((gridSeed(targetCol + p, targetRow + (int)(breakHoldTime * 100)) % 100) - 50) * 0.01f;
                        float ry = ((gridSeed(targetRow + p, targetCol + (int)(breakHoldTime * 137)) % 100) - 50) * 0.01f;
                        float rz = ((gridSeed(targetCol * 3 + p, targetRow + 7) % 100) - 50) * 0.01f;

                        BreakParticle bp;
                        bp.pos = blockCenter + glm::vec3(rx, ry, rz) * 0.5f;
                        // Velocity: burst outward toward camera + random spread
                        glm::vec3 outDir = glm::normalize(camPos - blockCenter);
                        float spreadX = ((gridSeed(targetCol + p * 7, targetRow) % 100) - 50) * 0.06f;
                        float spreadY = ((gridSeed(targetRow + p * 5, targetCol) % 100)) * 0.04f + 1.5f;
                        float spreadZ = ((gridSeed(targetCol, targetRow + p * 11) % 100) - 50) * 0.06f;
                        bp.vel = outDir * 2.5f + glm::vec3(spreadX, spreadY, spreadZ);
                        // Slight color variation (darker/lighter chips)
                        float tint = 0.7f + ((gridSeed(p, targetCol + targetRow) % 30)) * 0.01f;
                        bp.color = blockColor * tint;
                        bp.lifetime = 0.0f;
                        bp.maxLifetime = 0.35f + ((gridSeed(p * 3, targetCol) % 30)) * 0.01f;
                        bp.spin = 0.0f;
                        bp.spinSpeed = ((gridSeed(targetCol + p, targetRow * 2) % 200) - 100) * 0.12f;
                        breakParticles.push_back(bp);
                    }
                }

                if (breakHoldTime >= duration) {
                    breakBlock();
                    // Reset so next block (if still holding) starts fresh
                    breakHoldTime = 0.0f;
                    breakTargetCol = -9999; // force target mismatch next frame
                    breakTargetRow = -9999;
                    breakTargetH   = -9999;
                }
            }
        }
    } else if (!isBreaking) {
        breakHoldTime = 0.0f;
        breakParticleTimer = 0.0f;
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
        return;
    }
    
    if (inventoryOpen) {
        lastMouseX = xpos;
        lastMouseY = ypos;
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
    // Handle right/left-button RELEASE first (before any early returns) so isBreaking always clears
    if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_RELEASE) {
        isBreaking = false;
        // Don't reset breakHoldTime — progress pauses on release, resumes on re-press of same block
        return;
    }

    if (action != GLFW_PRESS) return;

    if (inventoryOpen) {
        if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) {
            bool isLeft = (button == GLFW_MOUSE_BUTTON_LEFT);
            bool isRight = (button == GLFW_MOUSE_BUTTON_RIGHT);
            bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

            int screenW, screenH;
            glfwGetWindowSize(window, &screenW, &screenH);

            // Replicate centered panel layout math from hud.h
            float slotSize = 40.0f;
            float slotGap = 4.0f;
            float slotStep = slotSize + slotGap; // 44
            float totalW = HOTBAR_SIZE * slotSize + (HOTBAR_SIZE - 1) * slotGap; // 392

            float panelPad = 15.0f;
            float sectionGap = 14.0f;
            int craftDim = 3; // always 3x3
            float craftGridW = craftDim * slotStep - slotGap;
            float craftRowW = 3 * slotStep - slotGap;  // kept for recipe book compat
            float arrowW = 30.0f;
            float arrowGapL = 10.0f, arrowGapR = 10.0f;
            float outputW = slotSize;

            float panelW = panelPad + totalW + panelPad;
            float craftAreaH = craftDim * slotStep - slotGap;
            float panelH = panelPad + slotSize + sectionGap + 3 * slotStep - slotGap + sectionGap + craftAreaH + panelPad;
            float panelX = (screenW - panelW) / 2.0f;
            float panelY = (screenH - panelH) / 2.0f;
            float gridX = panelX + panelPad;

            float hotbarY = panelY + panelPad;
            float storageY = hotbarY + slotSize + sectionGap;
            float craftY = storageY + 3 * slotStep - slotGap + sectionGap;
            float craftTotalW = craftGridW + arrowGapL + arrowW + arrowGapR + outputW;
            float craftOffX = gridX + (totalW - craftTotalW) / 2.0f;

            // Mouse Y: GLFW top=0, HUD bottom=0
            float my = screenH - lastMouseY;
            float mx = lastMouseX;

            int clickedSlot = -1;

            // Check hotbar (slots 0-8)
            for (int i = 0; i < HOTBAR_SIZE; i++) {
                float sx = gridX + i * slotStep;
                float sy = hotbarY;
                if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                    clickedSlot = i; break;
                }
            }

            // Check storage grid (slots 9-35)
            if (clickedSlot == -1) {
                for (int r = 0; r < 3; r++) {
                    for (int c = 0; c < 9; c++) {
                        float sx = gridX + c * slotStep;
                        float sy = storageY + (2 - r) * slotStep; // row 0 at top
                        if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                            clickedSlot = 9 + r * 9 + c; break;
                        }
                    }
                    if (clickedSlot != -1) break;
                }
            }

            // Check crafting grid (2x2 or 3x3)
            if (clickedSlot == -1) {
                for (int r = 0; r < craftDim; r++) {
                    for (int c = 0; c < craftDim; c++) {
                        float sx = craftOffX + c * slotStep;
                        float sy = craftY + (craftDim - 1 - r) * slotStep;
                        if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                            clickedSlot = 36 + r * 3 + c; break;
                        }
                    }
                    if (clickedSlot != -1) break;
                }
            }

            // Check crafting output slot (slot 45)
            if (clickedSlot == -1) {
                float arrowX = craftOffX + craftGridW + arrowGapL;
                float outX = arrowX + arrowW + arrowGapR;
                float outY = craftY + (craftDim == 3 ? slotStep : slotSize * 0.5f);
                if (mx >= outX && mx <= outX + slotSize && my >= outY && my <= outY + slotSize) {
                    clickedSlot = 45;
                }
            }

            // Recipe Book toggle button (always available)
            float bookBtnX = craftOffX - 40.0f;
            float bookBtnY = craftY + slotStep;
            if (mx >= bookBtnX && mx <= bookBtnX + 24.0f && my >= bookBtnY && my <= bookBtnY + 24.0f) {
                recipeBookOpen = !recipeBookOpen;
                recipeSearchText = "";
                recipePage = 0;
                printf("[RecipeBook] Toggled: %d\n", recipeBookOpen);
                return;
            }

            // Recipe Book interactions
            if (recipeBookOpen) {
                float rbW = 180.0f;
                float rbH = panelH;
                float rbX = panelX - rbW - 10.0f;
                float rbY = panelY;

                // Build same filtered list as hud.h
                std::vector<int> filtered;
                for (int i = 0; i < NUM_RECIPES; i++) {
                    if (recipeSearchText.empty()) {
                        filtered.push_back(i);
                    } else {
                        std::string name = getBlockName(recipes[i].resultType);
                        bool found = false;
                        if (name.size() >= recipeSearchText.size()) {
                            for (size_t k = 0; k <= name.size() - recipeSearchText.size(); k++) {
                                bool match = true;
                                for (size_t m = 0; m < recipeSearchText.size(); m++) {
                                    if (tolower(name[k+m]) != recipeSearchText[m]) { match = false; break; }
                                }
                                if (match) { found = true; break; }
                            }
                        }
                        if (found) filtered.push_back(i);
                    }
                }
                int totalPages = ((int)filtered.size() + 3) / 4;

                // Pagination buttons
                float btnY = rbY + 15.0f;
                float btnW = 30.0f;
                float btnH = 20.0f;
                float prevX = rbX + 30.0f;
                float nextX = rbX + rbW - 30.0f - btnW;

                if (my >= btnY && my <= btnY + btnH) {
                    if (mx >= prevX && mx <= prevX + btnW) {
                        if (recipePage > 0) recipePage--;
                        return;
                    }
                    if (mx >= nextX && mx <= nextX + btnW) {
                        if (recipePage < totalPages - 1) recipePage++;
                        return;
                    }
                }

                // Recipe row clicking (uses filtered list)
                float sbY = rbY + rbH - 26.0f;
                int startIdx = recipePage * 4;
                float startY = sbY - 18.0f - 60.0f;
                for (int i = 0; i < 4; i++) {
                    int fi = startIdx + i;
                    if (fi >= (int)filtered.size()) break;
                    int rIdx = filtered[fi];
                    float rowY = startY - i * 70.0f;
                    if (rowY < rbY + 40.0f) break;
                    if (mx >= rbX + 10 && mx <= rbX + 10 + rbW - 20 && my >= rowY && my <= rowY + 60) {
                        autoCraftRecipe(rIdx);
                        return;
                    }
                }
            }


            // Handle clicks
            if (clickedSlot >= 0 && clickedSlot <= 35) {
                InventorySlot* target = &playerInventory[clickedSlot];
                if (isShift && isLeft) {
                    if (target->type != BLOCK_AIR) {
                        int startIdx = (clickedSlot < 9) ? 9 : 0;
                        int endIdx = (clickedSlot < 9) ? 36 : 9;
                        for (int k = startIdx; k < endIdx; k++) {
                            if (playerInventory[k].type == target->type && playerInventory[k].count < 64) {
                                int space = 64 - playerInventory[k].count;
                                int move = (target->count < space) ? target->count : space;
                                playerInventory[k].count += move;
                                target->count -= move;
                                if (target->count == 0) { target->type = BLOCK_AIR; break; }
                            }
                        }
                        if (target->count > 0) {
                            for (int k = startIdx; k < endIdx; k++) {
                                if (playerInventory[k].type == BLOCK_AIR) {
                                    playerInventory[k] = *target;
                                    target->type = BLOCK_AIR;
                                    target->count = 0;
                                    break;
                                }
                            }
                        }
                    }
                } else if (isRight) {
                    if (draggedSlot.type == BLOCK_AIR && target->type != BLOCK_AIR) {
                        int half = (target->count + 1) / 2;
                        draggedSlot.type = target->type;
                        draggedSlot.count = half;
                        target->count -= half;
                        if (target->count == 0) target->type = BLOCK_AIR;
                    } else if (draggedSlot.type != BLOCK_AIR) {
                        if (target->type == BLOCK_AIR) {
                            target->type = draggedSlot.type;
                            target->count = 1;
                            draggedSlot.count--;
                            if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                        } else if (target->type == draggedSlot.type && target->count < 64) {
                            target->count++;
                            draggedSlot.count--;
                            if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                        }
                    }
                } else if (isLeft) {
                    if (draggedSlot.type == target->type && draggedSlot.type != BLOCK_AIR && target->count < 64) {
                        int space = 64 - target->count;
                        int move = (draggedSlot.count < space) ? draggedSlot.count : space;
                        target->count += move;
                        draggedSlot.count -= move;
                        if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                    } else {
                        InventorySlot temp = *target;
                        *target = draggedSlot;
                        draggedSlot = temp;
                    }
                }
            } else if (clickedSlot >= 36 && clickedSlot <= 44) {
                int ci = clickedSlot - 36;
                InventorySlot* target = &craftingGrid[ci];
                if (isRight) {
                    if (draggedSlot.type == BLOCK_AIR && target->type != BLOCK_AIR) {
                        int half = (target->count + 1) / 2;
                        draggedSlot.type = target->type;
                        draggedSlot.count = half;
                        target->count -= half;
                        if (target->count == 0) target->type = BLOCK_AIR;
                    } else if (draggedSlot.type != BLOCK_AIR) {
                        if (target->type == BLOCK_AIR || (target->type == draggedSlot.type && target->count < 64)) {
                            target->type = draggedSlot.type;
                            target->count++;
                            draggedSlot.count--;
                            if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                        }
                    }
                } else if (isShift && isLeft) {
                    // Shift-click: move crafting grid item back to inventory
                    if (target->type != BLOCK_AIR) {
                        bool placed = false;
                        for (int k = 0; k < 36; k++) {
                            if (playerInventory[k].type == target->type && playerInventory[k].count + target->count <= 64) {
                                playerInventory[k].count += target->count;
                                placed = true; break;
                            }
                        }
                        if (!placed) {
                            for (int k = 0; k < 36; k++) {
                                if (playerInventory[k].type == BLOCK_AIR) {
                                    playerInventory[k] = *target;
                                    placed = true; break;
                                }
                            }
                        }
                        if (placed) { target->type = BLOCK_AIR; target->count = 0; }
                    }
                } else if (isLeft) {
                    if (draggedSlot.type == target->type && draggedSlot.type != BLOCK_AIR && target->count < 64) {
                        int space = 64 - target->count;
                        int move = (draggedSlot.count < space) ? draggedSlot.count : space;
                        target->count += move;
                        draggedSlot.count -= move;
                        if (draggedSlot.count == 0) draggedSlot.type = BLOCK_AIR;
                    } else {
                        InventorySlot temp = *target;
                        *target = draggedSlot;
                        draggedSlot = temp;
                    }
                }
                checkCraftingRecipes();
            } else if (clickedSlot == 45) {
                if (craftingOutput.type != BLOCK_AIR) {
                    if (isShift && isLeft) {
                        // Mass craft: keep crafting until inputs run out or inventory full
                        int outputType = craftingOutput.type;
                        int crafted = 0;
                        while (craftingOutput.type == outputType && craftingOutput.type != BLOCK_AIR) {
                            bool placed = false;
                            for (int k = 0; k < 36; k++) {
                                if (playerInventory[k].type == craftingOutput.type && playerInventory[k].count + craftingOutput.count <= 64) {
                                    playerInventory[k].count += craftingOutput.count;
                                    placed = true; break;
                                }
                            }
                            if (!placed) {
                                for (int k = 0; k < 36; k++) {
                                    if (playerInventory[k].type == BLOCK_AIR) {
                                        playerInventory[k] = craftingOutput;
                                        placed = true; break;
                                    }
                                }
                            }
                            if (!placed) break; // inventory full
                            consumeCraftingInputs(); // consumes inputs and re-checks recipe
                            crafted++;
                            if (crafted >= 64) break; // safety limit
                        }
                    } else if (isLeft) {
                        if (draggedSlot.type == BLOCK_AIR) {
                            draggedSlot = craftingOutput;
                            consumeCraftingInputs();
                        } else if (draggedSlot.type == craftingOutput.type && draggedSlot.count + craftingOutput.count <= 64) {
                            draggedSlot.count += craftingOutput.count;
                            consumeCraftingInputs();
                        }
                    }
                }
            }
        }
        return;
    }

    // Left-click = attack mob (instant) + place block
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Melee attack is still instant
            attackMob(camPos, camFront);
            // Place block
            placeBlock();
        }
        return;
    }

    // Right-click = interact with special block OR hold-to-break
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (hasTarget) {
            int tbt = getBlock(targetCol, targetRow, targetHeight);
            BlockProperties tprops = getBlockProps(tbt);
            if (tbt == BLOCK_CRAFTING_TABLE) {
                // Open crafting/inventory UI with 3x3 grid
                inventoryOpen = true;
                useCraftingTable = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                printf("[Block] Opened crafting table at (%d,%d,%d)\n",
                       targetCol, targetRow, targetHeight);
                return;
            }
            if (tprops.isInteractive) {
                // Toggle open/closed (bit 2 of state)
                uint16_t state = getBlockState(targetCol, targetRow, targetHeight);
                state ^= (1 << 2);
                setBlockState(targetCol, targetRow, targetHeight, state);
                printf("[Block] Toggled interactive block at (%d,%d,%d) -> %s\n",
                       targetCol, targetRow, targetHeight, (state >> 2) & 1 ? "open" : "closed");
                return;
            }
        }
        // Begin hold-to-break — only reset progress if targeting a different block
        isBreaking = true;
        if (targetCol != breakTargetCol || targetRow != breakTargetRow || targetHeight != breakTargetH) {
            breakHoldTime = 0.0f;
            breakTargetCol = targetCol;
            breakTargetRow = targetRow;
            breakTargetH   = targetHeight;
        }
        return;
    }

    // Middle click = pick block (copy targeted block to hotbar)
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (hasTarget) {
            int picked = getBlock(targetCol, targetRow, targetHeight);
            if (picked != BLOCK_AIR) {
                // Check if already in hotbar — select it
                for (int i = 0; i < HOTBAR_SIZE; i++) {
                    if (playerInventory[i].type == picked) {
                        hotbarSlot = i;
                        printf("[PickBlock] Selected existing slot %d (%s)\n", i + 1, getBlockName(picked));
                        return;
                    }
                }
                // Otherwise put in current hotbar slot
                playerInventory[hotbarSlot] = {picked, 1};
                printf("[PickBlock] Picked %s into slot %d\n", getBlockName(picked), hotbarSlot + 1);
            }
        }
        return;
    }
}

void scrollCallback(GLFWwindow* window, double xoff, double yoff) {
    // Scroll always cycles hotbar
    // Hold Ctrl + Scroll for zoom
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        if (cameraMode == 0) {
            thirdPersonDist -= (float)yoff * 0.5f;
            if (thirdPersonDist < 1.5f) thirdPersonDist = 1.5f;
            if (thirdPersonDist > 10.0f) thirdPersonDist = 10.0f;
        } else if (cameraMode == 2) {
            camPos += camFront * (float)yoff * 1.5f;
        }
    } else {
        hotbarSlot = ((hotbarSlot - (int)yoff) % HOTBAR_SIZE + HOTBAR_SIZE) % HOTBAR_SIZE;
        printf("[Hotbar] Selected: slot %d (%s)\n", hotbarSlot + 1,
               getBlockName(playerInventory[hotbarSlot].type));
    }
}

// Typed character callback — feeds search box when recipe book is open
void charCallback(GLFWwindow* window, unsigned int codepoint) {
    if (!inventoryOpen || !recipeBookOpen) return;
    if (codepoint < 128) { // ASCII only
        recipeSearchText += (char)tolower((int)codepoint);
        recipePage = 0; // reset to first page of results
    }
}

void closeInventory(GLFWwindow* window) {
    inventoryOpen = false;
    recipeBookOpen = false;
    recipeSearchText = "";
    useCraftingTable = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // Return crafting grid items to inventory
    for (int ci = 0; ci < 9; ci++) {
        if (craftingGrid[ci].type != BLOCK_AIR) {
            bool placed = false;
            for (int j = 0; j < 36; j++) {
                if (playerInventory[j].type == craftingGrid[ci].type && playerInventory[j].count + craftingGrid[ci].count <= 64) {
                    playerInventory[j].count += craftingGrid[ci].count;
                    placed = true; break;
                }
            }
            if (!placed) {
                for (int j = 0; j < 36; j++) {
                    if (playerInventory[j].type == BLOCK_AIR) {
                        playerInventory[j] = craftingGrid[ci];
                        placed = true; break;
                    }
                }
            }
            if (!placed) {
                printf("[Inventory] Dropped %dx %s (inventory full)\n",
                       craftingGrid[ci].count, getBlockName(craftingGrid[ci].type));
            }
            craftingGrid[ci] = {BLOCK_AIR, 0};
        }
    }
    craftingOutput = {BLOCK_AIR, 0};
    // Return dragged item
    if (draggedSlot.type != BLOCK_AIR) {
        bool placed = false;
        for (int j = 0; j < 36; j++) {
            if (playerInventory[j].type == draggedSlot.type && playerInventory[j].count + draggedSlot.count <= 64) {
                playerInventory[j].count += draggedSlot.count;
                placed = true; break;
            }
        }
        if (!placed) {
            for (int j = 0; j < 36; j++) {
                if (playerInventory[j].type == BLOCK_AIR) {
                    playerInventory[j] = draggedSlot;
                    placed = true; break;
                }
            }
        }
        if (!placed) {
            printf("[Inventory] Dropped %dx %s (inventory full)\n",
                   draggedSlot.count, getBlockName(draggedSlot.type));
        }
        draggedSlot = {BLOCK_AIR, 0};
    }
    printf("[Inventory] Closed\n");
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    // While inventory is open: only Escape and Backspace (for search) are allowed
    if (inventoryOpen) {
        if (key == GLFW_KEY_ESCAPE) {
            closeInventory(window);
        } else if (key == GLFW_KEY_BACKSPACE) {
            if (recipeBookOpen && !recipeSearchText.empty()) {
                recipeSearchText.pop_back();
                recipePage = 0;
            }
        }
        return; // block all other keys
    }

    switch (key) {
        case GLFW_KEY_ESCAPE:
            if (inventoryOpen) {
                closeInventory(window);
            } else {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
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
            
        // Inventory UI Toggle
        case GLFW_KEY_BACKSPACE:
            if (inventoryOpen && recipeBookOpen && !recipeSearchText.empty()) {
                recipeSearchText.pop_back();
                recipePage = 0;
            }
            break;

        case GLFW_KEY_I:
            if (inventoryOpen) {
                closeInventory(window);
            } else {
                inventoryOpen = true;
                useCraftingTable = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                printf("[Inventory] Opened (3x3 crafting)\n");
            }
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

        // Gouraud/Phong shading toggle
        case GLFW_KEY_H:
            useGouraud = !useGouraud;
            printf("[Shading] %s\n", useGouraud ? "Gouraud (per-vertex)" : "Phong (per-fragment)");
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

        // P key: respawn if dead, toggle windows if alive
        case GLFW_KEY_P:
            if (playerDead) {
                glm::vec3 spawnGrid = hexGridPos(3, 5, 0.0f);
                playerWorldPos = glm::vec3(spawnGrid.x, getGroundY(3, 5), spawnGrid.z);
                playerHealth = playerMaxHealth;
                playerStamina = playerMaxStamina;
                playerHunger = playerMaxHunger;
                playerVelY = 0.0f;
                playerOnGround = true;
                playerDead = false;
                trackingFall = false;
                printf("[Player] Respawned!\n");
            } else {
                windowOpen = !windowOpen;
                printf("[Window] %s\n", windowOpen ? "Opening" : "Closing");
            }
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


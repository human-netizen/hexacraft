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

// Check if a block at (col, row, h) is solid (not air, not water)
bool isSolid(int col, int row, int h) {
    int bt = getBlock(col, row, h);
    return bt != BLOCK_AIR && bt != BLOCK_WATER;
}

// Check if player can stand at world position (wx, wz) at current height
bool canMoveTo(float wx, float wz, float currentY) {
    int col, row;
    worldToColRow(wx, wz, col, row);
    int feetH = (int)floorf(currentY / HEX_HEIGHT);
    int headH = feetH + 1;
    if (isSolid(col, row, feetH)) return false;
    if (isSolid(col, row, headH)) return false;
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
    if (bt == BLOCK_WATER) return;
    if (bt == BLOCK_BEDROCK) {
        printf("[Block] Bedrock is unbreakable!\n");
        return;
    }
    setBlock(targetCol, targetRow, targetHeight, BLOCK_AIR);
    spawnItemDrop(targetCol, targetRow, targetHeight, (BlockType)bt);
    printf("[Block] Broke block at (%d, %d, %d)\n", targetCol, targetRow, targetHeight);
}

void placeBlock() {
    if (!hasTarget) return;
    if (!gridInBounds(placeCol, placeRow, placeHeight)) return;
    if (getBlock(placeCol, placeRow, placeHeight) != BLOCK_AIR) return;
    setBlock(placeCol, placeRow, placeHeight, hotbarBlocks[hotbarSlot]);
    printf("[Block] Placed %d at (%d, %d, %d)\n", hotbarBlocks[hotbarSlot], placeCol, placeRow, placeHeight);
}

// =====================================================
// Mob system
// =====================================================
void spawnMob(MobType type, glm::vec3 pos) {
    if ((int)mobs.size() >= MAX_MOBS) return;
    Mob m;
    m.pos = pos;
    m.yaw = (float)(gridSeed((int)(pos.x * 7), (int)(pos.z * 13)) % 628) / 100.0f;
    m.type = type;
    m.state = MOB_IDLE;
    m.health = (type == MOB_ZOMBIE) ? 20.0f : 8.0f;
    m.maxHealth = m.health;
    m.stateTimer = 0.0f;
    m.attackCooldown = 0.0f;
    m.walkTime = 0.0f;
    m.alive = true;
    mobs.push_back(m);
}

void spawnInitialMobs() {
    // Passive mobs in grass zones
    for (int i = 0; i < 8; i++) {
        unsigned int s = gridSeed(i * 17, i * 31 + 5);
        int col = (int)(s % 40) - 20;
        int row = (int)((s >> 8) % 40) - 20;
        glm::vec3 pos = hexGridPos(col, row, 0.0f);
        pos.y = getGroundY(col, row);
        spawnMob((s % 2 == 0) ? MOB_CHICKEN : MOB_PIG, pos);
    }
}

void drawMob(const Mob& m, float time) {
    if (!m.alive) return;
    glm::vec3 p = m.pos;

    if (m.type == MOB_CHICKEN) {
        // Small white body + yellow beak + red comb
        float bob = sinf(m.walkTime * 4.0f) * 0.05f;
        drawHex(p + glm::vec3(0, 0.3f + bob, 0), glm::vec3(0.9f, 0.9f, 0.85f), glm::vec3(0.3f, 0.3f, 0.3f)); // body
        drawHex(p + glm::vec3(0, 0.6f + bob, 0), glm::vec3(0.9f, 0.9f, 0.85f), glm::vec3(0.2f, 0.2f, 0.2f)); // head
        drawHex(p + glm::vec3(0.15f, 0.6f + bob, 0), glm::vec3(0.9f, 0.7f, 0.1f), glm::vec3(0.08f, 0.05f, 0.05f)); // beak
        drawHex(p + glm::vec3(0, 0.75f + bob, 0), glm::vec3(0.8f, 0.1f, 0.1f), glm::vec3(0.08f, 0.1f, 0.08f)); // comb
        // Legs
        drawHex(p + glm::vec3(0.08f, 0.1f, 0), glm::vec3(0.9f, 0.7f, 0.1f), glm::vec3(0.04f, 0.15f, 0.04f));
        drawHex(p + glm::vec3(-0.08f, 0.1f, 0), glm::vec3(0.9f, 0.7f, 0.1f), glm::vec3(0.04f, 0.15f, 0.04f));
    } else if (m.type == MOB_PIG) {
        // Pink body, snout, ears
        float bob = sinf(m.walkTime * 3.0f) * 0.03f;
        glm::vec3 pink(0.9f, 0.6f, 0.6f);
        glm::vec3 darkPink(0.8f, 0.5f, 0.5f);
        drawHex(p + glm::vec3(0, 0.35f + bob, 0), pink, glm::vec3(0.4f, 0.35f, 0.35f)); // body
        drawHex(p + glm::vec3(0.3f, 0.4f + bob, 0), pink, glm::vec3(0.25f, 0.25f, 0.25f)); // head
        drawHex(p + glm::vec3(0.45f, 0.38f + bob, 0), darkPink, glm::vec3(0.08f, 0.08f, 0.08f)); // snout
        // Legs
        drawHex(p + glm::vec3(0.15f, 0.1f, 0.15f), darkPink, glm::vec3(0.06f, 0.15f, 0.06f));
        drawHex(p + glm::vec3(0.15f, 0.1f, -0.15f), darkPink, glm::vec3(0.06f, 0.15f, 0.06f));
        drawHex(p + glm::vec3(-0.15f, 0.1f, 0.15f), darkPink, glm::vec3(0.06f, 0.15f, 0.06f));
        drawHex(p + glm::vec3(-0.15f, 0.1f, -0.15f), darkPink, glm::vec3(0.06f, 0.15f, 0.06f));
    } else if (m.type == MOB_ZOMBIE) {
        // Green humanoid
        float bob = sinf(m.walkTime * 3.5f) * 0.04f;
        glm::vec3 zGreen(0.3f, 0.5f, 0.2f);
        glm::vec3 zDark(0.2f, 0.35f, 0.15f);
        // Body
        drawHex(p + glm::vec3(0, 0.6f + bob, 0), zGreen, glm::vec3(0.3f, 0.45f, 0.2f));
        // Head
        drawHex(p + glm::vec3(0, 1.1f + bob, 0), zDark, glm::vec3(0.22f, 0.22f, 0.22f));
        // Arms (outstretched forward)
        float armSwing = sinf(m.walkTime * 3.5f) * 0.15f;
        drawHex(p + glm::vec3(0.35f, 0.7f + bob, armSwing), zGreen, glm::vec3(0.08f, 0.35f, 0.08f));
        drawHex(p + glm::vec3(-0.35f, 0.7f + bob, -armSwing), zGreen, glm::vec3(0.08f, 0.35f, 0.08f));
        // Legs
        float legSwing = sinf(m.walkTime * 3.5f) * 0.1f;
        drawHex(p + glm::vec3(0.1f, 0.15f, legSwing), zDark, glm::vec3(0.1f, 0.25f, 0.1f));
        drawHex(p + glm::vec3(-0.1f, 0.15f, -legSwing), zDark, glm::vec3(0.1f, 0.25f, 0.1f));

        // Health bar above head if damaged
        if (m.health < m.maxHealth) {
            float hFrac = m.health / m.maxHealth;
            drawHex(p + glm::vec3(0, 1.5f, 0), glm::vec3(0.8f, 0.1f, 0.1f), glm::vec3(0.3f * hFrac, 0.04f, 0.04f));
        }
    }
}

void updateMobs(float dt, float time) {
    // Spawn zombies at night
    mobSpawnTimer += dt;
    if (dayFactor < 0.3f && mobSpawnTimer > 5.0f) {
        mobSpawnTimer = 0.0f;
        int zombieCount = 0;
        for (auto& m : mobs) if (m.type == MOB_ZOMBIE && m.alive) zombieCount++;
        if (zombieCount < 8) {
            // Spawn near player but not too close
            unsigned int s = gridSeed((int)(time * 100), (int)(playerWorldPos.x * 7));
            float angle = (float)(s % 628) / 100.0f;
            float dist = 15.0f + (float)(s % 10);
            glm::vec3 spawnPos = playerWorldPos + glm::vec3(cosf(angle) * dist, 0, sinf(angle) * dist);
            int sc, sr;
            worldToColRow(spawnPos.x, spawnPos.z, sc, sr);
            spawnPos.y = getGroundY(sc, sr);
            // Check spawn is not near a torch (safe zone)
            bool nearTorch = false;
            for (auto& tp : torchPositions) {
                if (glm::length(tp - spawnPos) < 5.0f) { nearTorch = true; break; }
            }
            if (!nearTorch) {
                spawnMob(MOB_ZOMBIE, spawnPos);
                printf("[Mob] Zombie spawned!\n");
            }
        }
    }
    // Despawn zombies during day
    if (dayFactor > 0.5f) {
        for (int i = (int)mobs.size() - 1; i >= 0; i--) {
            if (mobs[i].type == MOB_ZOMBIE) {
                mobs.erase(mobs.begin() + i);
            }
        }
    }

    for (auto& m : mobs) {
        if (!m.alive) continue;
        m.stateTimer += dt;
        m.attackCooldown -= dt;
        if (m.attackCooldown < 0.0f) m.attackCooldown = 0.0f;

        float distToPlayer = glm::length(m.pos - playerWorldPos);

        if (m.type == MOB_ZOMBIE) {
            // Hostile: chase player when within 15 blocks
            if (distToPlayer < 15.0f) {
                m.state = MOB_CHASE;
            } else {
                m.state = MOB_WANDER;
            }

            if (m.state == MOB_CHASE) {
                glm::vec3 dir = playerWorldPos - m.pos;
                dir.y = 0.0f;
                if (glm::length(dir) > 0.5f) {
                    dir = glm::normalize(dir);
                    m.yaw = atan2f(dir.z, dir.x);
                    float speed = 2.5f * dt;
                    m.pos.x += dir.x * speed;
                    m.pos.z += dir.z * speed;
                    m.walkTime += dt * 4.0f;
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
            }
        } else {
            // Passive: wander randomly
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

        // Snap to ground
        int mc, mr;
        worldToColRow(m.pos.x, m.pos.z, mc, mr);
        m.pos.y = getGroundYWorld(m.pos.x, m.pos.z);
    }

    // Remove dead mobs
    for (int i = (int)mobs.size() - 1; i >= 0; i--) {
        if (!mobs[i].alive) {
            // Drop loot
            if (mobs[i].type == MOB_PIG) {
                // Drop food (represented as a dirt block for now)
                playerHunger += 4.0f;
                if (playerHunger > playerMaxHunger) playerHunger = playerMaxHunger;
                printf("[Mob] Pig dropped food! Hunger restored.\n");
            }
            mobs.erase(mobs.begin() + i);
        }
    }
}

// 13F: Combat — left-click on mob to attack
void attackMob(glm::vec3 camOrigin, glm::vec3 camDir) {
    float bestDist = 4.0f; // melee range
    int bestIdx = -1;
    for (int i = 0; i < (int)mobs.size(); i++) {
        if (!mobs[i].alive) continue;
        // Simple distance + direction check
        glm::vec3 toMob = mobs[i].pos - camOrigin;
        float d = glm::length(toMob);
        if (d > bestDist) continue;
        float dot = glm::dot(glm::normalize(toMob), camDir);
        if (dot > 0.85f && d < bestDist) { // within ~30 degree cone
            bestDist = d;
            bestIdx = i;
        }
    }
    if (bestIdx >= 0) {
        mobs[bestIdx].health -= 5.0f;
        // Knockback
        glm::vec3 kb = glm::normalize(mobs[bestIdx].pos - camOrigin);
        kb.y = 0.2f;
        mobs[bestIdx].pos += kb * 1.5f;
        printf("[Combat] Hit %s! Health: %.0f\n",
            mobs[bestIdx].type == MOB_ZOMBIE ? "Zombie" :
            mobs[bestIdx].type == MOB_PIG ? "Pig" : "Chicken",
            mobs[bestIdx].health);
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
    drawFractalBranch(base, glm::vec3(0, 1, 0), 1.5f, 0.15f, 0, 4);
}

// =====================================================
// Render all scene objects
// =====================================================
void renderObjects(float time) {
    // Player — draw at actual player position (only in third-person)
    if (cameraMode == 0) {
        drawPlayer(playerWorldPos, time, playerYaw, playerWalking, playerWalkTime);
    }

    // Fan — inside castle great hall
    int fCol = -5, fRow = 18;
    glm::vec3 fanPos = hexGridPos(fCol, fRow, 0.0f);
    fanPos.y = getGroundY(fCol, fRow);
    drawFan(fanPos);

    // Door — castle great hall to throne room doorway
    int dCol = 2, dRow = 27;
    glm::vec3 doorPos = hexGridPos(dCol, dRow, 0.0f);
    doorPos.y = getGroundY(dCol, dRow);
    drawDoor(doorPos);

    // Windows — on castle walls (P key toggles)
    {
        int wCol1 = -8, wRow1 = 22;
        glm::vec3 wPos1 = hexGridPos(wCol1, wRow1, 0.0f);
        wPos1.y = getGroundY(wCol1, wRow1) + 1.5f;
        drawWindow(wPos1, windowAngle, 0.0f);

        int wCol2 = 8, wRow2 = 22;
        glm::vec3 wPos2 = hexGridPos(wCol2, wRow2, 0.0f);
        wPos2.y = getGroundY(wCol2, wRow2) + 1.5f;
        drawWindow(wPos2, windowAngle, 180.0f);

        int wCol3 = 0, wRow3 = 35;
        glm::vec3 wPos3 = hexGridPos(wCol3, wRow3, 0.0f);
        wPos3.y = getGroundY(wCol3, wRow3) + 1.5f;
        drawWindow(wPos3, windowAngle, 90.0f);
    }

    // Clock — inside castle throne room (on a pillar)
    int cCol = 10, cRow = 34;
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

    // --- Curvy Objects (with textures applied) ---

    // Sphere — crystal ball with BLENDED grass texture (texture * color)
    int sphCol = -10, sphRow = 48;
    glm::vec3 sphBase = hexGridPos(sphCol, sphRow, 0.0f);
    sphBase.y = getGroundY(sphCol, sphRow);
    drawHex(sphBase + glm::vec3(0, 0.5f, 0), COL_STONE, glm::vec3(0.4f, 1.0f, 0.4f));
    // Enable texture mode 2 (blended)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texGrass);
    setInt(shaderProgram, "texture1", 0);
    setInt(shaderProgram, "textureMode", 2); // blended
    drawSphere(sphBase + glm::vec3(0, 1.8f, 0), 0.6f, glm::vec3(0.3f, 0.8f, 1.0f));
    setInt(shaderProgram, "textureMode", 0); // reset

    // Cone — tent with SIMPLE wood texture (texture only, no color)
    int coneCol = 12, coneRow = 5;
    glm::vec3 coneBase = hexGridPos(coneCol, coneRow, 0.0f);
    coneBase.y = getGroundY(coneCol, coneRow);
    glBindTexture(GL_TEXTURE_2D, texWood);
    setInt(shaderProgram, "textureMode", 1); // simple
    drawCone(coneBase + glm::vec3(0, 0.1f, 0), 1.5f, glm::vec3(0.85f, 0.55f, 0.25f));
    setInt(shaderProgram, "textureMode", 0);

    // Brick wall panel — simple texture on a flat hex surface near castle
    {
        int bwCol = -12, bwRow = 15;
        glm::vec3 bwBase = hexGridPos(bwCol, bwRow, 0.0f);
        bwBase.y = getGroundY(bwCol, bwRow);
        glBindTexture(GL_TEXTURE_2D, texBrick);
        setInt(shaderProgram, "textureMode", 1); // simple texture
        drawHex(bwBase + glm::vec3(0, 1.0f, 0), glm::vec3(0.7f), glm::vec3(2.0f, 2.0f, 0.15f));
        setInt(shaderProgram, "textureMode", 0);
    }

    // Wine glass — surface of revolution on castle table
    {
        int wgCol = 5, wgRow = 30;
        glm::vec3 wgBase = hexGridPos(wgCol, wgRow, 0.0f);
        wgBase.y = getGroundY(wgCol, wgRow);
        // Small table
        drawHex(wgBase + glm::vec3(0, 0.5f, 0), glm::vec3(0.4f, 0.25f, 0.15f), glm::vec3(0.6f, 1.0f, 0.6f));
        drawHex(wgBase + glm::vec3(0, 1.1f, 0), glm::vec3(0.5f, 0.3f, 0.18f), glm::vec3(1.0f, 0.1f, 1.0f));
        // Glass on table (semi-transparent)
        setFloat(shaderProgram, "alpha", 0.6f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawWineGlass(wgBase + glm::vec3(0, 1.2f, 0), 0.5f, glm::vec3(0.85f, 0.92f, 0.95f));
        // Second glass
        drawWineGlass(wgBase + glm::vec3(0.6f, 1.2f, 0.3f), 0.4f, glm::vec3(0.9f, 0.85f, 0.95f));
        glDisable(GL_BLEND);
        setFloat(shaderProgram, "alpha", 1.0f);
    }

    // Bezier curve — decorative arch near ruins
    int bezCol = 57, bezRow = -12;
    glm::vec3 bezBase = hexGridPos(bezCol, bezRow, 0.0f);
    bezBase.y = getGroundY(bezCol, bezRow);
    drawBezier(bezBase + glm::vec3(0, 0.5f, 0), 2.0f, glm::vec3(0.7f, 0.5f, 0.9f));

    // Spline — winding fence along path to castle
    int splCol = -5, splRow = 8;
    glm::vec3 splBase = hexGridPos(splCol, splRow, 0.0f);
    splBase.y = getGroundY(splCol, splRow);
    drawSpline(splBase + glm::vec3(0, 0.3f, 0), 2.5f, glm::vec3(0.6f, 0.4f, 0.2f));

    // Ruled surface — ramp outside castle west wall
    int rmpCol = -25, rmpRow = 30;
    glm::vec3 rmpBase = hexGridPos(rmpCol, rmpRow, 0.0f);
    rmpBase.y = getGroundY(rmpCol, rmpRow);
    drawRuledSurface(rmpBase + glm::vec3(0, 0.3f, 0), 2.0f, glm::vec3(0.5f, 0.7f, 0.5f));

    // Fractal Trees — landmark trees at specific locations
    {
        int ftCol1 = 20, ftRow1 = 15;
        glm::vec3 ftBase1 = hexGridPos(ftCol1, ftRow1, 0.0f);
        ftBase1.y = getGroundY(ftCol1, ftRow1);
        drawFractalTree(ftBase1);

        int ftCol2 = -15, ftRow2 = 25;
        glm::vec3 ftBase2 = hexGridPos(ftCol2, ftRow2, 0.0f);
        ftBase2.y = getGroundY(ftCol2, ftRow2);
        drawFractalTree(ftBase2);

        int ftCol3 = 30, ftRow3 = -20;
        glm::vec3 ftBase3 = hexGridPos(ftCol3, ftRow3, 0.0f);
        ftBase3.y = getGroundY(ftCol3, ftRow3);
        drawFractalTree(ftBase3);
    }

    // --- Flying Birds ---
    for (auto& b : birds) {
        drawBird(b, time);
    }

    // --- Mobs ---
    for (auto& m : mobs) {
        drawMob(m, time);
    }

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
            inventoryCounts[d.type]++;
            printf("[Pickup] Got %d (total: %d)\n", d.type, inventoryCounts[d.type]);
            itemDrops.erase(itemDrops.begin() + i);
        }
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

    // Left-click = attack mob or break block
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        // Try attacking a mob first (melee range)
        attackMob(camPos, camFront);
        // Also break block if targeted
        if (hasTarget) {
            breakBlock();
            printf("[Debug] Break at (%d,%d,%d)\n", targetCol, targetRow, targetHeight);
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


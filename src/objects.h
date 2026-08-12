#pragma once
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

    // Hour hand: angular speed = 2π/3600 rad/s (spec), pivots from center
    float hourAngle = -fmodf(time * (2.0f * PI / 3600.0f), 2.0f * PI);
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0, 0, 0.13f));
        model = myRotate(model, hourAngle, glm::vec3(0, 0, 1));
        model = glm::translate(model, glm::vec3(0.2f, 0, 0));
        model = glm::scale(model, glm::vec3(0.4f, 0.07f, 0.05f));
        drawHexModel(model, glm::vec3(0.12f, 0.12f, 0.12f));
    }

    // Minute hand: angular speed = 2π/60 rad/s (spec), pivots from center
    float minAngle = -fmodf(time * (2.0f * PI / 60.0f), 2.0f * PI);
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

    // In third-person the camera is behind the player — cast from the player's
    // eye so the crosshair correctly hits what the player is looking at.
    glm::vec3 rayOrigin = (cameraMode == 2) ? camPos
                                             : (playerWorldPos + glm::vec3(0, 1.62f, 0));

    for (float d = 0.2f; d < maxDist; d += step) {
        glm::vec3 p = rayOrigin + camFront * d;
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


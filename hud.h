#pragma once
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
    glBufferData(GL_ARRAY_BUFFER, 8192 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    // pos(3) + normal(3) + color(3) + texCoord(2) = 11 floats
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9*sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);
}

// Helper: push a colored quad (two triangles) into vertex buffer
void pushQuad(std::vector<float>& verts, float x1, float y1, float x2, float y2, glm::vec3 col) {
    // normal = (0,0,1), all same color, uv = (0,0)
    float n[] = {0,0,1};
    // Triangle 1: 3 vertices × 11 floats = 33
    float tri1[] = {x1,y1,0, n[0],n[1],n[2], col.r,col.g,col.b, 0,0,
                    x2,y1,0, n[0],n[1],n[2], col.r,col.g,col.b, 0,0,
                    x2,y2,0, n[0],n[1],n[2], col.r,col.g,col.b, 0,0};
    // Triangle 2
    float tri2[] = {x1,y1,0, n[0],n[1],n[2], col.r,col.g,col.b, 0,0,
                    x2,y2,0, n[0],n[1],n[2], col.r,col.g,col.b, 0,0,
                    x1,y2,0, n[0],n[1],n[2], col.r,col.g,col.b, 0,0};
    for (int i = 0; i < 33; i++) verts.push_back(tri1[i]);
    for (int i = 0; i < 33; i++) verts.push_back(tri2[i]);
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
    float healthFrac = playerHealth / playerMaxHealth;
    pushQuad(verts, barX, barY, barX + barW, barY + barH, glm::vec3(0.3f, 0.0f, 0.0f)); // bg
    pushQuad(verts, barX, barY, barX + barW * healthFrac, barY + barH, glm::vec3(0.9f, 0.15f, 0.1f));

    // --- Stamina bar ---
    float stY = barY - 18.0f;
    float staminaFrac = playerStamina / playerMaxStamina;
    pushQuad(verts, barX, stY, barX + barW, stY + barH, glm::vec3(0.0f, 0.3f, 0.0f)); // bg
    pushQuad(verts, barX, stY, barX + barW * staminaFrac, stY + barH, glm::vec3(0.1f, 0.8f, 0.2f));

    // --- Hunger bar ---
    float huY = stY - 18.0f;
    float hungerFrac = playerHunger / playerMaxHunger;
    pushQuad(verts, barX, huY, barX + barW, huY + barH, glm::vec3(0.25f, 0.15f, 0.0f)); // bg
    pushQuad(verts, barX, huY, barX + barW * hungerFrac, huY + barH, glm::vec3(0.75f, 0.5f, 0.1f));

    // Upload and draw
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    int vertCount = (int)verts.size() / 11;
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
        glDrawArrays(GL_TRIANGLES, 0, (int)cv.size() / 11);
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
        glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 11);

        // Block color inner
        std::vector<float> iv;
        float pad = 6.0f;
        glm::vec3 bc = getBlockColor(hotbarBlocks[i]);
        pushQuad(iv, sx + pad, hbY + pad, sx + slotSize - pad, hbY + slotSize - pad, bc);
        glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size() * sizeof(float), iv.data());
        setVec3(shaderProgram, "emissiveColor", bc);
        glDrawArrays(GL_TRIANGLES, 0, (int)iv.size() / 11);
    }

    // Helper to draw a single bar
    auto drawBar = [&](float bx, float by, float bw, float bh, float frac, glm::vec3 color) {
        // Background
        std::vector<float> bv;
        pushQuad(bv, bx - 1, by - 1, bx + bw + 1, by + bh + 1, glm::vec3(0.15f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, bv.size() * sizeof(float), bv.data());
        setVec3(shaderProgram, "emissiveColor", glm::vec3(0.15f));
        glDrawArrays(GL_TRIANGLES, 0, (int)bv.size() / 11);
        // Fill
        if (frac > 0.001f) {
            bv.clear();
            pushQuad(bv, bx, by, bx + bw * frac, by + bh, color);
            glBufferSubData(GL_ARRAY_BUFFER, 0, bv.size() * sizeof(float), bv.data());
            setVec3(shaderProgram, "emissiveColor", color);
            glDrawArrays(GL_TRIANGLES, 0, (int)bv.size() / 11);
        }
    };

    // Health bar (red)
    drawBar(barX, barY, barW, barH, healthFrac, glm::vec3(0.9f, 0.15f, 0.1f));
    // Stamina bar (green)
    drawBar(barX, stY, barW, barH, staminaFrac, glm::vec3(0.1f, 0.8f, 0.2f));
    // Hunger bar (brown/orange)
    drawBar(barX, huY, barW, barH, hungerFrac, glm::vec3(0.75f, 0.5f, 0.1f));

    // Death screen overlay
    if (playerDead) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        setFloat(shaderProgram, "alpha", 0.6f);
        // Dark red overlay
        std::vector<float> dv;
        pushQuad(dv, 0, 0, screenW, screenH, glm::vec3(0.4f, 0.0f, 0.0f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, dv.size() * sizeof(float), dv.data());
        setVec3(shaderProgram, "emissiveColor", glm::vec3(0.4f, 0.0f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 0, (int)dv.size() / 11);
        setFloat(shaderProgram, "alpha", 1.0f);
        glDisable(GL_BLEND);

        // "YOU DIED" text as a bright bar in center
        float txtW = 200.0f, txtH = 30.0f;
        float txtX = (screenW - txtW) / 2.0f, txtY = screenH / 2.0f;
        std::vector<float> tv;
        pushQuad(tv, txtX, txtY, txtX + txtW, txtY + txtH, glm::vec3(0.8f, 0.1f, 0.1f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, tv.size() * sizeof(float), tv.data());
        setVec3(shaderProgram, "emissiveColor", glm::vec3(0.8f, 0.1f, 0.1f));
        glDrawArrays(GL_TRIANGLES, 0, (int)tv.size() / 11);
    }

    glBindVertexArray(0);
    setBool(shaderProgram, "isEmissive", false);
    glEnable(GL_DEPTH_TEST);
}


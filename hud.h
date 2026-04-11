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

extern GLuint texBrick, texGrass, texWood;
extern GLuint texItemOakDoor, texItemIronDoor;

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

// Helper: push a colored quad into vertex buffer with specific UVs
void pushQuadUV(std::vector<float>& verts, float x1, float y1, float x2, float y2, glm::vec3 col, float u1, float v1, float u2, float v2) {
    float n[] = {0,0,1};
    // Triangle 1
    float tri1[] = {x1,y1,0, n[0],n[1],n[2], col.r,col.g,col.b, u1,v1,
                    x2,y1,0, n[0],n[1],n[2], col.r,col.g,col.b, u2,v1,
                    x2,y2,0, n[0],n[1],n[2], col.r,col.g,col.b, u2,v2};
    // Triangle 2
    float tri2[] = {x1,y1,0, n[0],n[1],n[2], col.r,col.g,col.b, u1,v1,
                    x2,y2,0, n[0],n[1],n[2], col.r,col.g,col.b, u2,v2,
                    x1,y2,0, n[0],n[1],n[2], col.r,col.g,col.b, u1,v2};
    for (int i = 0; i < 33; i++) verts.push_back(tri1[i]);
    for (int i = 0; i < 33; i++) verts.push_back(tri2[i]);
}

// Helper: push a colored quad (two triangles) into vertex buffer (default no UV)
void pushQuad(std::vector<float>& verts, float x1, float y1, float x2, float y2, glm::vec3 col) {
    pushQuadUV(verts, x1, y1, x2, y2, col, 0,0,0,0);
}

// Helper: push an arbitrary 4-point polygon (2 triangles, CCW: a→b→c→d) — for isometric faces
void pushPoly4(std::vector<float>& v, float ax, float ay, float bx, float by,
               float cx, float cy, float dx, float dy, glm::vec3 col) {
    auto pv = [&](float px, float py, float u, float vv) {
        v.push_back(px); v.push_back(py); v.push_back(0);
        v.push_back(0);  v.push_back(0);  v.push_back(1);
        v.push_back(col.r); v.push_back(col.g); v.push_back(col.b);
        v.push_back(u); v.push_back(vv);
    };
    pv(ax,ay,0,0); pv(bx,by,1,0); pv(cx,cy,1,1);   // tri 1
    pv(ax,ay,0,0); pv(cx,cy,1,1); pv(dx,dy,0,1);   // tri 2
}

// Draw a Minecraft-style isometric block icon into the HUD vertex buffer.
// ox,oy = bottom-left of drawing area, b = size in pixels.
// sideH = fraction of b used by side faces (rest is top face).
// Faces: top (lightest), left (mid), right (darkest) — matching MC shading.
void pushIsoBlock(std::vector<float>& v, float ox, float oy, float b,
                  glm::vec3 col, float sideHFrac = 0.62f) {
    float sh  = b * sideHFrac;          // side face height
    float hw  = b * 0.5f;               // half width
    float lean = b * 0.22f;             // top-face parallelogram lean

    glm::vec3 top   = glm::min(col * 1.30f, glm::vec3(1.0f)); // lighter
    glm::vec3 left  = col;                                      // base
    glm::vec3 right = col * 0.68f;                              // shadow

    // Left face  (ox, oy) → (ox+hw, oy) → (ox+hw, oy+sh) → (ox, oy+sh)
    pushQuad(v, ox,    oy,    ox+hw, oy+sh, left);
    // Right face (ox+hw, oy) → (ox+b, oy) → (ox+b, oy+sh) → (ox+hw, oy+sh)
    pushQuad(v, ox+hw, oy,    ox+b,  oy+sh, right);
    // Top face — parallelogram: BL, BR, TR, TL
    pushPoly4(v,
        ox,          oy+sh,          // BL
        ox+b,        oy+sh,          // BR
        ox+b-lean,   oy+b,           // TR
        ox+lean,     oy+b,           // TL
        top);
}

// Draw isometric block icon using an actual texture on all 3 faces.
// Draws 3 separate GL calls: left face, right face, top face — each tinted differently.
// Must be called with hudVAO/VBO bound. Resets textureMode to 0 when done.
void drawIsoBlockTex(GLuint tex, float ox, float oy, float b, glm::vec3 tint,
                     float sideHFrac = 0.62f) {
    float sh   = b * sideHFrac;
    float hw   = b * 0.5f;
    float lean = b * 0.22f;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    setInt(shaderProgram, "textureMode", 2);   // texture * vertex color (tint)

    std::vector<float> v;
    // Left face — normal brightness
    pushQuadUV(v, ox, oy, ox+hw, oy+sh, tint, 0,0,0.5f,1);
    glBufferSubData(GL_ARRAY_BUFFER, 0, v.size()*sizeof(float), v.data());
    glDrawArrays(GL_TRIANGLES, 0, (int)v.size()/11);

    v.clear();
    // Right face — dark (shadow side)
    glm::vec3 dark = tint * 0.62f;
    pushQuadUV(v, ox+hw, oy, ox+b, oy+sh, dark, 0.5f,0,1,1);
    glBufferSubData(GL_ARRAY_BUFFER, 0, v.size()*sizeof(float), v.data());
    glDrawArrays(GL_TRIANGLES, 0, (int)v.size()/11);

    v.clear();
    // Top face — light (parallelogram, UV mapped to full texture)
    glm::vec3 lite = glm::min(tint * 1.25f, glm::vec3(1.0f));
    pushPoly4(v,
        ox,        oy+sh,       ox+b,      oy+sh,
        ox+b-lean, oy+b,        ox+lean,   oy+b,  lite);
    // set proper UV on the top poly vertices (overwrite u,v bytes — simpler to re-push)
    // poly4 already has UV 0,0 / 1,0 / 1,1 / 0,1 baked in from the updated pushPoly4
    glBufferSubData(GL_ARRAY_BUFFER, 0, v.size()*sizeof(float), v.data());
    glDrawArrays(GL_TRIANGLES, 0, (int)v.size()/11);

    setInt(shaderProgram, "textureMode", 0);
}

void renderHUD(int screenW, int screenH) {
    // Set up orthographic projection for screen-space rendering
    glm::mat4 ortho = glm::ortho(0.0f, (float)screenW, 0.0f, (float)screenH, -1.0f, 1.0f);
    setMat4(shaderProgram, "projection", ortho);
    setMat4(shaderProgram, "view", glm::mat4(1.0f));
    glDisable(GL_DEPTH_TEST);

    // Disable fog and lighting for HUD
    setFloat(shaderProgram, "fogDensity", 0.0f);
    setBool(shaderProgram, "isHUD", true);

    std::vector<float> verts;

    // --- Crosshair (+) at screen center ---
    float cx = screenW / 2.0f, cy = screenH / 2.0f;
    float cLen = 12.0f, cThick = 2.0f;
    glm::vec3 white(1.0f, 1.0f, 1.0f);
    pushQuad(verts, cx - cLen, cy - cThick, cx + cLen, cy + cThick, white); // horizontal
    pushQuad(verts, cx - cThick, cy - cLen, cx + cThick, cy + cLen, white); // vertical


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
    setVec3(shaderProgram, "objectColor", glm::vec3(1.0f));
    glm::mat4 identity(1.0f);
    setMat4(shaderProgram, "model", identity);

    // Draw each element with its color via objectColor uniform
    // Actually since we're in emissive mode, the objectColor is what matters
    // But we set vertex colors in the buffer — we need to use objectColor per quad
    // Simpler: draw all at once with emissive=true, the color is baked into objectColor
    // Let's just iterate and draw quads

    // Actually the simplest approach: draw each quad group separately
    // But for efficiency, let's use a single draw call and rely on objectColor
    // Since emissive mode uses objectColor directly, we need a different approach.

    // Simplest fix: temporarily disable the custom emissive and just set objectColor per vertex
    // But fragment shader uses objectColor, not objectColor, in emissive mode.
    // Solution: draw each element separately with the right objectColor.

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
        setVec3(shaderProgram, "objectColor", white);
        setMat4(shaderProgram, "model", identity);
        glDrawArrays(GL_TRIANGLES, 0, (int)cv.size() / 11);
    }

    // --- Inventory + Hotbar UI ---
    float slotSize = 40.0f;
    float slotGap = 4.0f;
    float slotStep = slotSize + slotGap; // 44
    float totalW = HOTBAR_SIZE * slotSize + (HOTBAR_SIZE - 1) * slotGap; // 392

    // Helper: flush iv to GPU and draw, then clear
    auto flushDraw = [&](std::vector<float>& iv) {
        if (iv.empty()) return;
        glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size() * sizeof(float), iv.data());
        glDrawArrays(GL_TRIANGLES, 0, (int)iv.size() / 11);
        iv.clear();
    };

    // Helper: draw a single inventory item in a slot — Minecraft-style isometric icons
    auto drawSlotItem = [&](float sx, float sy, int bType) {
        if (bType == BLOCK_AIR) return;
        std::vector<float> iv;
        float pad  = 5.0f;
        float b    = slotSize - pad * 2.0f;   // 30px drawing area
        float ox   = sx + pad;
        float oy   = sy + pad;
        glm::vec3 bc = getBlockColor(bType);
        BlockShape shape = getBlockProps(bType).shape;
        GLuint blockTex = getBlockTexture(bType);

        // Helper: draw iso block, using real texture if available, else solid color
        auto isoBlock = [&](float _ox, float _oy, float _b, glm::vec3 tint, float shf = 0.62f) {
            if (blockTex) drawIsoBlockTex(blockTex, _ox, _oy, _b, tint, shf);
            else          { pushIsoBlock(iv, _ox, _oy, _b, tint, shf); flushDraw(iv); }
        };

        // ---- DOOR: item sprite (tall flat panel) ----
        if (shape == SHAPE_DOOR) {
            GLuint doorTex = (bType == BLOCK_DOOR_OAK  && texItemOakDoor)  ? texItemOakDoor
                           : (bType == BLOCK_DOOR_IRON && texItemIronDoor) ? texItemIronDoor : 0;
            float dw = b * 0.52f, dh = b * 0.96f;
            float dx = ox + (b - dw) * 0.5f, dy = oy + (b - dh) * 0.5f;
            if (doorTex) {
                pushQuadUV(iv, dx, dy, dx+dw, dy+dh, glm::vec3(1.0f), 0,0,1,1);
                glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size()*sizeof(float), iv.data());
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, doorTex);
                setInt(shaderProgram, "textureMode", 1);
                glDrawArrays(GL_TRIANGLES, 0, (int)iv.size()/11);
                setInt(shaderProgram, "textureMode", 0); iv.clear();
                return;
            }
            // Fallback procedural door
            pushQuad(iv, dx, dy, dx+dw, dy+dh, bc);  flushDraw(iv);
            glm::vec3 fr = bc*0.55f; float ft = 2.0f;
            pushQuad(iv,dx,dy,dx+dw,dy+ft,fr); pushQuad(iv,dx,dy+dh-ft,dx+dw,dy+dh,fr);
            pushQuad(iv,dx,dy,dx+ft,dy+dh,fr); pushQuad(iv,dx+dw-ft,dy,dx+dw,dy+dh,fr);
            pushQuad(iv,dx+dw*0.5f-1.0f,dy+ft,dx+dw*0.5f+1.0f,dy+dh-ft,fr); flushDraw(iv);
            pushQuad(iv,dx+dw*0.74f-2.0f,dy+dh*0.5f-2.0f,dx+dw*0.74f+2.0f,dy+dh*0.5f+2.0f,glm::vec3(0.9f,0.82f,0.25f));
            flushDraw(iv); return;
        }

        // ---- TRAPDOOR ----
        if (shape == SHAPE_TRAPDOOR) { isoBlock(ox, oy+b*0.28f, b, bc, 0.20f); return; }

        // ---- SLAB ----
        if (shape == SHAPE_SLAB)     { isoBlock(ox, oy,         b, bc, 0.32f); return; }

        // ---- STAIR ----
        if (shape == SHAPE_STAIR) {
            isoBlock(ox, oy, b, bc, 0.30f);
            float hw = b*0.5f, stepBase = oy + b*0.30f*0.62f;
            isoBlock(ox+hw*0.5f, stepBase, hw, bc*0.92f, 0.55f);
            return;
        }

        // ---- CARPET ----
        if (shape == SHAPE_CARPET)   { isoBlock(ox, oy, b, bc, 0.10f); return; }

        // ---- FENCE: post + rails (textured if available) ----
        if (shape == SHAPE_FENCE) {
            float cx2 = ox+b*0.5f, pw = b*0.18f;
            if (blockTex) {
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, blockTex);
                setInt(shaderProgram, "textureMode", 2);
                pushQuadUV(iv,cx2-pw*0.5f,oy,cx2+pw*0.5f,oy+b,      bc,  0.4f,0,0.6f,1);
                pushQuadUV(iv,ox+b*0.08f,oy+b*0.28f,ox+b*0.92f,oy+b*0.28f+pw,bc*0.85f,0,0.4f,1,0.6f);
                pushQuadUV(iv,ox+b*0.08f,oy+b*0.62f,ox+b*0.92f,oy+b*0.62f+pw,bc*0.85f,0,0.4f,1,0.6f);
                glBufferSubData(GL_ARRAY_BUFFER,0,iv.size()*sizeof(float),iv.data());
                glDrawArrays(GL_TRIANGLES,0,(int)iv.size()/11);
                setInt(shaderProgram,"textureMode",0); iv.clear();
            } else {
                pushQuad(iv,cx2-pw*0.5f,oy,cx2+pw*0.5f,oy+b,bc);
                pushQuad(iv,ox+b*0.08f,oy+b*0.28f,ox+b*0.92f,oy+b*0.28f+pw,bc*0.85f);
                pushQuad(iv,ox+b*0.08f,oy+b*0.62f,ox+b*0.92f,oy+b*0.62f+pw,bc*0.85f);
                flushDraw(iv);
            }
            return;
        }

        // ---- PANE / FLAT PANEL ----
        if (shape == SHAPE_PANE || shape == SHAPE_FLAT_PANEL) {
            float pw = b*0.14f, cx2 = ox+b*0.5f;
            glm::vec3 pc = getBlockProps(bType).isTransparent
                ? glm::mix(bc,glm::vec3(0.7f,0.9f,1.0f),0.4f) : bc;
            if (blockTex) {
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, blockTex);
                setInt(shaderProgram,"textureMode",2);
                pushQuadUV(iv,cx2-pw*0.5f,oy+b*0.04f,cx2+pw*0.5f,oy+b*0.96f,pc, 0,0,1,1);
                pushQuadUV(iv,ox+b*0.04f,oy+b*0.47f,ox+b*0.96f,oy+b*0.53f,pc*0.85f, 0,0,1,1);
                glBufferSubData(GL_ARRAY_BUFFER,0,iv.size()*sizeof(float),iv.data());
                glDrawArrays(GL_TRIANGLES,0,(int)iv.size()/11);
                setInt(shaderProgram,"textureMode",0); iv.clear();
            } else {
                pushQuad(iv,cx2-pw*0.5f,oy+b*0.04f,cx2+pw*0.5f,oy+b*0.96f,pc);
                pushQuad(iv,ox+b*0.04f,oy+b*0.47f,ox+b*0.96f,oy+b*0.53f,pc*0.85f);
                flushDraw(iv);
            }
            return;
        }

        // ---- SMALL HEX / EMISSIVE ----
        if (shape == SHAPE_SMALL_HEX) {
            float sb = b*0.72f;
            isoBlock(ox+(b-sb)*0.5f, oy+(b-sb)*0.5f, sb, bc, 0.55f);
            return;
        }

        // ---- DEFAULT: full isometric cube ----
        isoBlock(ox, oy, b, bc);
    };

    // Helper: draw a slot background
    auto drawSlotBg = [&](float sx, float sy, glm::vec3 col) {
        std::vector<float> sv;
        pushQuad(sv, sx, sy, sx + slotSize, sy + slotSize, col);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
        setVec3(shaderProgram, "objectColor", col);
        glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 11);
    };

    if (inventoryOpen) {
        // ===== CENTERED INVENTORY PANEL =====
        // Layout (bottom to top inside panel):
        //   Hotbar (1x9)  |  gap  |  Storage (3x9)  |  gap  |  Crafting (3x3) + Arrow + Output
        float panelPad = 15.0f;
        float sectionGap = 14.0f;
        float craftRowW = 3 * slotStep - slotGap;  // 128 (3x3 grid width)
        float arrowW = 30.0f;
        float arrowGapL = 10.0f, arrowGapR = 10.0f;
        float outputW = slotSize; // 40

        float panelW = panelPad + totalW + panelPad; // 392 + 30 = 422
        float panelH = panelPad + slotSize + sectionGap + 3 * slotStep - slotGap + sectionGap + 3 * slotStep - slotGap + panelPad;
        // = 15 + 40 + 14 + 128 + 14 + 128 + 15 = 354
        float panelX = (screenW - panelW) / 2.0f;
        float panelY = (screenH - panelH) / 2.0f;

        // Darken screen
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        setFloat(shaderProgram, "alpha", 0.6f);
        {
            std::vector<float> dv;
            pushQuad(dv, 0, 0, screenW, screenH, glm::vec3(0.0f));
            glBufferSubData(GL_ARRAY_BUFFER, 0, dv.size() * sizeof(float), dv.data());
            setVec3(shaderProgram, "objectColor", glm::vec3(0.0f));
            glDrawArrays(GL_TRIANGLES, 0, (int)dv.size() / 11);
        }
        setFloat(shaderProgram, "alpha", 1.0f);
        glDisable(GL_BLEND);

        // Panel background
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            setFloat(shaderProgram, "alpha", 0.85f);
            std::vector<float> pv;
            pushQuad(pv, panelX, panelY, panelX + panelW, panelY + panelH, glm::vec3(0.15f, 0.13f, 0.12f));
            glBufferSubData(GL_ARRAY_BUFFER, 0, pv.size() * sizeof(float), pv.data());
            setVec3(shaderProgram, "objectColor", glm::vec3(0.15f, 0.13f, 0.12f));
            glDrawArrays(GL_TRIANGLES, 0, (int)pv.size() / 11);
            setFloat(shaderProgram, "alpha", 1.0f);
            glDisable(GL_BLEND);
        }

        // Panel border
        {
            std::vector<float> bv;
            float b = 2.0f;
            glm::vec3 borderCol(0.4f, 0.35f, 0.3f);
            pushQuad(bv, panelX, panelY, panelX + panelW, panelY + b, borderCol); // bottom
            pushQuad(bv, panelX, panelY + panelH - b, panelX + panelW, panelY + panelH, borderCol); // top
            pushQuad(bv, panelX, panelY, panelX + b, panelY + panelH, borderCol); // left
            pushQuad(bv, panelX + panelW - b, panelY, panelX + panelW, panelY + panelH, borderCol); // right
            glBufferSubData(GL_ARRAY_BUFFER, 0, bv.size() * sizeof(float), bv.data());
            setVec3(shaderProgram, "objectColor", borderCol);
            glDrawArrays(GL_TRIANGLES, 0, (int)bv.size() / 11);
        }

        // Grid left edge (all 9-wide grids start here)
        float gridX = panelX + panelPad;

        // ---- HOTBAR (bottom of panel) ----
        float hotbarY = panelY + panelPad;
        for (int i = 0; i < HOTBAR_SIZE; i++) {
            float sx = gridX + i * slotStep;
            glm::vec3 bgCol = (i == hotbarSlot) ? glm::vec3(0.9f, 0.9f, 0.3f) : glm::vec3(0.3f, 0.3f, 0.3f);
            drawSlotBg(sx, hotbarY, bgCol);
            drawSlotItem(sx, hotbarY, playerInventory[i].type);
        }

        // ---- Separator line ----
        {
            std::vector<float> sv;
            float sepY = hotbarY + slotSize + sectionGap / 2 - 1;
            pushQuad(sv, gridX, sepY, gridX + totalW, sepY + 2, glm::vec3(0.35f, 0.3f, 0.25f));
            glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
            setVec3(shaderProgram, "objectColor", glm::vec3(0.35f, 0.3f, 0.25f));
            glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 11);
        }

        // ---- STORAGE GRID (3x9, middle) ----
        float storageY = hotbarY + slotSize + sectionGap;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 9; c++) {
                int i = 9 + r * 9 + c; // slots 9 to 35
                float sx = gridX + c * slotStep;
                float sy = storageY + r * slotStep;
                drawSlotBg(sx, sy, glm::vec3(0.3f));
                drawSlotItem(sx, sy, playerInventory[i].type);
            }
        }

        // ---- Separator line ----
        {
            std::vector<float> sv;
            float sepY = storageY + 3 * slotStep - slotGap + sectionGap / 2 - 1;
            pushQuad(sv, gridX, sepY, gridX + totalW, sepY + 2, glm::vec3(0.35f, 0.3f, 0.25f));
            glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
            setVec3(shaderProgram, "objectColor", glm::vec3(0.35f, 0.3f, 0.25f));
            glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 11);
        }

        // ---- CRAFTING AREA (top of panel) ----
        float craftY = storageY + 3 * slotStep - slotGap + sectionGap;
        // Center crafting row: 3x3 grid + gap + arrow + gap + output
        float craftTotalW = craftRowW + arrowGapL + arrowW + arrowGapR + outputW;
        float craftOffX = gridX + (totalW - craftTotalW) / 2.0f;

        // "Recipe Book" toggle button
        float bookBtnX = craftOffX - 40.0f;
        float bookBtnY = craftY + slotStep;
        {
            std::vector<float> bv;
            glm::vec3 col = recipeBookOpen ? glm::vec3(0.2f, 0.6f, 0.2f) : glm::vec3(0.4f, 0.4f, 0.8f);
            pushQuad(bv, bookBtnX, bookBtnY, bookBtnX + 24.0f, bookBtnY + 24.0f, col);
            glBufferSubData(GL_ARRAY_BUFFER, 0, bv.size() * sizeof(float), bv.data());
            setVec3(shaderProgram, "objectColor", col);
            glDrawArrays(GL_TRIANGLES, 0, (int)bv.size() / 11);
        }

        // "Crafting" label bar
        {
            std::vector<float> lv;
            pushQuad(lv, craftOffX, craftY + 3 * slotStep - slotGap + 4, craftOffX + craftRowW, craftY + 3 * slotStep - slotGap + 18, glm::vec3(0.5f, 0.35f, 0.15f));
            glBufferSubData(GL_ARRAY_BUFFER, 0, lv.size() * sizeof(float), lv.data());
            setVec3(shaderProgram, "objectColor", glm::vec3(0.5f, 0.35f, 0.15f));
            glDrawArrays(GL_TRIANGLES, 0, (int)lv.size() / 11);
        }

        // 3x3 crafting grid
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                int i = r * 3 + c;
                float sx = craftOffX + c * slotStep;
                float sy = craftY + (2 - r) * slotStep; // row 0 at top
                drawSlotBg(sx, sy, glm::vec3(0.25f, 0.22f, 0.18f));
                drawSlotItem(sx, sy, craftingGrid[i].type);
            }
        }

        // ==== RECIPE BOOK PANEL ====
        if (recipeBookOpen) {
            float rbW = 180.0f;
            float rbH = panelH;
            float rbX = panelX - rbW - 10.0f;
            float rbY = panelY;

            // Build filtered recipe list based on search text
            std::vector<int> filtered;
            for (int i = 0; i < NUM_RECIPES; i++) {
                if (recipeSearchText.empty()) {
                    filtered.push_back(i);
                } else {
                    std::string name = getBlockName(recipes[i].resultType);
                    // case-insensitive contains check
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
            if (recipePage >= totalPages && totalPages > 0) recipePage = totalPages - 1;
            if (totalPages == 0) recipePage = 0;

            auto drawRect = [&](float x, float y, float w, float h, glm::vec3 c) {
                std::vector<float> rv;
                pushQuad(rv, x, y, x+w, y+h, c);
                glBufferSubData(GL_ARRAY_BUFFER, 0, rv.size() * sizeof(float), rv.data());
                setVec3(shaderProgram, "objectColor", c);
                glDrawArrays(GL_TRIANGLES, 0, (int)rv.size() / 11);
            };

            // Panel background
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                setFloat(shaderProgram, "alpha", 0.85f);
                std::vector<float> pv;
                pushQuad(pv, rbX, rbY, rbX + rbW, rbY + rbH, glm::vec3(0.15f, 0.13f, 0.12f));
                glBufferSubData(GL_ARRAY_BUFFER, 0, pv.size() * sizeof(float), pv.data());
                setVec3(shaderProgram, "objectColor", glm::vec3(0.15f, 0.13f, 0.12f));
                glDrawArrays(GL_TRIANGLES, 0, (int)pv.size() / 11);
                setFloat(shaderProgram, "alpha", 1.0f);
                glDisable(GL_BLEND);
            }

            // Panel border
            {
                std::vector<float> bv;
                float b = 2.0f;
                glm::vec3 borderCol(0.4f, 0.35f, 0.3f);
                pushQuad(bv, rbX, rbY, rbX + rbW, rbY + b, borderCol);
                pushQuad(bv, rbX, rbY + rbH - b, rbX + rbW, rbY + rbH, borderCol);
                pushQuad(bv, rbX, rbY, rbX + b, rbY + rbH, borderCol);
                pushQuad(bv, rbX + rbW - b, rbY, rbX + rbW, rbY + rbH, borderCol);
                glBufferSubData(GL_ARRAY_BUFFER, 0, bv.size() * sizeof(float), bv.data());
                setVec3(shaderProgram, "objectColor", borderCol);
                glDrawArrays(GL_TRIANGLES, 0, (int)bv.size() / 11);
            }

            // ---- Search box (top of panel) ----
            float sbX = rbX + 8.0f;
            float sbY = rbY + rbH - 26.0f;
            float sbW = rbW - 16.0f;
            float sbH = 18.0f;
            bool isSearching = !recipeSearchText.empty();
            // box bg: dark gray normally, golden when active
            drawRect(sbX, sbY, sbW, sbH, isSearching ? glm::vec3(0.25f, 0.2f, 0.05f) : glm::vec3(0.12f, 0.12f, 0.12f));
            // box border: bright gold when active, dim otherwise
            glm::vec3 sbBorder = isSearching ? glm::vec3(0.85f, 0.7f, 0.1f) : glm::vec3(0.35f, 0.35f, 0.35f);
            drawRect(sbX,          sbY,          sbW, 1.5f, sbBorder);
            drawRect(sbX,          sbY+sbH-1.5f, sbW, 1.5f, sbBorder);
            drawRect(sbX,          sbY,          1.5f, sbH,  sbBorder);
            drawRect(sbX+sbW-1.5f, sbY,          1.5f, sbH,  sbBorder);

            // Draw one small colored square per typed character (shows search activity)
            float dotSize = 7.0f;
            float dotGap  = 2.0f;
            float dotStartX = sbX + 4.0f;
            float dotY = sbY + (sbH - dotSize) * 0.5f;
            for (int ci = 0; ci < (int)recipeSearchText.size() && ci < 20; ci++) {
                // Map each character to a hue so different letters look different
                char ch = recipeSearchText[ci];
                float hue = (float)((ch - 'a' + 26) % 26) / 26.0f; // 0..1
                // HSV->RGB simple conversion
                int hi = (int)(hue * 6.0f);
                float f = hue * 6.0f - hi;
                float q = 1.0f - f, t = f;
                glm::vec3 dotCol;
                switch (hi % 6) {
                    case 0: dotCol = glm::vec3(1,t,0); break;
                    case 1: dotCol = glm::vec3(q,1,0); break;
                    case 2: dotCol = glm::vec3(0,1,t); break;
                    case 3: dotCol = glm::vec3(0,q,1); break;
                    case 4: dotCol = glm::vec3(t,0,1); break;
                    default:dotCol = glm::vec3(1,0,q); break;
                }
                drawRect(dotStartX + ci * (dotSize + dotGap), dotY, dotSize, dotSize, dotCol);
            }

            // Match count indicator: small bar below search box
            // width proportional to filtered.size() / NUM_RECIPES
            if (isSearching) {
                float barW = (filtered.empty() ? 0.0f : ((float)filtered.size() / NUM_RECIPES) * sbW);
                glm::vec3 barCol = filtered.empty() ? glm::vec3(0.7f, 0.1f, 0.1f) : glm::vec3(0.2f, 0.7f, 0.2f);
                drawRect(sbX, sbY - 4.0f, barW > 2.0f ? barW : 2.0f, 3.0f, barCol);
            }

            // ---- Pagination Controls (bottom strip) ----
            float btnY = rbY + 15.0f;
            float btnW = 30.0f;
            float btnH = 20.0f;
            float prevX = rbX + 30.0f;
            float nextX = rbX + rbW - 30.0f - btnW;
            bool hasPrev = recipePage > 0;
            bool hasNext = recipePage < totalPages - 1;
            drawRect(prevX, btnY, btnW, btnH, hasPrev ? glm::vec3(0.5f,0.5f,0.5f) : glm::vec3(0.25f,0.25f,0.25f));
            drawRect(nextX, btnY, btnW, btnH, hasNext ? glm::vec3(0.5f,0.5f,0.5f) : glm::vec3(0.25f,0.25f,0.25f));
            // Page indicator dots between buttons
            if (totalPages > 1) {
                float dotR = 3.0f;
                float dotsW = totalPages * (dotR*2 + 2.0f);
                float dotsStartX = rbX + rbW * 0.5f - dotsW * 0.5f;
                for (int p = 0; p < totalPages && p < 10; p++) {
                    glm::vec3 dc = (p == recipePage) ? glm::vec3(0.9f,0.8f,0.2f) : glm::vec3(0.35f,0.35f,0.35f);
                    drawRect(dotsStartX + p*(dotR*2+2.0f), btnY + btnH*0.5f - dotR, dotR*2, dotR*2, dc);
                }
            }

            // ---- Draw Recipes (filtered, 4 per page) ----
            int startIdx = recipePage * 4;
            float startY = sbY - 18.0f - 60.0f; // start just below search box
            for (int i = 0; i < 4; i++) {
                int fi = startIdx + i;
                if (fi >= (int)filtered.size()) break;
                int rIdx = filtered[fi];

                float rowY = startY - i * 70.0f;
                if (rowY < rbY + 40.0f) break; // don't overlap pagination

                // recipe row bg — highlighted gold if it matches search
                glm::vec3 rowBg = isSearching ? glm::vec3(0.25f, 0.22f, 0.08f) : glm::vec3(0.2f, 0.2f, 0.2f);
                drawRect(rbX + 10, rowY, rbW - 20, 60, rowBg);

                float mSlot = 12.0f;
                float mGap  = 2.0f;
                float mStep = mSlot + mGap;
                float gX = rbX + 18.0f;
                float gY = rowY + 9.0f;

                auto drawMiniItem = [&](float sx, float sy, int bType) {
                    if (bType == BLOCK_AIR) return;
                    std::vector<float> iv;
                    glm::vec3 bc = getBlockColor(bType);
                    BlockShape mShape = getBlockProps(bType).shape;
                    GLuint mTex = getBlockTexture(bType);
                    float b = mSlot;

                    auto mFlush = [&]() {
                        if (iv.empty()) return;
                        glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size()*sizeof(float), iv.data());
                        glDrawArrays(GL_TRIANGLES, 0, (int)iv.size()/11);
                        iv.clear();
                    };
                    auto mIso = [&](float _ox, float _oy, float _b, glm::vec3 t, float shf=0.62f){
                        if (mTex) drawIsoBlockTex(mTex, _ox, _oy, _b, t, shf);
                        else { pushIsoBlock(iv, _ox, _oy, _b, t, shf); mFlush(); }
                    };

                    if (mShape == SHAPE_DOOR) {
                        GLuint doorTex = (bType==BLOCK_DOOR_OAK && texItemOakDoor) ? texItemOakDoor
                                       : (bType==BLOCK_DOOR_IRON && texItemIronDoor) ? texItemIronDoor : 0;
                        float dw=b*0.50f, dh=b*0.96f;
                        float dx=sx+(b-dw)*0.5f, dy=sy+(b-dh)*0.5f;
                        if (doorTex) {
                            pushQuadUV(iv,dx,dy,dx+dw,dy+dh,glm::vec3(1.0f),0,0,1,1);
                            glBufferSubData(GL_ARRAY_BUFFER,0,iv.size()*sizeof(float),iv.data());
                            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, doorTex);
                            setInt(shaderProgram,"textureMode",1);
                            glDrawArrays(GL_TRIANGLES,0,(int)iv.size()/11);
                            setInt(shaderProgram,"textureMode",0); iv.clear(); return;
                        }
                        pushQuad(iv,dx,dy,dx+dw,dy+dh,bc); mFlush();
                        glm::vec3 fr=bc*0.55f;
                        pushQuad(iv,dx,dy,dx+dw,dy+1.5f,fr); pushQuad(iv,dx,dy+dh-1.5f,dx+dw,dy+dh,fr);
                        pushQuad(iv,dx,dy,dx+1.5f,dy+dh,fr); pushQuad(iv,dx+dw-1.5f,dy,dx+dw,dy+dh,fr);
                        mFlush(); return;
                    }
                    if (mShape == SHAPE_TRAPDOOR) { mIso(sx, sy+b*0.28f, b, bc, 0.20f); return; }
                    if (mShape == SHAPE_SLAB)     { mIso(sx, sy, b, bc, 0.32f);          return; }
                    if (mShape == SHAPE_STAIR) {
                        mIso(sx, sy, b, bc, 0.30f);
                        mIso(sx+b*0.25f, sy+b*0.30f*0.62f, b*0.5f, bc*0.92f, 0.55f); return;
                    }
                    if (mShape == SHAPE_CARPET)  { mIso(sx, sy, b, bc, 0.10f); return; }
                    if (mShape == SHAPE_FENCE) {
                        float cx2=sx+b*0.5f, pw=b*0.20f;
                        if (mTex) {
                            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, mTex);
                            setInt(shaderProgram,"textureMode",2);
                            pushQuadUV(iv,cx2-pw*0.5f,sy,cx2+pw*0.5f,sy+b,bc,0.4f,0,0.6f,1);
                            pushQuadUV(iv,sx+b*0.08f,sy+b*0.28f,sx+b*0.92f,sy+b*0.28f+pw,bc*0.85f,0,0.4f,1,0.6f);
                            pushQuadUV(iv,sx+b*0.08f,sy+b*0.62f,sx+b*0.92f,sy+b*0.62f+pw,bc*0.85f,0,0.4f,1,0.6f);
                            glBufferSubData(GL_ARRAY_BUFFER,0,iv.size()*sizeof(float),iv.data());
                            glDrawArrays(GL_TRIANGLES,0,(int)iv.size()/11);
                            setInt(shaderProgram,"textureMode",0); iv.clear();
                        } else {
                            pushQuad(iv,cx2-pw*0.5f,sy,cx2+pw*0.5f,sy+b,bc);
                            pushQuad(iv,sx+b*0.08f,sy+b*0.28f,sx+b*0.92f,sy+b*0.28f+pw,bc*0.8f);
                            pushQuad(iv,sx+b*0.08f,sy+b*0.62f,sx+b*0.92f,sy+b*0.62f+pw,bc*0.8f);
                            mFlush();
                        }
                        return;
                    }
                    if (mShape == SHAPE_PANE || mShape == SHAPE_FLAT_PANEL) {
                        float pw=b*0.16f, cx2=sx+b*0.5f;
                        glm::vec3 pc = getBlockProps(bType).isTransparent
                            ? glm::mix(bc,glm::vec3(0.7f,0.9f,1.0f),0.4f) : bc;
                        if (mTex) {
                            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, mTex);
                            setInt(shaderProgram,"textureMode",2);
                            pushQuadUV(iv,cx2-pw*0.5f,sy+b*0.04f,cx2+pw*0.5f,sy+b*0.96f,pc,0,0,1,1);
                            pushQuadUV(iv,sx+b*0.04f,sy+b*0.47f,sx+b*0.96f,sy+b*0.53f,pc*0.85f,0,0,1,1);
                            glBufferSubData(GL_ARRAY_BUFFER,0,iv.size()*sizeof(float),iv.data());
                            glDrawArrays(GL_TRIANGLES,0,(int)iv.size()/11);
                            setInt(shaderProgram,"textureMode",0); iv.clear();
                        } else {
                            pushQuad(iv,cx2-pw*0.5f,sy+b*0.04f,cx2+pw*0.5f,sy+b*0.96f,pc);
                            pushQuad(iv,sx+b*0.04f,sy+b*0.47f,sx+b*0.96f,sy+b*0.53f,pc*0.85f);
                            mFlush();
                        }
                        return;
                    }
                    if (mShape == SHAPE_SMALL_HEX) {
                        float sb=b*0.72f;
                        mIso(sx+(b-sb)*0.5f, sy+(b-sb)*0.5f, sb, bc, 0.55f); return;
                    }
                    mIso(sx, sy, b, bc);
                };

                for (int r = 0; r < 3; r++) {
                    for (int c = 0; c < 3; c++) {
                        float sx = gX + c * mStep;
                        float sy = gY + (2-r) * mStep;
                        drawRect(sx, sy, mSlot, mSlot, glm::vec3(0.15f));
                        drawMiniItem(sx, sy, recipes[rIdx].pattern[r*3+c]);
                    }
                }

                // arrow
                float mArrowX = gX + 3*mStep + 8.0f;
                drawRect(mArrowX, gY + mStep, 15.0f, 4.0f, glm::vec3(0.6f));

                // output slot — gold tint when search active
                float outX = mArrowX + 22.0f;
                float outSize = 25.0f;
                float outY2 = gY + (3*mStep - outSize)/2.0f;
                drawRect(outX, outY2, outSize, outSize, isSearching ? glm::vec3(0.5f, 0.45f, 0.1f) : glm::vec3(0.4f, 0.4f, 0.3f));

                float oldSlotSize = slotSize;
                slotSize = outSize;
                drawSlotItem(outX - 6.0f, outY2 - 6.0f, recipes[rIdx].resultType);
                slotSize = oldSlotSize;
            }

            // "No results" indicator — red bar across middle
            if (isSearching && filtered.empty()) {
                drawRect(rbX + 10, rbY + rbH * 0.5f - 3.0f, rbW - 20, 6.0f, glm::vec3(0.7f, 0.1f, 0.1f));
            }
        }

        // Arrow indicator
        float arrowX = craftOffX + craftRowW + arrowGapL;
        float arrowCenterY = craftY + slotStep; // vertically centered with middle row
        {
            std::vector<float> av;
            pushQuad(av, arrowX, arrowCenterY + slotSize * 0.35f, arrowX + 20.0f, arrowCenterY + slotSize * 0.65f, glm::vec3(0.7f));
            pushQuad(av, arrowX + 16.0f, arrowCenterY + slotSize * 0.2f, arrowX + 30.0f, arrowCenterY + slotSize * 0.8f, glm::vec3(0.7f));
            glBufferSubData(GL_ARRAY_BUFFER, 0, av.size() * sizeof(float), av.data());
            setVec3(shaderProgram, "objectColor", glm::vec3(0.7f));
            glDrawArrays(GL_TRIANGLES, 0, (int)av.size() / 11);
        }

        // Output slot
        float outX = arrowX + arrowW + arrowGapR;
        float outY = arrowCenterY;
        {
            glm::vec3 outBg = (craftingOutput.type != BLOCK_AIR) ? glm::vec3(0.2f, 0.45f, 0.2f) : glm::vec3(0.35f, 0.3f, 0.25f);
            drawSlotBg(outX, outY, outBg);
            drawSlotItem(outX, outY, craftingOutput.type);
        }

        // Draw Dragged item at mouse cursor
        if (draggedSlot.type != BLOCK_AIR) {
            float mx = lastMouseX;
            float my = screenH - lastMouseY;
            drawSlotItem(mx - slotSize / 2, my - slotSize / 2, draggedSlot.type);
        }
    } else {
        // ---- HOTBAR ONLY (not in inventory mode, at bottom of screen) ----
        float hbX = (screenW - totalW) / 2.0f;
        float hbY = 10.0f;
        for (int i = 0; i < HOTBAR_SIZE; i++) {
            float sx = hbX + i * slotStep;
            glm::vec3 bgCol = (i == hotbarSlot) ? glm::vec3(0.9f, 0.9f, 0.3f) : glm::vec3(0.3f, 0.3f, 0.3f);
            drawSlotBg(sx, hbY, bgCol);
            drawSlotItem(sx, hbY, playerInventory[i].type);
        }
    }

    // Helper to draw a single bar
    auto drawBar = [&](float bx, float by, float bw, float bh, float frac, glm::vec3 color) {
        // Background
        std::vector<float> bv;
        pushQuad(bv, bx - 1, by - 1, bx + bw + 1, by + bh + 1, glm::vec3(0.15f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, bv.size() * sizeof(float), bv.data());
        setVec3(shaderProgram, "objectColor", glm::vec3(0.15f));
        glDrawArrays(GL_TRIANGLES, 0, (int)bv.size() / 11);
        // Fill
        if (frac > 0.001f) {
            bv.clear();
            pushQuad(bv, bx, by, bx + bw * frac, by + bh, color);
            glBufferSubData(GL_ARRAY_BUFFER, 0, bv.size() * sizeof(float), bv.data());
            setVec3(shaderProgram, "objectColor", color);
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
        setVec3(shaderProgram, "objectColor", glm::vec3(0.4f, 0.0f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 0, (int)dv.size() / 11);
        setFloat(shaderProgram, "alpha", 1.0f);
        glDisable(GL_BLEND);

        // "YOU DIED" text as a bright bar in center
        float txtW = 200.0f, txtH = 30.0f;
        float txtX = (screenW - txtW) / 2.0f, txtY = screenH / 2.0f;
        std::vector<float> tv;
        pushQuad(tv, txtX, txtY, txtX + txtW, txtY + txtH, glm::vec3(0.8f, 0.1f, 0.1f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, tv.size() * sizeof(float), tv.data());
        setVec3(shaderProgram, "objectColor", glm::vec3(0.8f, 0.1f, 0.1f));
        glDrawArrays(GL_TRIANGLES, 0, (int)tv.size() / 11);
    }

    glBindVertexArray(0);
    setBool(shaderProgram, "isHUD", false);
    glEnable(GL_DEPTH_TEST);
}


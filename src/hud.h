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
// Breaking animation — semi-transparent dark overlay in 5 stages
// progress: 0.0 (just started) → 1.0 (about to break)
// =====================================================
void drawBreakOverlay(float progress) {
    if (!hasTarget || progress <= 0.0f) return;

    glm::vec3 pos = hexGridPos(targetCol, targetRow, 0.0f);
    pos.y = targetHeight * HEX_HEIGHT;

    // 5 discrete stages → alpha 0.12, 0.25, 0.40, 0.55, 0.70
    int stage = (int)(progress * 5.0f);
    if (stage > 4) stage = 4;
    float alpha = 0.12f + stage * 0.145f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Draw slightly enlarged dark hex over target block
    setBool(shaderProgram, "isEmissive", true);
    setVec3(shaderProgram, "emissiveColor", glm::vec3(0.0f)); // pure black
    // Use colorTint to make it dark
    setVec3(shaderProgram, "colorTint", glm::vec3(0.0f));
    setFloat(shaderProgram, "colorTintStrength", alpha);

    drawHex(pos, glm::vec3(0, 0, 0), glm::vec3(1.02f, 1.02f, 1.02f));

    setBool(shaderProgram, "isEmissive", false);
    setFloat(shaderProgram, "colorTintStrength", 0.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
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
    // Will be filled dynamically.
    //
    // Was 8192 floats = 124 quads per glBufferSubData. The 5x7 bitmap font emits
    // one quad per lit pixel — up to 35 per character — so a string of roughly
    // nine or more capitals overruns it, and glBufferSubData rejects an oversized
    // write outright rather than truncating: the string silently does not draw.
    // Caught on the Build tab (plan_2 Step 4), where "FIREPLACE" and the panel's
    // subtitle both vanished while every shorter label rendered. Any long HUD
    // string had the same latent limit.
    glBufferData(GL_ARRAY_BUFFER, 65536 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
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

// Draw a block icon as a flat sprite — textured if tex!=0, else solid color.
// Adds a dark right+bottom shadow strip to give a subtle 2.5D feel.
// x1,y1 = bottom-left corner, x2,y2 = top-right corner of the icon area.
void drawFlatIcon(std::vector<float>& v, GLuint tex, float x1, float y1, float x2, float y2,
                  glm::vec3 col, bool tint = false) {
    if (tex) {
        // If tint=true, pass block color for textureMode=2 (texture * color)
        // Otherwise pass white for textureMode=1 (texture only)
        pushQuadUV(v, x1, y1, x2, y2, tint ? col : glm::vec3(1.0f), 0, 0, 1, 1);
    } else {
        // Solid color with a lighter top-left and darker bottom-right strip (fake 2.5D)
        float s = (x2-x1) * 0.1f; // shadow strip width
        pushQuad(v, x1,   y1,   x2,   y2,   col);                          // main face
        pushQuad(v, x2-s, y1,   x2,   y2,   col * 0.55f);                  // right shadow
        pushQuad(v, x1,   y1,   x2,   y1+s, col * 0.55f);                  // bottom shadow
        pushQuad(v, x1,   y2-s, x2,   y2,   glm::min(col*1.3f,glm::vec3(1))); // top highlight
    }
}

// Check if a block type needs color tinting on its texture (grayscale textures like grass, leaves)
bool needsTint(int bType) {
    return bType == BLOCK_GRASS || bType == BLOCK_LEAF || bType == BLOCK_WATER;
}

// =====================================================
// Minimal 5x7 bitmap font — column-major, bit0=top
// 95 printable ASCII chars starting at index 0 = 0x20 (space).
// Each char: 5 bytes, one per column, 7 bits per byte (bit 0 = topmost row).
// =====================================================
static const uint8_t FONT5x7[95][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 0x20 space
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14}, // #
    {0x24,0x2A,0x7F,0x2A,0x12}, // $
    {0x23,0x13,0x08,0x64,0x62}, // %
    {0x36,0x49,0x55,0x22,0x50}, // &
    {0x00,0x05,0x03,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00}, // )
    {0x08,0x2A,0x1C,0x2A,0x08}, // *
    {0x08,0x08,0x3E,0x08,0x08}, // +
    {0x00,0x50,0x30,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02}, // /
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00}, // ;
    {0x08,0x14,0x22,0x41,0x00}, // <
    {0x14,0x14,0x14,0x14,0x14}, // =
    {0x00,0x41,0x22,0x14,0x08}, // >
    {0x02,0x01,0x51,0x09,0x06}, // ?
    {0x32,0x49,0x79,0x41,0x3E}, // @
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x04,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
    {0x00,0x7F,0x41,0x41,0x00}, // [
    {0x02,0x04,0x08,0x10,0x20}, // backslash
    {0x00,0x41,0x41,0x7F,0x00}, // ]
    {0x04,0x02,0x01,0x02,0x04}, // ^
    {0x40,0x40,0x40,0x40,0x40}, // _
    {0x00,0x01,0x02,0x04,0x00}, // `
    {0x20,0x54,0x54,0x54,0x78}, // a
    {0x7F,0x48,0x44,0x44,0x38}, // b
    {0x38,0x44,0x44,0x44,0x20}, // c
    {0x38,0x44,0x44,0x48,0x7F}, // d
    {0x38,0x54,0x54,0x54,0x18}, // e
    {0x08,0x7E,0x09,0x01,0x02}, // f
    {0x08,0x54,0x54,0x54,0x3C}, // g
    {0x7F,0x08,0x04,0x04,0x78}, // h
    {0x00,0x44,0x7D,0x40,0x00}, // i
    {0x20,0x40,0x44,0x3D,0x00}, // j
    {0x00,0x7F,0x10,0x28,0x44}, // k
    {0x00,0x41,0x7F,0x40,0x00}, // l
    {0x7C,0x04,0x18,0x04,0x78}, // m
    {0x7C,0x08,0x04,0x04,0x78}, // n
    {0x38,0x44,0x44,0x44,0x38}, // o
    {0x7C,0x14,0x14,0x14,0x08}, // p
    {0x08,0x14,0x14,0x18,0x7C}, // q
    {0x7C,0x08,0x04,0x04,0x08}, // r
    {0x48,0x54,0x54,0x54,0x20}, // s
    {0x04,0x3F,0x44,0x40,0x20}, // t
    {0x3C,0x40,0x40,0x20,0x7C}, // u
    {0x1C,0x20,0x40,0x20,0x1C}, // v
    {0x3C,0x40,0x30,0x40,0x3C}, // w
    {0x44,0x28,0x10,0x28,0x44}, // x
    {0x0C,0x50,0x50,0x50,0x3C}, // y
    {0x44,0x64,0x54,0x4C,0x44}, // z
    {0x00,0x08,0x36,0x41,0x00}, // {
    {0x00,0x00,0x7F,0x00,0x00}, // |
    {0x00,0x41,0x36,0x08,0x00}, // }
    {0x08,0x04,0x08,0x10,0x08}, // ~
};

// Draw a string into a HUD vertex buffer at (x, y = bottom-left).
// pixH = total character height in screen pixels.
float hudDrawString(std::vector<float>& v, const char* str, float x, float y,
                    float pixH, glm::vec3 color) {
    if (!str) return 0.0f;
    float pxH = pixH / 7.0f;               // height of one pixel row
    float pxW = pxH;                        // square pixels
    float charAdv = pxW * 6.0f;            // 5 cols + 1 gap
    float cx = x;
    for (const char* p = str; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c < 0x20 || c > 0x7E) { cx += charAdv; continue; }
        const uint8_t* glyph = FONT5x7[c - 0x20];
        for (int col = 0; col < 5; col++) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row)) {
                    float px = cx + col * pxW;
                    float py = y  + (6 - row) * pxH; // bit0=top, but y=0 is bottom in GL
                    pushQuad(v, px, py, px + pxW, py + pxH, color);
                }
            }
        }
        cx += charAdv;
    }
    return cx - x;
}

// Measure string width without drawing
float hudMeasureString(const char* str, float pixH) {
    if (!str) return 0.0f;
    float pxH = pixH / 7.0f;
    float charAdv = pxH * 6.0f;
    float w = 0.0f;
    for (const char* p = str; *p; ++p) {
        if ((unsigned char)*p >= 0x20) w += charAdv;
    }
    return w;
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
        float pad2 = 4.0f;
        float b    = slotSize - pad2 * 2.0f;
        float ox   = sx + pad2, oy = sy + pad2;
        glm::vec3 bc = getBlockColor(bType);
        BlockShape shape = getBlockProps(bType).shape;
        GLuint blockTex = getBlockTexture(bType);

        bool tint = needsTint(bType);

        // Helper: upload iv and draw, then clear
        auto go = [&]() {
            if (iv.empty()) return;
            if (blockTex) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, blockTex);
                setInt(shaderProgram, "textureMode", tint ? 2 : 1);
            }
            glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size()*sizeof(float), iv.data());
            glDrawArrays(GL_TRIANGLES, 0, (int)iv.size()/11);
            if (blockTex) setInt(shaderProgram, "textureMode", 0);
            iv.clear();
        };

        // ---- ITEM (tools, sticks, etc.): always flat sprite, full slot ----
        if (getBlockProps(bType).isItem) {
            drawFlatIcon(iv, blockTex, ox, oy, ox+b, oy+b, bc, tint);
            go();
            return;
        }

        // ---- DOOR: tall panel ----
        if (shape == SHAPE_DOOR) {
            GLuint doorTex = (bType==BLOCK_DOOR_OAK  && texItemOakDoor)  ? texItemOakDoor
                           : (bType==BLOCK_DOOR_IRON && texItemIronDoor) ? texItemIronDoor
                           : blockTex;
            float dw = b*0.55f, dh = b*0.98f;
            float dx = ox+(b-dw)*0.5f, dy = oy+(b-dh)*0.5f;
            if (doorTex) {
                pushQuadUV(iv, dx, dy, dx+dw, dy+dh, glm::vec3(1.0f), 0,0,1,1);
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, doorTex);
                setInt(shaderProgram,"textureMode",1);
                glBufferSubData(GL_ARRAY_BUFFER,0,iv.size()*sizeof(float),iv.data());
                glDrawArrays(GL_TRIANGLES,0,(int)iv.size()/11);
                setInt(shaderProgram,"textureMode",0); iv.clear();
            } else {
                drawFlatIcon(iv, 0, dx, dy, dx+dw, dy+dh, bc, tint); go();
                glm::vec3 knob(0.9f,0.82f,0.25f);
                pushQuad(iv, dx+dw*0.72f-2.0f, dy+dh*0.5f-2.0f, dx+dw*0.72f+2.0f, dy+dh*0.5f+2.0f, knob);
                go();
            }
            return;
        }

        // ---- TRAPDOOR: flat wide panel (bottom half of slot) ----
        if (shape == SHAPE_TRAPDOOR) {
            float th = b*0.45f;
            float ty = oy + (b-th)*0.5f;
            drawFlatIcon(iv, blockTex, ox, ty, ox+b, ty+th, bc, tint); go();
            return;
        }

        // ---- SLAB: bottom half ----
        if (shape == SHAPE_SLAB) {
            float sh = b*0.5f;
            drawFlatIcon(iv, blockTex, ox, oy, ox+b, oy+sh, bc, tint); go();
            return;
        }

        // ---- STAIR: L-shape (slab bottom + step right half top) ----
        if (shape == SHAPE_STAIR) {
            float sh = b*0.5f;
            drawFlatIcon(iv, blockTex, ox,       oy,    ox+b,    oy+sh,  bc, tint);        // bottom
            drawFlatIcon(iv, blockTex, ox+b*0.5f, oy+sh, ox+b,   oy+b,   bc*0.88f, tint); // top-right step
            go();
            return;
        }

        // ---- CARPET: thin strip at bottom ----
        if (shape == SHAPE_CARPET) {
            float th = b*0.18f;
            drawFlatIcon(iv, blockTex, ox, oy, ox+b, oy+th, bc, tint); go();
            return;
        }

        // ---- FENCE: post + two rails ----
        if (shape == SHAPE_FENCE) {
            float pw = b*0.20f, cx2 = ox+b*0.5f;
            drawFlatIcon(iv, blockTex, cx2-pw*0.5f, oy,       cx2+pw*0.5f, oy+b,       bc, tint);
            drawFlatIcon(iv, blockTex, ox+b*0.05f,  oy+b*0.3f, ox+b*0.95f, oy+b*0.3f+pw*0.9f, bc*0.8f, tint);
            drawFlatIcon(iv, blockTex, ox+b*0.05f,  oy+b*0.62f,ox+b*0.95f, oy+b*0.62f+pw*0.9f,bc*0.8f, tint);
            go();
            return;
        }

        // ---- PANE / FLAT PANEL: thin cross ----
        if (shape == SHAPE_PANE || shape == SHAPE_FLAT_PANEL) {
            float pw = b*0.15f, cx2 = ox+b*0.5f;
            glm::vec3 pc = getBlockProps(bType).isTransparent
                ? glm::mix(bc,glm::vec3(0.6f,0.85f,1.0f),0.5f) : bc;
            drawFlatIcon(iv, blockTex, cx2-pw*0.5f, oy+b*0.05f, cx2+pw*0.5f, oy+b*0.95f, pc, tint);
            drawFlatIcon(iv, blockTex, ox+b*0.05f, oy+b*0.45f, ox+b*0.95f,  oy+b*0.55f, pc*0.8f, tint);
            go();
            return;
        }

        // ---- SMALL HEX: small centered square ----
        if (shape == SHAPE_SMALL_HEX) {
            float sb = b*0.7f, off = (b-sb)*0.5f;
            drawFlatIcon(iv, blockTex, ox+off, oy+off, ox+off+sb, oy+off+sb, bc, tint); go();
            return;
        }

        // ---- DEFAULT: full flat icon ----
        drawFlatIcon(iv, blockTex, ox, oy, ox+b, oy+b, bc, tint);
        go();
    };

    // Helper: draw a slot background
    auto drawSlotBg = [&](float sx, float sy, glm::vec3 col) {
        std::vector<float> sv;
        pushQuad(sv, sx, sy, sx + slotSize, sy + slotSize, col);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
        setVec3(shaderProgram, "objectColor", col);
        glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 11);
    };

    // Helper: draw item stack count (bottom-right of slot, Minecraft-style)
    auto drawSlotCount = [&](float sx, float sy, int count) {
        if (count <= 1) return; // don't show "1"
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", count);
        float textH = 10.0f;
        float tw = hudMeasureString(buf, textH);
        float tx = sx + slotSize - tw - 2.0f;
        float ty = sy + 2.0f;
        // Shadow
        std::vector<float> sv;
        hudDrawString(sv, buf, tx + 1.0f, ty - 1.0f, textH, glm::vec3(0.1f, 0.1f, 0.1f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
        glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 11);
        // Text
        sv.clear();
        hudDrawString(sv, buf, tx, ty, textH, glm::vec3(1.0f, 1.0f, 1.0f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
        glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 11);
    };

    if (inventoryOpen) {
        // ===== CENTERED INVENTORY PANEL =====
        // Layout (bottom to top inside panel):
        //   Hotbar (1x9)  |  gap  |  Storage (3x9)  |  gap  |  Crafting (3x3) + Arrow + Output
        float panelPad = 15.0f;
        float sectionGap = 14.0f;
        float arrowW = 30.0f;
        float arrowGapL = 10.0f, arrowGapR = 10.0f;
        float outputW = slotSize; // 40

        float panelW = panelPad + totalW + panelPad; // 392 + 30 = 422
        float craftAreaH = 3 * slotStep - slotGap; // always 3x3 height
        float panelH = panelPad + slotSize + sectionGap + 3 * slotStep - slotGap + sectionGap + craftAreaH + panelPad;
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

        // Mouse position for hover highlight (GLFW top=0, HUD bottom=0)
        float mx = lastMouseX;
        float my = screenH - lastMouseY;
        int hoveredItemType = BLOCK_AIR; // track for tooltip

        // Helper: check if mouse is inside a slot and draw highlight overlay
        auto drawSlotHighlight = [&](float sx, float sy, int itemType = BLOCK_AIR) {
            if (mx >= sx && mx <= sx + slotSize && my >= sy && my <= sy + slotSize) {
                if (itemType != BLOCK_AIR) hoveredItemType = itemType;
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                setFloat(shaderProgram, "alpha", 0.3f);
                std::vector<float> hv;
                pushQuad(hv, sx, sy, sx + slotSize, sy + slotSize, glm::vec3(1.0f));
                glBufferSubData(GL_ARRAY_BUFFER, 0, hv.size() * sizeof(float), hv.data());
                setVec3(shaderProgram, "objectColor", glm::vec3(1.0f));
                glDrawArrays(GL_TRIANGLES, 0, (int)hv.size() / 11);
                setFloat(shaderProgram, "alpha", 1.0f);
                glDisable(GL_BLEND);
            }
        };

        // ---- HOTBAR (bottom of panel) ----
        float hotbarY = panelY + panelPad;
        for (int i = 0; i < HOTBAR_SIZE; i++) {
            float sx = gridX + i * slotStep;
            glm::vec3 bgCol = (i == hotbarSlot) ? glm::vec3(0.9f, 0.9f, 0.3f) : glm::vec3(0.3f, 0.3f, 0.3f);
            drawSlotBg(sx, hotbarY, bgCol);
            drawSlotItem(sx, hotbarY, playerInventory[i].type);
            drawSlotCount(sx, hotbarY, playerInventory[i].count);
            drawSlotHighlight(sx, hotbarY, playerInventory[i].type);
            // Durability bar
            int dur = playerInventory[i].durability;
            int maxDur = getMaxDurability(playerInventory[i].type);
            if (dur > 0 && maxDur > 0) {
                float frac = (float)dur / (float)maxDur;
                glm::vec3 durCol = (frac > 0.5f) ? glm::vec3(0.1f, 0.85f, 0.1f)
                                 : (frac > 0.2f) ? glm::vec3(0.9f, 0.8f, 0.1f)
                                                 : glm::vec3(0.9f, 0.15f, 0.1f);
                std::vector<float> dv;
                float barW2 = slotSize - 4.0f;
                pushQuad(dv, sx+2, hotbarY+2, sx+2+barW2, hotbarY+5, glm::vec3(0.1f));
                glBufferSubData(GL_ARRAY_BUFFER, 0, dv.size()*sizeof(float), dv.data());
                setVec3(shaderProgram, "objectColor", glm::vec3(0.1f));
                glDrawArrays(GL_TRIANGLES, 0, (int)dv.size()/11);
                dv.clear();
                pushQuad(dv, sx+2, hotbarY+2, sx+2+barW2*frac, hotbarY+5, durCol);
                glBufferSubData(GL_ARRAY_BUFFER, 0, dv.size()*sizeof(float), dv.data());
                setVec3(shaderProgram, "objectColor", durCol);
                glDrawArrays(GL_TRIANGLES, 0, (int)dv.size()/11);
            }
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
                float sy = storageY + (2 - r) * slotStep; // row 0 (slots 9-17) at top
                drawSlotBg(sx, sy, glm::vec3(0.3f));
                drawSlotItem(sx, sy, playerInventory[i].type);
                drawSlotCount(sx, sy, playerInventory[i].count);
                drawSlotHighlight(sx, sy, playerInventory[i].type);
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
        int craftDim = 3; // always 3x3 so all recipes are craftable
        float craftGridW = craftDim * slotStep - slotGap;
        // Center crafting row: grid + gap + arrow + gap + output
        float craftTotalW = craftGridW + arrowGapL + arrowW + arrowGapR + outputW;
        float craftOffX = gridX + (totalW - craftTotalW) / 2.0f;

        // "Recipe Book" toggle button (always visible)
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
            pushQuad(lv, craftOffX, craftY + craftDim * slotStep - slotGap + 4, craftOffX + craftGridW, craftY + craftDim * slotStep - slotGap + 18, glm::vec3(0.5f, 0.35f, 0.15f));
            glBufferSubData(GL_ARRAY_BUFFER, 0, lv.size() * sizeof(float), lv.data());
            setVec3(shaderProgram, "objectColor", glm::vec3(0.5f, 0.35f, 0.15f));
            glDrawArrays(GL_TRIANGLES, 0, (int)lv.size() / 11);
        }

        // Crafting grid (3x3 or 2x2)
        for (int r = 0; r < craftDim; r++) {
            for (int c = 0; c < craftDim; c++) {
                int i = r * 3 + c; // always index into 3x3 grid array
                float sx = craftOffX + c * slotStep;
                float sy = craftY + (craftDim - 1 - r) * slotStep; // row 0 at top
                drawSlotBg(sx, sy, glm::vec3(0.25f, 0.22f, 0.18f));
                drawSlotItem(sx, sy, craftingGrid[i].type);
                drawSlotCount(sx, sy, craftingGrid[i].count);
                drawSlotHighlight(sx, sy, craftingGrid[i].type);
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

            // Draw search text using bitmap font
            if (!recipeSearchText.empty()) {
                std::vector<float> tv;
                float tH   = sbH * 0.72f;
                float tY   = sbY + (sbH - tH) * 0.5f;
                float tX   = sbX + 5.0f;
                glm::vec3 textCol = isSearching ? glm::vec3(1.0f, 0.9f, 0.5f) : glm::vec3(0.7f, 0.7f, 0.7f);
                hudDrawString(tv, recipeSearchText.c_str(), tX, tY, tH, textCol);
                // upload and draw text quads
                glBufferSubData(GL_ARRAY_BUFFER, 0, tv.size()*sizeof(float), tv.data());
                setInt(shaderProgram, "textureMode", 0);
                glDrawArrays(GL_TRIANGLES, 0, (int)tv.size()/11);
            } else {
                // Placeholder hint text when empty
                std::vector<float> tv;
                float tH = sbH * 0.60f;
                float tY = sbY + (sbH - tH) * 0.5f;
                hudDrawString(tv, "Search...", sbX + 5.0f, tY, tH, glm::vec3(0.4f, 0.4f, 0.4f));
                glBufferSubData(GL_ARRAY_BUFFER, 0, tv.size()*sizeof(float), tv.data());
                setInt(shaderProgram, "textureMode", 0);
                glDrawArrays(GL_TRIANGLES, 0, (int)tv.size()/11);
            }

            // Blinking cursor at end of text
            {
                float tH  = sbH * 0.72f;
                float tY  = sbY + (sbH - tH) * 0.5f;
                float tX  = sbX + 5.0f;
                float curX = tX + hudMeasureString(recipeSearchText.c_str(), tH);
                float curW = 1.5f, curH = tH;
                // blink at ~1Hz
                if (fmod((float)glfwGetTime(), 1.0f) < 0.5f) {
                    std::vector<float> cv;
                    pushQuad(cv, curX, tY, curX + curW, tY + curH, glm::vec3(1.0f, 0.9f, 0.5f));
                    glBufferSubData(GL_ARRAY_BUFFER, 0, cv.size()*sizeof(float), cv.data());
                    glDrawArrays(GL_TRIANGLES, 0, (int)cv.size()/11);
                }
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

                // drawMiniItem: same flat-sprite logic scaled to mSlot size
                auto drawMiniItem = [&](float sx, float sy, int bType) {
                    if (bType == BLOCK_AIR) return;
                    std::vector<float> iv;
                    glm::vec3 bc = getBlockColor(bType);
                    BlockShape mShape = getBlockProps(bType).shape;
                    GLuint mTex = getBlockTexture(bType);
                    bool mTint = needsTint(bType);
                    float b = mSlot;

                    auto mGo = [&]() {
                        if (iv.empty()) return;
                        if (mTex) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, mTex); setInt(shaderProgram,"textureMode", mTint ? 2 : 1); }
                        glBufferSubData(GL_ARRAY_BUFFER,0,iv.size()*sizeof(float),iv.data());
                        glDrawArrays(GL_TRIANGLES,0,(int)iv.size()/11);
                        if (mTex) setInt(shaderProgram,"textureMode",0);
                        iv.clear();
                    };

                    // Items (tools etc.): full flat sprite
                    if (getBlockProps(bType).isItem) {
                        drawFlatIcon(iv, mTex, sx, sy, sx+b, sy+b, bc, mTint); mGo(); return;
                    }

                    if (mShape == SHAPE_DOOR) {
                        GLuint doorTex = (bType==BLOCK_DOOR_OAK && texItemOakDoor) ? texItemOakDoor
                                       : (bType==BLOCK_DOOR_IRON && texItemIronDoor) ? texItemIronDoor
                                       : mTex;
                        float dw=b*0.55f, dh=b;
                        float dx=sx+(b-dw)*0.5f;
                        if (doorTex) {
                            pushQuadUV(iv,dx,sy,dx+dw,sy+dh,glm::vec3(1.0f),0,0,1,1);
                            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,doorTex);
                            setInt(shaderProgram,"textureMode",1);
                            glBufferSubData(GL_ARRAY_BUFFER,0,iv.size()*sizeof(float),iv.data());
                            glDrawArrays(GL_TRIANGLES,0,(int)iv.size()/11);
                            setInt(shaderProgram,"textureMode",0); iv.clear();
                        } else {
                            drawFlatIcon(iv,0,dx,sy,dx+dw,sy+dh,bc, mTint); mGo();
                        }
                        return;
                    }
                    if (mShape == SHAPE_TRAPDOOR) {
                        drawFlatIcon(iv,mTex,sx,sy+b*0.3f,sx+b,sy+b*0.7f,bc, mTint); mGo(); return;
                    }
                    if (mShape == SHAPE_SLAB) {
                        drawFlatIcon(iv,mTex,sx,sy,sx+b,sy+b*0.5f,bc, mTint); mGo(); return;
                    }
                    if (mShape == SHAPE_STAIR) {
                        drawFlatIcon(iv,mTex,sx,     sy,       sx+b, sy+b*0.5f, bc, mTint);
                        drawFlatIcon(iv,mTex,sx+b*0.5f,sy+b*0.5f, sx+b, sy+b,  bc*0.85f, mTint);
                        mGo(); return;
                    }
                    if (mShape == SHAPE_CARPET) {
                        drawFlatIcon(iv,mTex,sx,sy,sx+b,sy+b*0.22f,bc, mTint); mGo(); return;
                    }
                    if (mShape == SHAPE_FENCE) {
                        float pw=b*0.22f, cx2=sx+b*0.5f;
                        drawFlatIcon(iv,mTex,cx2-pw*0.5f,sy,cx2+pw*0.5f,sy+b,bc, mTint);
                        drawFlatIcon(iv,mTex,sx,sy+b*0.3f,sx+b,sy+b*0.3f+pw*0.85f,bc*0.8f, mTint);
                        drawFlatIcon(iv,mTex,sx,sy+b*0.62f,sx+b,sy+b*0.62f+pw*0.85f,bc*0.8f, mTint);
                        mGo(); return;
                    }
                    if (mShape == SHAPE_PANE || mShape == SHAPE_FLAT_PANEL) {
                        float pw=b*0.18f, cx2=sx+b*0.5f;
                        glm::vec3 pc = getBlockProps(bType).isTransparent
                            ? glm::mix(bc,glm::vec3(0.6f,0.85f,1.0f),0.5f) : bc;
                        drawFlatIcon(iv,mTex,cx2-pw*0.5f,sy,cx2+pw*0.5f,sy+b,pc, mTint);
                        drawFlatIcon(iv,mTex,sx,sy+b*0.44f,sx+b,sy+b*0.56f,pc*0.8f, mTint);
                        mGo(); return;
                    }
                    // default: full flat square
                    drawFlatIcon(iv, mTex, sx, sy, sx+b, sy+b, bc, mTint);
                    mGo();
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

                // Draw output item directly (no slotSize swap needed with flat icons)
                drawSlotItem(outX, outY2, recipes[rIdx].resultType);
            }

            // "No results" indicator — red bar across middle
            if (isSearching && filtered.empty()) {
                drawRect(rbX + 10, rbY + rbH * 0.5f - 3.0f, rbW - 20, 6.0f, glm::vec3(0.7f, 0.1f, 0.1f));
            }
        }

        // ==== BUILD TAB (plan_2 Step 4) ====
        // A tab on the crafting screen rather than its own full-screen menu, as
        // plan_2 asks: this screen already owns the cursor, the dim overlay and
        // the hit-testing, and a second menu with its own cursor state is where
        // input bugs come from.
        float buildBtnX = craftOffX - 40.0f;
        float buildBtnY = craftY + slotStep - 30.0f;   // directly under the recipe-book button
        {
            std::vector<float> bv;
            glm::vec3 col = buildTabOpen ? glm::vec3(0.75f, 0.45f, 0.15f)
                                         : glm::vec3(0.35f, 0.28f, 0.20f);
            pushQuad(bv, buildBtnX, buildBtnY, buildBtnX + 24.0f, buildBtnY + 24.0f, col);
            // A tiny house glyph, so the two square buttons are distinguishable.
            glm::vec3 ink(0.95f, 0.90f, 0.80f);
            pushQuad(bv, buildBtnX + 6, buildBtnY + 5, buildBtnX + 18, buildBtnY + 14, ink);
            pushQuad(bv, buildBtnX + 4, buildBtnY + 14, buildBtnX + 20, buildBtnY + 17, ink);
            glBufferSubData(GL_ARRAY_BUFFER, 0, bv.size() * sizeof(float), bv.data());
            setInt(shaderProgram, "textureMode", 0);
            glDrawArrays(GL_TRIANGLES, 0, (int)bv.size() / 11);
        }

        if (buildTabOpen) {
            // Same slab of screen the recipe book uses — they are mutually
            // exclusive for exactly that reason (see input.h).
            float bpW = 180.0f;
            float bpH = panelH;
            float bpX = panelX - bpW - 10.0f;
            float bpY = panelY;

            // drawRect above is scoped inside the recipeBookOpen block, so this
            // panel needs its own.
            auto rect = [&](float x, float y, float w, float h, glm::vec3 c) {
                std::vector<float> rv;
                pushQuad(rv, x, y, x + w, y + h, c);
                glBufferSubData(GL_ARRAY_BUFFER, 0, rv.size() * sizeof(float), rv.data());
                setInt(shaderProgram, "textureMode", 0);
                glDrawArrays(GL_TRIANGLES, 0, (int)rv.size() / 11);
            };
            auto text = [&](const char* s, float x, float y, float h, glm::vec3 c) {
                std::vector<float> tv;
                hudDrawString(tv, s, x, y, h, c);
                if (tv.empty()) return;
                glBufferSubData(GL_ARRAY_BUFFER, 0, tv.size() * sizeof(float), tv.data());
                setInt(shaderProgram, "textureMode", 0);
                glDrawArrays(GL_TRIANGLES, 0, (int)tv.size() / 11);
            };

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            setFloat(shaderProgram, "alpha", 0.85f);
            rect(bpX, bpY, bpW, bpH, glm::vec3(0.15f, 0.13f, 0.12f));
            setFloat(shaderProgram, "alpha", 1.0f);
            glDisable(GL_BLEND);

            glm::vec3 borderCol(0.5f, 0.35f, 0.18f);
            rect(bpX, bpY, bpW, 2.0f, borderCol);
            rect(bpX, bpY + bpH - 2.0f, bpW, 2.0f, borderCol);
            rect(bpX, bpY, 2.0f, bpH, borderCol);
            rect(bpX + bpW - 2.0f, bpY, 2.0f, bpH, borderCol);

            text("BUILD", bpX + 10.0f, bpY + bpH - 22.0f, 12.0f, glm::vec3(1.0f, 0.8f, 0.4f));
            text("places 3m ahead", bpX + 10.0f, bpY + bpH - 36.0f, 8.0f, glm::vec3(0.55f, 0.5f, 0.45f));

            const float rowH = 58.0f;
            float rowTop = bpY + bpH - 48.0f;
            for (int i = 0; i < NUM_BUILD_RECIPES; i++) {
                const BuildRecipe& br = buildRecipes[i];
                int have1 = countInInventory(br.t1);
                int have2 = countInInventory(br.t2);
                bool afford = (have1 >= br.c1 && have2 >= br.c2);

                float ry = rowTop - (i + 1) * rowH;
                // Green when the player can afford it, red when they cannot —
                // the same read the crafting output slot gives.
                rect(bpX + 8.0f, ry, bpW - 16.0f, rowH - 6.0f,
                     afford ? glm::vec3(0.16f, 0.26f, 0.16f) : glm::vec3(0.26f, 0.15f, 0.14f));

                text(br.name, bpX + 14.0f, ry + rowH - 22.0f, 11.0f,
                     afford ? glm::vec3(0.85f, 1.0f, 0.85f) : glm::vec3(0.85f, 0.6f, 0.6f));

                // Ingredient lines: swatch, then "3/5 stone" so the shortfall is
                // visible without opening the recipe book.
                int   ct[2] = { br.c1, br.c2 };
                int   tt[2] = { br.t1, br.t2 };
                int   hv[2] = { have1, have2 };
                for (int k = 0; k < 2; k++) {
                    float ly = ry + rowH - 36.0f - k * 12.0f;
                    rect(bpX + 14.0f, ly, 8.0f, 8.0f, getBlockColor(tt[k]));
                    char line[64];
                    snprintf(line, sizeof(line), "%d/%d %s", hv[k], ct[k], getBlockName(tt[k]));
                    bool ok = (hv[k] >= ct[k]);
                    text(line, bpX + 26.0f, ly, 8.0f,
                         ok ? glm::vec3(0.8f, 0.9f, 0.8f) : glm::vec3(0.95f, 0.5f, 0.45f));
                }
            }
        }

        // Arrow indicator
        float arrowX = craftOffX + craftGridW + arrowGapL;
        float arrowCenterY = craftY + slotStep; // vertically centered for 3x3
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
            drawSlotCount(outX, outY, craftingOutput.count);
            drawSlotHighlight(outX, outY, craftingOutput.type);
        }

        // Draw item name tooltip on hover
        if (hoveredItemType != BLOCK_AIR && draggedSlot.type == BLOCK_AIR) {
            const char* name = getBlockName(hoveredItemType);
            float tipH = 11.0f;
            float tipW = hudMeasureString(name, tipH);
            float tipPad = 4.0f;
            float tipX = mx - tipW / 2.0f;
            float tipY = my + slotSize + 4.0f; // above the slot
            // Clamp to screen
            if (tipX < 2.0f) tipX = 2.0f;
            if (tipX + tipW + tipPad * 2 > screenW) tipX = screenW - tipW - tipPad * 2;
            // Background
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            setFloat(shaderProgram, "alpha", 0.85f);
            {
                std::vector<float> tbv;
                pushQuad(tbv, tipX - tipPad, tipY - tipPad, tipX + tipW + tipPad, tipY + tipH + tipPad,
                         glm::vec3(0.1f, 0.05f, 0.15f));
                glBufferSubData(GL_ARRAY_BUFFER, 0, tbv.size() * sizeof(float), tbv.data());
                setVec3(shaderProgram, "objectColor", glm::vec3(0.1f, 0.05f, 0.15f));
                glDrawArrays(GL_TRIANGLES, 0, (int)tbv.size() / 11);
            }
            // Border
            setFloat(shaderProgram, "alpha", 1.0f);
            {
                std::vector<float> brv;
                float b = 1.0f;
                glm::vec3 bc(0.3f, 0.1f, 0.5f);
                pushQuad(brv, tipX - tipPad, tipY - tipPad, tipX + tipW + tipPad, tipY - tipPad + b, bc);
                pushQuad(brv, tipX - tipPad, tipY + tipH + tipPad - b, tipX + tipW + tipPad, tipY + tipH + tipPad, bc);
                pushQuad(brv, tipX - tipPad, tipY - tipPad, tipX - tipPad + b, tipY + tipH + tipPad, bc);
                pushQuad(brv, tipX + tipW + tipPad - b, tipY - tipPad, tipX + tipW + tipPad, tipY + tipH + tipPad, bc);
                glBufferSubData(GL_ARRAY_BUFFER, 0, brv.size() * sizeof(float), brv.data());
                setVec3(shaderProgram, "objectColor", bc);
                glDrawArrays(GL_TRIANGLES, 0, (int)brv.size() / 11);
            }
            // Text
            {
                std::vector<float> tv;
                hudDrawString(tv, name, tipX, tipY, tipH, glm::vec3(1.0f));
                glBufferSubData(GL_ARRAY_BUFFER, 0, tv.size() * sizeof(float), tv.data());
                setVec3(shaderProgram, "objectColor", glm::vec3(1.0f));
                glDrawArrays(GL_TRIANGLES, 0, (int)tv.size() / 11);
            }
            glDisable(GL_BLEND);
        }

        // Draw Dragged item at mouse cursor
        if (draggedSlot.type != BLOCK_AIR) {
            float mx = lastMouseX;
            float my = screenH - lastMouseY;
            drawSlotItem(mx - slotSize / 2, my - slotSize / 2, draggedSlot.type);
            drawSlotCount(mx - slotSize / 2, my - slotSize / 2, draggedSlot.count);
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
            drawSlotCount(sx, hbY, playerInventory[i].count);
            // Durability bar (3px tall at bottom of slot)
            int dur = playerInventory[i].durability;
            int maxDur = getMaxDurability(playerInventory[i].type);
            if (dur > 0 && maxDur > 0) {
                float frac = (float)dur / (float)maxDur;
                glm::vec3 durCol = (frac > 0.5f) ? glm::vec3(0.1f, 0.85f, 0.1f)
                                 : (frac > 0.2f) ? glm::vec3(0.9f, 0.8f, 0.1f)
                                                 : glm::vec3(0.9f, 0.15f, 0.1f);
                std::vector<float> dv;
                float barW2 = slotSize - 4.0f;
                pushQuad(dv, sx + 2, hbY + 2, sx + 2 + barW2, hbY + 5, glm::vec3(0.1f));
                glBufferSubData(GL_ARRAY_BUFFER, 0, dv.size() * sizeof(float), dv.data());
                setVec3(shaderProgram, "objectColor", glm::vec3(0.1f));
                glDrawArrays(GL_TRIANGLES, 0, (int)dv.size() / 11);
                dv.clear();
                pushQuad(dv, sx + 2, hbY + 2, sx + 2 + barW2 * frac, hbY + 5, durCol);
                glBufferSubData(GL_ARRAY_BUFFER, 0, dv.size() * sizeof(float), dv.data());
                setVec3(shaderProgram, "objectColor", durCol);
                glDrawArrays(GL_TRIANGLES, 0, (int)dv.size() / 11);
            }
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


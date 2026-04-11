# A Python script to completely upgrade renderHUD for full 36-slot inventories and 3x3 crafting grids

import re

with open('hud.h', 'r') as f:
    text = f.read()

# We will locate `// Hotbar slots` and rewrite it.
old_hotbar = re.search(r'// Hotbar slots.*?// Helper to draw a single bar', text, re.DOTALL)

new_hud_logic = """// --- Inventory + Hotbar UI ---
    float slotSize = 40.0f;
    float slotGap = 4.0f;
    float totalW = HOTBAR_SIZE * slotSize + (HOTBAR_SIZE - 1) * slotGap;
    float hbX = (screenW - totalW) / 2.0f;
    float hbY = 10.0f; // Hotbar at bottom

    // Draw inventory screen if open
    if (inventoryOpen) {
        // Darken screen
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        setFloat(shaderProgram, "alpha", 0.6f);
        std::vector<float> dv;
        pushQuad(dv, 0, 0, screenW, screenH, glm::vec3(0.0f));
        glBufferSubData(GL_ARRAY_BUFFER, 0, dv.size() * sizeof(float), dv.data());
        setVec3(shaderProgram, "emissiveColor", glm::vec3(0.0f));
        glDrawArrays(GL_TRIANGLES, 0, (int)dv.size() / 11);
        setFloat(shaderProgram, "alpha", 1.0f);
        glDisable(GL_BLEND);

        // Draw the 3x9 main storage grid above the hotbar
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 9; c++) {
                int i = 9 + r * 9 + c; // slots 9 to 35
                float sx = hbX + c * (slotSize + slotGap);
                float sy = hbY + (r + 2) * (slotSize + slotGap); // leave gap above hotbar

                // Draw background
                std::vector<float> sv;
                pushQuad(sv, sx, sy, sx + slotSize, sy + slotSize, glm::vec3(0.3f));
                glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
                setVec3(shaderProgram, "emissiveColor", glm::vec3(0.3f));
                glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 11);

                // Draw Block
                int bType = playerInventory[i].type;
                if (bType != BLOCK_AIR) {
                    std::vector<float> iv;
                    float pad = 6.0f;
                    glm::vec3 bc = getBlockColor(bType);

                    bool useTex = false; GLuint texToBind = 0;
                    if (bType == BLOCK_GRASS || bType == BLOCK_LEAF || bType == BLOCK_SNOW) { useTex = true; texToBind = texGrass; }
                    else if (bType == BLOCK_WOOD) { useTex = true; texToBind = texWood; }
                    else if (bType == BLOCK_DIRT || bType == BLOCK_STONE || bType == BLOCK_STONE_LIGHT || bType == BLOCK_COAL_ORE) { 
                        useTex = true; texToBind = texBrick; 
                    }

                    if (useTex) {
                        pushQuadUV(iv, sx + pad, sy + pad, sx + slotSize - pad, sy + slotSize - pad, bc, 0.0f, 0.0f, 1.0f, 1.0f);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size() * sizeof(float), iv.data());
                        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texToBind);
                        setInt(shaderProgram, "textureMode", 1);
                        setVec3(shaderProgram, "emissiveColor", bc);
                        glDrawArrays(GL_TRIANGLES, 0, (int)iv.size() / 11);
                        setInt(shaderProgram, "textureMode", 0);
                    } else {
                        pushQuad(iv, sx + pad, sy + pad, sx + slotSize - pad, sy + slotSize - pad, bc);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size() * sizeof(float), iv.data());
                        setVec3(shaderProgram, "emissiveColor", bc);
                        glDrawArrays(GL_TRIANGLES, 0, (int)iv.size() / 11);
                    }
                }
            }
        }
    }

    // Hotbar slots (drawn always)
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        float sx = hbX + i * (slotSize + slotGap);
        glm::vec3 bgCol = (!inventoryOpen && i == hotbarSlot) ? glm::vec3(0.9f, 0.9f, 0.3f) : glm::vec3(0.3f, 0.3f, 0.3f);

        std::vector<float> sv;
        pushQuad(sv, sx, hbY, sx + slotSize, hbY + slotSize, bgCol);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sv.size() * sizeof(float), sv.data());
        setVec3(shaderProgram, "emissiveColor", bgCol);
        glDrawArrays(GL_TRIANGLES, 0, (int)sv.size() / 11);

        int bType = playerInventory[i].type;
        if (bType != BLOCK_AIR) {
            std::vector<float> iv;
            float pad = 6.0f;
            glm::vec3 bc = getBlockColor(bType);

            bool useTex = false; GLuint texToBind = 0;
            if (bType == BLOCK_GRASS || bType == BLOCK_LEAF || bType == BLOCK_SNOW) { useTex = true; texToBind = texGrass; }
            else if (bType == BLOCK_WOOD) { useTex = true; texToBind = texWood; }
            else if (bType == BLOCK_DIRT || bType == BLOCK_STONE || bType == BLOCK_STONE_LIGHT || bType == BLOCK_COAL_ORE) { 
                useTex = true; texToBind = texBrick; 
            }

            if (useTex) {
                pushQuadUV(iv, sx + pad, hbY + pad, sx + slotSize - pad, hbY + slotSize - pad, bc, 0.0f, 0.0f, 1.0f, 1.0f);
                glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size() * sizeof(float), iv.data());
                glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texToBind);
                setInt(shaderProgram, "textureMode", 1);
                setVec3(shaderProgram, "emissiveColor", bc);
                glDrawArrays(GL_TRIANGLES, 0, (int)iv.size() / 11);
                setInt(shaderProgram, "textureMode", 0);
            } else {
                pushQuad(iv, sx + pad, hbY + pad, sx + slotSize - pad, hbY + slotSize - pad, bc);
                glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size() * sizeof(float), iv.data());
                setVec3(shaderProgram, "emissiveColor", bc);
                glDrawArrays(GL_TRIANGLES, 0, (int)iv.size() / 11);
            }
        }
    }

    // Helper to draw a single bar"""

if old_hotbar:
    text = text.replace(old_hotbar.group(0), new_hud_logic)
else:
    print("Could not find hotbar logic in hud.h!")

with open('hud.h', 'w') as f:
    f.write(text)

print("Injected full 36-slot GUI overlay!")

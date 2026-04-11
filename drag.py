import re

with open('hud.h', 'r') as f:
    text = f.read()

drag_render = """    // Draw Dragged item
    if (inventoryOpen && draggedSlot.type != BLOCK_AIR) {
        float mx = lastMouseX;
        float my = screenH - lastMouseY;
        std::vector<float> iv;
        float pad = 6.0f;
        glm::vec3 bc = getBlockColor(draggedSlot.type);
        int bType = draggedSlot.type;
        
        bool useTex = false; GLuint texToBind = 0;
        if (bType == BLOCK_GRASS || bType == BLOCK_LEAF || bType == BLOCK_SNOW) { useTex = true; texToBind = texGrass; }
        else if (bType == BLOCK_WOOD) { useTex = true; texToBind = texWood; }
        else if (bType == BLOCK_DIRT || bType == BLOCK_STONE || bType == BLOCK_STONE_LIGHT || bType == BLOCK_COAL_ORE) { 
            useTex = true; texToBind = texBrick; 
        }

        if (useTex) {
            pushQuadUV(iv, mx - slotSize/2 + pad, my - slotSize/2 + pad, mx + slotSize/2 - pad, my + slotSize/2 - pad, bc, 0.0f, 0.0f, 1.0f, 1.0f);
            glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size() * sizeof(float), iv.data());
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texToBind);
            setInt(shaderProgram, "textureMode", 1);
            setVec3(shaderProgram, "emissiveColor", bc);
            glDrawArrays(GL_TRIANGLES, 0, (int)iv.size() / 11);
            setInt(shaderProgram, "textureMode", 0);
        } else {
            pushQuad(iv, mx - slotSize/2 + pad, my - slotSize/2 + pad, mx + slotSize/2 - pad, my + slotSize/2 - pad, bc);
            glBufferSubData(GL_ARRAY_BUFFER, 0, iv.size() * sizeof(float), iv.data());
            setVec3(shaderProgram, "emissiveColor", bc);
            glDrawArrays(GL_TRIANGLES, 0, (int)iv.size() / 11);
        }
    }

    // Helper to draw a single bar"""

text = text.replace('    // Helper to draw a single bar', drag_render)

with open('hud.h', 'w') as f:
    f.write(text)

print("Injected dragged block rendering!")

#pragma once
// =====================================================
// Player Character (hierarchical hexagonal model)
// Minecraft-proportioned: 1.8 blocks tall
// Head: 0.5x0.5x0.5, Torso: 0.5x0.75x0.25, Arms/Legs: 0.25x0.75x0.25
// =====================================================
void drawPlayer(glm::vec3 pos, float time, float yaw = 0.0f, bool walking = false, float walkT = 0.0f) {
    float walkCycle = walking ? sinf(walkT * 3.0f) : 0.0f;

    auto makePartModel = [&](glm::vec3 offset, glm::vec3 scale) -> glm::mat4 {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, offset);
        m = glm::scale(m, scale);
        return m;
    };

    // Minecraft Steve colors
    glm::vec3 shirtColor(0.25f, 0.55f, 0.55f); // teal shirt
    glm::vec3 skinColor(0.73f, 0.56f, 0.44f);   // skin
    glm::vec3 hairColor(0.28f, 0.18f, 0.10f);    // brown hair
    glm::vec3 pantsColor(0.22f, 0.22f, 0.55f);   // dark blue jeans
    glm::vec3 shoeColor(0.3f, 0.3f, 0.3f);       // dark shoes

    // Layout (y from feet at 0):
    //   Legs:  0.0 to 0.65  (center 0.325, scale.y=0.65)
    //   Torso: 0.65 to 1.30 (center 0.975, scale.y=0.65)
    //   Head:  1.30 to 1.80 (center 1.55,  scale.y=0.50)

    // Body (torso) — teal shirt
    drawHexModel(makePartModel(glm::vec3(0, 0.975f, 0), glm::vec3(0.45f, 0.65f, 0.30f)), shirtColor);

    // Head — skin colored cube
    drawHexModel(makePartModel(glm::vec3(0, 1.55f, 0), glm::vec3(0.38f, 0.50f, 0.38f)), skinColor);

    // Hair on top of head
    drawHexModel(makePartModel(glm::vec3(0, 1.82f, 0), glm::vec3(0.40f, 0.08f, 0.40f)), hairColor);

    // Left arm (swings with walk)
    float armSwing = walkCycle * 0.4f;
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(-0.38f, 0.975f, 0)); // shoulder pivot
        m = myRotate(m, armSwing, glm::vec3(1, 0, 0));
        m = glm::translate(m, glm::vec3(0, -0.325f, 0)); // offset down from pivot
        m = glm::scale(m, glm::vec3(0.18f, 0.65f, 0.18f));
        drawHexModel(m, skinColor);
    }
    // Right arm
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(0.38f, 0.975f, 0));
        m = myRotate(m, -armSwing, glm::vec3(1, 0, 0));
        m = glm::translate(m, glm::vec3(0, -0.325f, 0));
        m = glm::scale(m, glm::vec3(0.18f, 0.65f, 0.18f));
        drawHexModel(m, skinColor);
    }

    // Left leg
    float legSwing = walkCycle * 0.35f;
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(-0.15f, 0.65f, 0)); // hip pivot
        m = myRotate(m, legSwing, glm::vec3(1, 0, 0));
        m = glm::translate(m, glm::vec3(0, -0.325f, 0)); // offset down from pivot
        m = glm::scale(m, glm::vec3(0.18f, 0.65f, 0.18f));
        drawHexModel(m, pantsColor);
    }
    // Right leg
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(0.15f, 0.65f, 0));
        m = myRotate(m, -legSwing, glm::vec3(1, 0, 0));
        m = glm::translate(m, glm::vec3(0, -0.325f, 0));
        m = glm::scale(m, glm::vec3(0.18f, 0.65f, 0.18f));
        drawHexModel(m, pantsColor);
    }
}


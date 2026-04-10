#pragma once
// =====================================================
// Player Character (hierarchical hexagonal model)
// Body parts have individual transforms relative to parent
// =====================================================
void drawPlayer(glm::vec3 pos, float time, float yaw = 0.0f, bool walking = false, float walkT = 0.0f) {
    float walkCycle = walking ? sinf(walkT * 3.0f) : 0.0f; // no movement when idle

    // Base transform: translate to pos, then rotate to face yaw
    auto makePartModel = [&](glm::vec3 offset, glm::vec3 scale) -> glm::mat4 {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, offset);
        m = glm::scale(m, scale);
        return m;
    };

    glm::vec3 bodyColor(0.55f, 0.45f, 0.3f);
    glm::vec3 skinColor(0.8f, 0.65f, 0.5f);

    // Body (torso)
    drawHexModel(makePartModel(glm::vec3(0, 1.0f, 0), glm::vec3(0.7f, 0.9f, 0.5f)), bodyColor);

    // Head
    drawHexModel(makePartModel(glm::vec3(0, 1.85f, 0), glm::vec3(0.5f, 0.5f, 0.5f)), skinColor);

    // Helmet
    drawHexModel(makePartModel(glm::vec3(0, 2.1f, 0), glm::vec3(0.55f, 0.2f, 0.55f)),
                 glm::vec3(0.2f, 0.5f, 0.7f));

    // Left arm (swings with walk)
    float armSwing = walkCycle * 0.35f;
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(-0.55f, 1.1f, 0));
        m = myRotate(m, armSwing, glm::vec3(1, 0, 0));
        m = glm::scale(m, glm::vec3(0.25f, 0.7f, 0.25f));
        drawHexModel(m, skinColor);
    }
    // Right arm
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(0.55f, 1.1f, 0));
        m = myRotate(m, -armSwing, glm::vec3(1, 0, 0));
        m = glm::scale(m, glm::vec3(0.25f, 0.7f, 0.25f));
        drawHexModel(m, skinColor);
    }

    // Left leg
    float legSwing = walkCycle * 0.3f;
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(-0.2f, 0.25f, 0));
        m = myRotate(m, legSwing, glm::vec3(1, 0, 0));
        m = glm::scale(m, glm::vec3(0.25f, 0.55f, 0.25f));
        drawHexModel(m, glm::vec3(0.35f, 0.25f, 0.15f));
    }
    // Right leg
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, yaw - PI/2.0f, glm::vec3(0, 1, 0));
        m = glm::translate(m, glm::vec3(0.2f, 0.25f, 0));
        m = myRotate(m, -legSwing, glm::vec3(1, 0, 0));
        m = glm::scale(m, glm::vec3(0.25f, 0.55f, 0.25f));
        drawHexModel(m, glm::vec3(0.35f, 0.25f, 0.15f));
    }
}


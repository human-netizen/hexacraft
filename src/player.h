#pragma once
// =====================================================
// Player Character (hierarchical hexagonal model)
// Minecraft Steve proportions — blocky and chunky
// =====================================================

const float PS = 0.55f; // player scale factor

void drawPlayer(glm::vec3 pos, float time, float yaw = 0.0f, bool walking = false, float walkT = 0.0f) {
    float walkCycle = walking ? sinf(walkT * 3.0f) : 0.0f;

    // Base transform: translate to pos, rotate to face yaw
    auto baseM = [&]() -> glm::mat4 {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        m = myRotate(m, PI/2.0f - yaw, glm::vec3(0, 1, 0));
        return m;
    };

    auto makePartModel = [&](glm::vec3 offset, glm::vec3 scale) -> glm::mat4 {
        glm::mat4 m = baseM();
        m = glm::translate(m, offset * PS);
        m = glm::scale(m, scale * PS);
        return m;
    };


    // Cute chibi colors
    glm::vec3 shirtColor(0.25f, 0.55f, 0.55f);   // teal shirt
    glm::vec3 skinColor(0.73f, 0.56f, 0.44f);     // skin
    glm::vec3 hairColor(0.28f, 0.18f, 0.10f);     // brown hair
    glm::vec3 pantsColor(0.22f, 0.22f, 0.55f);    // dark blue jeans
    glm::vec3 shoeColor(0.25f, 0.25f, 0.25f);     // dark grey shoes
    glm::vec3 eyeWhite(0.9f, 0.9f, 0.9f);         // white of eyes
    glm::vec3 eyePupil(0.15f, 0.10f, 0.50f);      // dark blue pupil
    glm::vec3 mouthColor(0.45f, 0.30f, 0.22f);    // darker skin for mouth

    // Chibi layout (y from feet at 0):
    //   Shoes: 0.0  to 0.06
    //   Legs:  0.06 to 0.42
    //   Torso: 0.42 to 0.85
    //   Head:  0.85 to 1.30  (big head = cute)

    // ---- TORSO (wide barrel) ----
    drawHexModel(makePartModel(glm::vec3(0, 0.635f, 0), glm::vec3(0.48f, 0.43f, 0.36f)), shirtColor);

    // ---- HEAD (big and round) ----
    drawHexModel(makePartModel(glm::vec3(0, 1.075f, 0), glm::vec3(0.44f, 0.45f, 0.44f)), skinColor);

    // ---- HAIR (brown, wraps around head) ----
    // Top
    drawHexModel(makePartModel(glm::vec3(0, 1.30f, 0), glm::vec3(0.46f, 0.06f, 0.46f)), hairColor);
    // Back of head
    drawHexModel(makePartModel(glm::vec3(0, 1.08f, -0.23f), glm::vec3(0.46f, 0.40f, 0.04f)), hairColor);
    // Left side hair
    drawHexModel(makePartModel(glm::vec3(-0.23f, 1.10f, 0), glm::vec3(0.04f, 0.30f, 0.46f)), hairColor);
    // Right side hair
    drawHexModel(makePartModel(glm::vec3(0.23f, 1.10f, 0), glm::vec3(0.04f, 0.30f, 0.46f)), hairColor);
    // Fringe/bangs
    drawHexModel(makePartModel(glm::vec3(0, 1.26f, 0.20f), glm::vec3(0.46f, 0.06f, 0.04f)), hairColor);

    // ---- FACE (big cute eyes) ----
    // Left eye white (bigger)
    drawHexModel(makePartModel(glm::vec3(-0.11f, 1.09f, 0.23f), glm::vec3(0.10f, 0.11f, 0.02f)), eyeWhite);
    // Right eye white (bigger)
    drawHexModel(makePartModel(glm::vec3(0.11f, 1.09f, 0.23f), glm::vec3(0.10f, 0.11f, 0.02f)), eyeWhite);
    // Left pupil (large)
    drawHexModel(makePartModel(glm::vec3(-0.09f, 1.08f, 0.24f), glm::vec3(0.07f, 0.09f, 0.02f)), eyePupil);
    // Right pupil (large)
    drawHexModel(makePartModel(glm::vec3(0.09f, 1.08f, 0.24f), glm::vec3(0.07f, 0.09f, 0.02f)), eyePupil);
    // Nose (tiny dot)
    drawHexModel(makePartModel(glm::vec3(0, 1.00f, 0.24f), glm::vec3(0.03f, 0.03f, 0.02f)), glm::vec3(0.68f, 0.50f, 0.38f));
    // Mouth (cute smile — two small blocks)
    drawHexModel(makePartModel(glm::vec3(-0.06f, 0.93f, 0.23f), glm::vec3(0.04f, 0.02f, 0.02f)), mouthColor);
    drawHexModel(makePartModel(glm::vec3(0.06f, 0.93f, 0.23f), glm::vec3(0.04f, 0.02f, 0.02f)), mouthColor);
    drawHexModel(makePartModel(glm::vec3(0, 0.91f, 0.23f), glm::vec3(0.08f, 0.02f, 0.02f)), mouthColor);

    // ---- LEFT ARM (short stubby, swings with walk) ----
    float armSwing = walkCycle * 0.4f;
    {
        glm::mat4 m = baseM();
        m = glm::translate(m, glm::vec3(-0.44f * PS, 0.85f * PS, 0));  // shoulder pivot
        m = myRotate(m, armSwing, glm::vec3(1, 0, 0));

        // Upper arm (shirt color) — short and fat
        glm::mat4 upper = m;
        upper = glm::translate(upper, glm::vec3(0, -0.14f * PS, 0));
        upper = glm::scale(upper, glm::vec3(0.22f, 0.28f, 0.22f) * PS);
        drawHexModel(upper, shirtColor);

        // Lower arm (skin) — stubby
        glm::mat4 lower = m;
        lower = glm::translate(lower, glm::vec3(0, -0.36f * PS, 0));
        lower = glm::scale(lower, glm::vec3(0.20f, 0.22f, 0.20f) * PS);
        drawHexModel(lower, skinColor);
    }

    // ---- RIGHT ARM + HELD ITEM ----
    {
        glm::mat4 m = baseM();
        m = glm::translate(m, glm::vec3(0.44f * PS, 0.85f * PS, 0)); // shoulder pivot
        m = myRotate(m, -armSwing, glm::vec3(1, 0, 0));
        glm::mat4 shoulderPivot = m; // save for held item

        // Upper arm (shirt color) — short and fat
        glm::mat4 upper = m;
        upper = glm::translate(upper, glm::vec3(0, -0.14f * PS, 0));
        upper = glm::scale(upper, glm::vec3(0.22f, 0.28f, 0.22f) * PS);
        drawHexModel(upper, shirtColor);

        // Lower arm (skin) — stubby
        glm::mat4 lower = m;
        lower = glm::translate(lower, glm::vec3(0, -0.36f * PS, 0));
        lower = glm::scale(lower, glm::vec3(0.20f, 0.22f, 0.20f) * PS);
        drawHexModel(lower, skinColor);

        // Draw held item in right hand
        int heldType = playerInventory[hotbarSlot].type;
        if (heldType != BLOCK_AIR) {
            glm::vec3 itemColor = getBlockColor(heldType);
            BlockProperties heldProps = getBlockProps(heldType);

            // Hand position: bottom of right arm (stubby chibi arm)
            glm::mat4 im = shoulderPivot;
            im = glm::translate(im, glm::vec3(0, -0.52f * PS, 0)); // bottom of arm

            if (isSword(heldType)) {
                im = myRotate(im, -0.85f, glm::vec3(1, 0, 0));
                im = glm::translate(im, glm::vec3(0, -0.25f * PS, 0));
                im = glm::scale(im, glm::vec3(0.02f, 0.50f, 0.08f) * PS);
                drawHexModel(im, itemColor);
            } else if (isAxe(heldType)) {
                im = myRotate(im, -0.85f, glm::vec3(1, 0, 0));
                glm::mat4 hm = im;
                hm = glm::translate(hm, glm::vec3(0, -0.22f * PS, 0));
                hm = glm::scale(hm, glm::vec3(0.025f, 0.50f, 0.025f) * PS);
                drawHexModel(hm, glm::vec3(0.6f, 0.4f, 0.15f));
                glm::mat4 ah = im;
                ah = glm::translate(ah, glm::vec3(0, 0.02f * PS, 0.04f * PS));
                ah = glm::scale(ah, glm::vec3(0.025f, 0.15f, 0.10f) * PS);
                drawHexModel(ah, itemColor);
            } else if (isPickaxe(heldType)) {
                im = myRotate(im, -0.85f, glm::vec3(1, 0, 0));
                glm::mat4 hm = im;
                hm = glm::translate(hm, glm::vec3(0, -0.22f * PS, 0));
                hm = glm::scale(hm, glm::vec3(0.025f, 0.50f, 0.025f) * PS);
                drawHexModel(hm, glm::vec3(0.6f, 0.4f, 0.15f));
                glm::mat4 ph = im;
                ph = glm::translate(ph, glm::vec3(0, 0.02f * PS, 0));
                ph = glm::scale(ph, glm::vec3(0.025f, 0.04f, 0.18f) * PS);
                drawHexModel(ph, itemColor);
            } else if (isShovel(heldType)) {
                im = myRotate(im, -0.85f, glm::vec3(1, 0, 0));
                glm::mat4 hm = im;
                hm = glm::translate(hm, glm::vec3(0, -0.20f * PS, 0));
                hm = glm::scale(hm, glm::vec3(0.025f, 0.45f, 0.025f) * PS);
                drawHexModel(hm, glm::vec3(0.6f, 0.4f, 0.15f));
                glm::mat4 sh = im;
                sh = glm::translate(sh, glm::vec3(0, -0.45f * PS, 0));
                sh = glm::scale(sh, glm::vec3(0.025f, 0.10f, 0.08f) * PS);
                drawHexModel(sh, itemColor);
            } else if (heldType == ITEM_STICK) {
                im = myRotate(im, -0.85f, glm::vec3(1, 0, 0));
                im = glm::translate(im, glm::vec3(0, -0.22f * PS, 0));
                im = glm::scale(im, glm::vec3(0.025f, 0.45f, 0.025f) * PS);
                drawHexModel(im, itemColor);
            } else if (heldType == ITEM_BOW) {
                im = myRotate(im, -0.85f, glm::vec3(1, 0, 0));
                im = glm::translate(im, glm::vec3(0, -0.20f * PS, 0));
                im = glm::scale(im, glm::vec3(0.02f, 0.45f, 0.06f) * PS);
                drawHexModel(im, itemColor);
            } else if (!heldProps.isItem) {
                im = glm::translate(im, glm::vec3(0, -0.08f * PS, -0.06f * PS));
                im = glm::scale(im, glm::vec3(0.15f, 0.15f, 0.15f) * PS);
                drawHexModel(im, itemColor);
            }
        }
    }

    // ---- LEFT LEG (short stubby) ----
    float legSwing = walkCycle * 0.35f;
    {
        glm::mat4 m = baseM();
        m = glm::translate(m, glm::vec3(-0.14f * PS, 0.42f * PS, 0)); // hip pivot
        m = myRotate(m, legSwing, glm::vec3(1, 0, 0));

        // Upper leg (jeans) — short and wide
        glm::mat4 upper = m;
        upper = glm::translate(upper, glm::vec3(0, -0.14f * PS, 0));
        upper = glm::scale(upper, glm::vec3(0.22f, 0.28f, 0.22f) * PS);
        drawHexModel(upper, pantsColor);

        // Shoe — chunky
        glm::mat4 shoe = m;
        shoe = glm::translate(shoe, glm::vec3(0, -0.38f * PS, 0.03f * PS));
        shoe = glm::scale(shoe, glm::vec3(0.22f, 0.10f, 0.26f) * PS);
        drawHexModel(shoe, shoeColor);
    }

    // ---- RIGHT LEG (short stubby) ----
    {
        glm::mat4 m = baseM();
        m = glm::translate(m, glm::vec3(0.14f * PS, 0.42f * PS, 0)); // hip pivot
        m = myRotate(m, -legSwing, glm::vec3(1, 0, 0));

        // Upper leg (jeans) — short and wide
        glm::mat4 upper = m;
        upper = glm::translate(upper, glm::vec3(0, -0.14f * PS, 0));
        upper = glm::scale(upper, glm::vec3(0.22f, 0.28f, 0.22f) * PS);
        drawHexModel(upper, pantsColor);

        // Shoe — chunky
        glm::mat4 shoe = m;
        shoe = glm::translate(shoe, glm::vec3(0, -0.38f * PS, 0.03f * PS));
        shoe = glm::scale(shoe, glm::vec3(0.22f, 0.10f, 0.26f) * PS);
        drawHexModel(shoe, shoeColor);
    }
}

#pragma once
// =====================================================
// Player Character (hierarchical hexagonal model)
// Minecraft Steve proportions — blocky and chunky
// =====================================================

const float PS = 0.55f; // player scale factor

void drawPlayer(glm::vec3 pos, float time, float yaw = 0.0f, bool walking = false, float walkT = 0.0f) {
    // playerWalkTime ticks at 6.0/s, so this converts it into the shared
    // GAIT_RATE cadence every walking figure in the game uses. `walking` is
    // false the instant the keys are released, which zeroes the gait and lets
    // the model settle rather than freezing mid-stride.
    Gait g = walking ? makeGait(walkT * (GAIT_RATE / 6.0f), PS)
                     : makeGait(0.0f, PS);

    // Base transform: translate to pos, rotate to face yaw. The bob is folded
    // in here so every part inherits it — head, limbs and held item alike.
    auto baseM = [&]() -> glm::mat4 {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos + glm::vec3(0.0f, g.bob, 0.0f));
        m = myRotate(m, PI/2.0f - yaw, glm::vec3(0, 1, 0));
        // Roll about the forward axis. This model is built +Z forward (the face
        // and eyes sit at +Z), so forward is Z here — the mobs are +X forward
        // and roll about X instead.
        m = myRotate(m, g.sway, glm::vec3(0, 0, 1));
        return m;
    };

    // The player's limbs swing about X, and for a +Z-forward model a positive
    // rotation about +X carries a hanging limb BACKWARD. drawLimb's contract is
    // that positive swing means forward, so the axis is passed negated — see
    // the note on frames in geometry.h.
    const glm::vec3 SWING_AXIS(-1, 0, 0);

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

    // ---- ARMS (short stubby, swings with walk) ----
    // The elbow is a real joint now: the forearm chains off the upper arm's
    // matrix rather than off the shoulder, so it pivots at the elbow. A small
    // negative bend folds it forward, the way a human forearm hangs.
    //
    // The shoulder moved in from 0.44 to 0.33. The torso is 0.48 wide, so its
    // edge is at 0.24, and an arm 0.22 thick hung at 0.44 spans 0.33..0.55 —
    // it never touched the body. That gap predates this change (the old code
    // used the same 0.44 and the same thickness), but it is very visible from
    // behind in third person, which is the view this model is mostly seen in.
    // At 0.33 the arm's inner face lands at 0.22, just inside the torso.
    const float SHOULDER_X = 0.33f;
    drawLimb(baseM(), glm::vec3(-SHOULDER_X, 0.85f, 0) * PS, SWING_AXIS,
             g.armL, -0.18f,
             0.28f * PS, 0.24f * PS, 0.22f * PS, shirtColor, skinColor, true);

    // ---- RIGHT ARM + HELD ITEM ----
    {
        // The wrist frame comes back out so the held item hangs off the hand
        // and inherits the elbow. Previously the item was chained to the
        // shoulder pivot, so a swinging arm slid out from under whatever it was
        // supposed to be holding.
        glm::mat4 wrist = drawLimb(baseM(), glm::vec3(SHOULDER_X, 0.85f, 0) * PS, SWING_AXIS,
                                   g.armR, -0.18f,
                                   0.28f * PS, 0.24f * PS, 0.22f * PS,
                                   shirtColor, skinColor, true);

        // Draw held item in right hand
        int heldType = playerInventory[hotbarSlot].type;
        if (heldType != BLOCK_AIR) {
            glm::vec3 itemColor = getBlockColor(heldType);
            BlockProperties heldProps = getBlockProps(heldType);

            // Hand position: the wrist frame drawLimb handed back, which is
            // already 0.52 * PS below the shoulder — the same place the old
            // fixed translate put it, but now carrying the elbow's rotation too.
            glm::mat4 im = wrist;

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

    // ---- LEGS (short stubby, with knees) ----
    // Note these take g.legL / g.legR, which run half a cycle out of phase with
    // g.armL / g.armR above. The old code drove arms and legs from the same
    // walkCycle, so the left arm and left leg went forward together — that is a
    // march, not a walk. The knee bends only on the forward half of the stride.
    // Thigh 0.24 + shin 0.18 = 0.42, which is where the hip sits, so the shoes
    // still meet the ground when the knee is straight.
    for (int side = 0; side < 2; side++) {
        float hipX  = (side == 0) ? -0.14f : 0.14f;
        float swing = (side == 0) ? g.legL : g.legR;
        float knee  = (side == 0) ? g.kneeL : g.kneeR;

        glm::mat4 ankle = drawLimb(baseM(), glm::vec3(hipX, 0.42f, 0) * PS, SWING_AXIS,
                                   swing, knee,
                                   0.24f * PS, 0.18f * PS, 0.22f * PS,
                                   pantsColor, pantsColor, true);
        // Shoe — chunky. Hung off the ankle, so it tilts with the shin instead
        // of staying flat while the leg above it swings.
        glm::mat4 shoe = glm::translate(ankle, glm::vec3(0, -0.02f, 0.03f) * PS);
        shoe = glm::scale(shoe, glm::vec3(0.22f, 0.10f, 0.26f) * PS);
        drawHexModel(shoe, shoeColor);
    }
}

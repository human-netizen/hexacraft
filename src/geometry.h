#pragma once

// =====================================================
// Hexagonal Prism Mesh Generation
// 8 faces: 6 rectangular sides + 2 hexagonal caps
// Each vertex: pos(3) + normal(3) + color(3) + texCoord(2) = 11 floats
// =====================================================
struct Vertex {
    glm::vec3 pos, normal, color;
    glm::vec2 texCoord;
};

// Helper to setup VAO attribs for Vertex struct (used by all mesh init functions)
void setupVertexAttribs() {
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(glm::vec3)));
    glEnableVertexAttribArray(3);
}

std::vector<Vertex> generateHexPrism() {
    std::vector<Vertex> verts;
    float r = HEX_RADIUS;
    float h = HEX_HEIGHT;
    float halfH = h / 2.0f;

    // Hex corner positions (XZ plane)
    glm::vec3 top[6], bot[6];
    for (int i = 0; i < 6; i++) {
        float angle = PI / 3.0f * i + PI / 6.0f; // offset 30deg for flat-top hex
        float x = r * cosf(angle);
        float z = r * sinf(angle);
        top[i] = glm::vec3(x, halfH, z);
        bot[i] = glm::vec3(x, -halfH, z);
    }

    glm::vec3 white(1.0f); // default color, overridden per-draw
    glm::vec2 uv0(0.0f, 0.0f);

    // --- Top cap (6 triangles, fan from center) ---
    glm::vec3 topCenter(0, halfH, 0);
    glm::vec3 topNorm(0, 1, 0);
    for (int i = 0; i < 6; i++) {
        int next = (i + 1) % 6;
        verts.push_back({topCenter, topNorm, white, glm::vec2(0.5f, 0.5f)});
        verts.push_back({top[i],    topNorm, white, glm::vec2(0.5f + 0.5f * cosf(PI/3.0f*i), 0.5f + 0.5f * sinf(PI/3.0f*i))});
        verts.push_back({top[next], topNorm, white, glm::vec2(0.5f + 0.5f * cosf(PI/3.0f*(i+1)), 0.5f + 0.5f * sinf(PI/3.0f*(i+1)))});
    }

    // --- Bottom cap (6 triangles, fan from center) ---
    glm::vec3 botCenter(0, -halfH, 0);
    glm::vec3 botNorm(0, -1, 0);
    for (int i = 0; i < 6; i++) {
        int next = (i + 1) % 6;
        verts.push_back({botCenter, botNorm, white, glm::vec2(0.5f, 0.5f)});
        verts.push_back({bot[next], botNorm, white, uv0});
        verts.push_back({bot[i],    botNorm, white, uv0});
    }

    // --- 6 rectangular side faces (2 triangles each) ---
    for (int i = 0; i < 6; i++) {
        int next = (i + 1) % 6;
        glm::vec3 edge = top[next] - top[i];
        glm::vec3 up(0, 1, 0);
        glm::vec3 sideNorm = glm::normalize(glm::cross(edge, up));
        float u1 = (float)i / 6.0f;
        float u2 = (float)(i + 1) / 6.0f;

        verts.push_back({top[i],    sideNorm, white, glm::vec2(u1, 1.0f)});
        verts.push_back({bot[i],    sideNorm, white, glm::vec2(u1, 0.0f)});
        verts.push_back({bot[next], sideNorm, white, glm::vec2(u2, 0.0f)});

        verts.push_back({top[i],    sideNorm, white, glm::vec2(u1, 1.0f)});
        verts.push_back({bot[next], sideNorm, white, glm::vec2(u2, 0.0f)});
        verts.push_back({top[next], sideNorm, white, glm::vec2(u2, 1.0f)});
    }

    return verts;
}

void initHexMesh() {
    std::vector<Vertex> verts = generateHexPrism();
    hexVertexCount = (int)verts.size();

    glGenVertexArrays(1, &hexVAO);
    glGenBuffers(1, &hexVBO);
    glBindVertexArray(hexVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hexVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

void drawHex(glm::vec3 pos, glm::vec3 color, glm::vec3 scale = glm::vec3(1.0f)) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, scale);
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(hexVAO);
    glDrawArrays(GL_TRIANGLES, 0, hexVertexCount);
}

// =====================================================
// Bake a hex prism into a vertex list instead of drawing it.
// Same transform drawHex() applies (translate * scale), but folded into the
// vertices so a whole object can be one static VBO and one draw call.
// The caller draws the result with a translation-only model matrix.
// =====================================================
const std::vector<Vertex>& hexPrismTemplate() {
    static std::vector<Vertex> tmpl = generateHexPrism();
    return tmpl;
}

void bakeHex(std::vector<Vertex>& out, glm::vec3 pos, glm::vec3 attr,
             glm::vec3 scale = glm::vec3(1.0f)) {
    const std::vector<Vertex>& tmpl = hexPrismTemplate();
    // Non-uniform scale: normals transform by the inverse-transpose, which for a
    // pure diagonal scale is just componentwise division. The shader's
    // transpose(inverse(model)) is identity here since model is translation only.
    glm::vec3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    out.reserve(out.size() + tmpl.size());
    for (const Vertex& v : tmpl) {
        Vertex o;
        o.pos      = pos + v.pos * scale;
        o.normal   = glm::normalize(v.normal * invScale);
        o.color    = attr;
        o.texCoord = v.texCoord;
        out.push_back(o);
    }
}

// =====================================================
// Box Mesh — a unit cube centred on the origin (spans +/-0.5 on every axis)
// At limb scale the hex prism reads as a 6-sided drum, which is why the mobs
// look like stacks of barrels. Mob body parts use this instead. Same Vertex
// layout and same shader path as the hex prism, so it is a drop-in swap.
// =====================================================
GLuint boxVAO = 0, boxVBO = 0;
int boxVertexCount = 0;

std::vector<Vertex> generateBox() {
    std::vector<Vertex> verts;
    glm::vec3 white(1.0f);
    const float h = 0.5f;

    // One face per axis direction: (normal, right, up) spanning the face.
    glm::vec3 norms[6] = {{ 0, 0, 1}, { 0, 0,-1}, { 1, 0, 0},
                          {-1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}};
    glm::vec3 rights[6] = {{ 1, 0, 0}, {-1, 0, 0}, { 0, 0,-1},
                           { 0, 0, 1}, { 1, 0, 0}, { 1, 0, 0}};
    glm::vec3 ups[6] = {{ 0, 1, 0}, { 0, 1, 0}, { 0, 1, 0},
                        { 0, 1, 0}, { 0, 0,-1}, { 0, 0, 1}};

    for (int f = 0; f < 6; f++) {
        glm::vec3 n = norms[f], r = rights[f] * h, u = ups[f] * h;
        glm::vec3 c = n * h;
        // Corners counter-clockwise seen from outside, so the winding matches
        // the hex prism's (GL_CCW front faces).
        glm::vec3 p0 = c - r - u, p1 = c + r - u, p2 = c + r + u, p3 = c - r + u;
        verts.push_back({p0, n, white, glm::vec2(0.0f, 0.0f)});
        verts.push_back({p1, n, white, glm::vec2(1.0f, 0.0f)});
        verts.push_back({p2, n, white, glm::vec2(1.0f, 1.0f)});
        verts.push_back({p0, n, white, glm::vec2(0.0f, 0.0f)});
        verts.push_back({p2, n, white, glm::vec2(1.0f, 1.0f)});
        verts.push_back({p3, n, white, glm::vec2(0.0f, 1.0f)});
    }
    return verts;
}

void initBoxMesh() {
    std::vector<Vertex> verts = generateBox();
    boxVertexCount = (int)verts.size();

    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
    glBindVertexArray(0);
    printf("[Mesh] Box mesh ready (%d verts)\n", boxVertexCount);
}

void drawBoxModel(glm::mat4 model, glm::vec3 color) {
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(boxVAO);
    glDrawArrays(GL_TRIANGLES, 0, boxVertexCount);
}

void drawHexRotated(glm::vec3 pos, glm::vec3 color, float angle, glm::vec3 axis, glm::vec3 scale = glm::vec3(1.0f)) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = myRotate(model, angle, axis);
    model = glm::scale(model, scale);
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(hexVAO);
    glDrawArrays(GL_TRIANGLES, 0, hexVertexCount);
}

void drawHexModel(glm::mat4 model, glm::vec3 color) {
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(hexVAO);
    glDrawArrays(GL_TRIANGLES, 0, hexVertexCount);
}

// =====================================================
// Articulated limbs — a two-segment chain with a real joint
// =====================================================
// Adapted from ../Tasfia-007-OpenGL-3.3--Medieval-European-Countryside-/Project1/
// main.cpp:846-898 (the arm and leg loops inside drawHuman).
//
// The single idea worth taking is that the lower segment's matrix is built FROM
// the upper segment's — `mFore = translate(mUpper, ...)` — rather than from the
// shoulder. That is the whole difference between a joint and a hinge painted on:
// the forearm inherits the upper arm's rotation and then adds its own, so
// bending the elbow pivots the forearm about the ELBOW. Build it from the
// shoulder instead and the forearm swings about the shoulder and detaches.
//
// Three things had to change on the way in:
//
//  * Their primitives are unit HALF-extent. hexacraft's box spans [-0.5, 0.5]
//    (see `const float h = 0.5f` in generateBox above), so their scale numbers
//    are half-lengths and mine are full lengths. Copying their figures verbatim
//    would have built every limb at half size.
//
//  * Their humanoid faces +Z and swings about (1,0,0). hexacraft has two frames:
//    mobs are built +X forward (drawMob, input.h) and the player +Z forward
//    (drawPlayer, player.h). Rather than bake an axis in, the caller passes one,
//    and passes it SIGNED so that positive swing means forward in its own frame
//    — mobs pass (0,0,1), the player passes (-1,0,0). Without that convention
//    the knee bend quietly inverts between the two models and one of them ends
//    up with backwards-hinging legs.
//
//  * They draw cylinders and spheres. hexacraft has boxes and hex prisms, so
//    `useHex` picks the primitive: mobs are box stacks, the player is hex prisms
//    like the rest of the world.
//
// Returns the UNSCALED frame at the far end of the limb — the wrist or the
// ankle — so a hand, a boot or a held item can be hung off it and inherit both
// joints. That is why the scale is applied to throwaway copies inside: letting
// it into the chain would stretch everything downstream by the limb's own
// thickness.
glm::mat4 drawLimb(const glm::mat4& base, glm::vec3 socket, glm::vec3 swingAxis,
                   float swing, float bend,
                   float upperLen, float lowerLen, float thick,
                   glm::vec3 upperCol, glm::vec3 lowerCol, bool useHex = false) {
    void (*draw)(glm::mat4, glm::vec3) = useHex ? drawHexModel : drawBoxModel;

    // Upper segment: hangs down from the socket, rotated by the shoulder/hip.
    glm::mat4 mUpper = glm::translate(base, socket);
    mUpper = myRotate(mUpper, swing, swingAxis);
    draw(glm::scale(glm::translate(mUpper, glm::vec3(0.0f, -upperLen * 0.5f, 0.0f)),
                    glm::vec3(thick, upperLen, thick)), upperCol);

    // The joint, at the far end of the upper segment. Positive `bend` folds the
    // lower segment BACKWARD — opposite to positive swing — because that is the
    // way a knee goes. Arms pass a negative bend to fold the forearm forward.
    glm::mat4 mLower = glm::translate(mUpper, glm::vec3(0.0f, -upperLen, 0.0f));
    mLower = myRotate(mLower, -bend, swingAxis);
    // Taper: the shin is thinner than the thigh, the forearm than the upper arm.
    // Uniform segments read as a stack of identical blocks rather than a limb.
    float lowerThick = thick * 0.88f;
    draw(glm::scale(glm::translate(mLower, glm::vec3(0.0f, -lowerLen * 0.5f, 0.0f)),
                    glm::vec3(lowerThick, lowerLen, lowerThick)), lowerCol);

    return glm::translate(mLower, glm::vec3(0.0f, -lowerLen, 0.0f));
}

// Every number a walking figure needs, all derived from ONE phase so the parts
// cannot drift out of sync with each other.
struct Gait {
    float bob;             // vertical rise, world units
    float sway;            // torso roll, radians
    float armL,  armR;     // shoulder swing, radians
    float legL,  legR;     // hip swing, radians
    float kneeL, kneeR;    // knee bend for the leg of the same name
};

// `phase` is in radians; `amp` scales the translation-valued parts for models
// that are not human-sized. Angles are deliberately NOT scaled by amp — a small
// figure swings its legs through the same angle, just over a shorter arc.
inline Gait makeGait(float phase, float amp = 1.0f) {
    float s = sinf(phase), sOpp = -s;   // sin(phase + PI)

    Gait g;
    // fabsf gives TWO bounces per stride, one per footfall. A plain sin bobs
    // once per stride, which reads as a limp rather than a walk.
    g.bob  = fabsf(s) * 0.06f * amp;
    g.sway = s * 0.025f;

    g.armR = s * 0.40f;      g.armL = sOpp * 0.40f;
    // Each leg runs opposite the arm on the same side. That is what makes it a
    // contralateral gait — same-side arm and leg swinging together is marching.
    g.legR = sOpp * 0.35f;   g.legL = s * 0.35f;
    // max(0, ...) bends the knee only on the forward half of the stride and
    // leaves it straight through the stance half. Bending on both halves is the
    // scissoring look that reads as skating rather than walking.
    g.kneeR = fmaxf(0.0f, sOpp) * 0.30f;
    g.kneeL = fmaxf(0.0f, s)    * 0.30f;
    return g;
}

// Walk phase in radians. The reference walks at 4.2 rad/s (their main.cpp:807).
// Each caller accumulates its own walk timer at its own rate, so each converts
// into this one cadence instead of every model inventing a tempo:
//   mob   walkTime += dt * 4.0 (hostile) / 3.0 (passive)   — input.h, moveMob
//   player walkTime += dt * 6.0                            — input.h, movement
// The timers only advance while the figure is actually moving, so a standing
// mob holds phase and its limbs come to rest instead of marching on the spot.
const float GAIT_RATE = 4.2f;

// =====================================================
// Curvy Objects: Sphere, Cone, Bezier, Spline, Ruled Surface, Wine Glass
// =====================================================
GLuint sphereVAO, sphereVBO;
int sphereVertCount = 0;
GLuint coneVAO, coneVBO;
int coneVertCount = 0;
GLuint bezierVAO, bezierVBO;
int bezierVertCount = 0;
GLuint splineVAO, splineVBO;
int splineVertCount = 0;
GLuint ruledVAO, ruledVBO;
int ruledVertCount = 0;
GLuint wineGlassVAO, wineGlassVBO;
int wineGlassVertCount = 0;

void initSphere(int stacks = 16, int slices = 24, float radius = 1.0f) {
    std::vector<Vertex> verts;
    for (int i = 0; i < stacks; i++) {
        float phi1 = PI * i / stacks;
        float phi2 = PI * (i + 1) / stacks;
        for (int j = 0; j < slices; j++) {
            float th1 = 2.0f * PI * j / slices;
            float th2 = 2.0f * PI * (j + 1) / slices;
            glm::vec3 p00(radius * sinf(phi1) * cosf(th1), radius * cosf(phi1), radius * sinf(phi1) * sinf(th1));
            glm::vec3 p10(radius * sinf(phi1) * cosf(th2), radius * cosf(phi1), radius * sinf(phi1) * sinf(th2));
            glm::vec3 p01(radius * sinf(phi2) * cosf(th1), radius * cosf(phi2), radius * sinf(phi2) * sinf(th1));
            glm::vec3 p11(radius * sinf(phi2) * cosf(th2), radius * cosf(phi2), radius * sinf(phi2) * sinf(th2));
            glm::vec3 n00 = glm::normalize(p00), n10 = glm::normalize(p10);
            glm::vec3 n01 = glm::normalize(p01), n11 = glm::normalize(p11);
            glm::vec3 w(1.0f);
            // UV mapping: spherical coordinates
            glm::vec2 uv00((float)j / slices, (float)i / stacks);
            glm::vec2 uv10((float)(j+1) / slices, (float)i / stacks);
            glm::vec2 uv01((float)j / slices, (float)(i+1) / stacks);
            glm::vec2 uv11((float)(j+1) / slices, (float)(i+1) / stacks);
            verts.push_back({p00, n00, w, uv00}); verts.push_back({p01, n01, w, uv01}); verts.push_back({p10, n10, w, uv10});
            verts.push_back({p10, n10, w, uv10}); verts.push_back({p01, n01, w, uv01}); verts.push_back({p11, n11, w, uv11});
        }
    }
    sphereVertCount = (int)verts.size();
    glGenVertexArrays(1, &sphereVAO); glGenBuffers(1, &sphereVBO);
    glBindVertexArray(sphereVAO); glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

void drawSphere(glm::vec3 pos, float radius, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(radius));
    setMat4(shaderProgram, "model", model); setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(sphereVAO); glDrawArrays(GL_TRIANGLES, 0, sphereVertCount);
}

void initCone(int slices = 24, float radius = 1.0f, float height = 2.0f) {
    std::vector<Vertex> verts;
    glm::vec3 tip(0, height, 0); glm::vec3 w(1.0f);
    for (int j = 0; j < slices; j++) {
        float th1 = 2.0f * PI * j / slices;
        float th2 = 2.0f * PI * (j + 1) / slices;
        glm::vec3 b1(radius * cosf(th1), 0, radius * sinf(th1));
        glm::vec3 b2(radius * cosf(th2), 0, radius * sinf(th2));
        glm::vec3 edge1 = tip - b1, edge2 = b2 - b1;
        glm::vec3 n = glm::normalize(glm::cross(edge2, edge1));
        // UV mapping for cone side
        glm::vec2 uvTip(((float)j + 0.5f) / slices, 1.0f);
        glm::vec2 uvB1((float)j / slices, 0.0f);
        glm::vec2 uvB2((float)(j+1) / slices, 0.0f);
        verts.push_back({tip, n, w, uvTip}); verts.push_back({b1, n, w, uvB1}); verts.push_back({b2, n, w, uvB2});
        // Base cap
        glm::vec3 dn(0, -1, 0);
        verts.push_back({glm::vec3(0,0,0), dn, w, glm::vec2(0.5f, 0.5f)});
        verts.push_back({b2, dn, w, glm::vec2(0.5f + 0.5f*cosf(th2), 0.5f + 0.5f*sinf(th2))});
        verts.push_back({b1, dn, w, glm::vec2(0.5f + 0.5f*cosf(th1), 0.5f + 0.5f*sinf(th1))});
    }
    coneVertCount = (int)verts.size();
    glGenVertexArrays(1, &coneVAO); glGenBuffers(1, &coneVBO);
    glBindVertexArray(coneVAO); glBindBuffer(GL_ARRAY_BUFFER, coneVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

void drawCone(glm::vec3 pos, float scale, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(scale));
    setMat4(shaderProgram, "model", model); setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(coneVAO); glDrawArrays(GL_TRIANGLES, 0, coneVertCount);
}

// =====================================================
// Cylinder
// =====================================================
// Adapted from ../../Downloads/gfx_b3/Cylinder.h. Added because the car's wheels
// were hex prisms laid on their side (objects.h drawWheel), which reads as a
// hexagonal wheel from anywhere close.
//
// TWO DELIBERATE DIFFERENCES FROM THEIRS.
//
// Axis is +Y, not +Z. Theirs is Z-aligned and its car compensates with an extra
// rotation at every wheel; it gets away with that because nothing else ever uses
// the primitive. Everything in this file is built Y-up — drawLimb hangs segments
// down -Y, drawCone stands on its base — so a Z-aligned cylinder would need a
// fixup rotation buried in every future call site.
//
// Unit size is the BOX convention, not the cone's: centred on the origin,
// spanning [-0.5, 0.5] on every axis, so glm::scale takes the FULL extent
// (diameter, length, diameter). drawCone is the odd one out here — it takes a
// radius and stands on y=0 — and matching the box instead means a cylinder and a
// box given the same scale vector occupy the same volume.
GLuint cylVAO = 0, cylVBO = 0;
int cylVertCount = 0;

std::vector<Vertex> generateCylinder(int slices) {
    std::vector<Vertex> verts;
    const float r = 0.5f, h = 0.5f;   // diameter 1, length 1
    const glm::vec3 w(1.0f);
    const glm::vec3 up(0, 1, 0), dn(0, -1, 0);

    for (int j = 0; j < slices; j++) {
        float t1 = 2.0f * PI * j / slices;
        float t2 = 2.0f * PI * (j + 1) / slices;
        float c1 = cosf(t1), s1 = sinf(t1);
        float c2 = cosf(t2), s2 = sinf(t2);

        // Side normals are radial and per-vertex, so the seam between slices is
        // smooth-shaded. The caps below use a flat axial normal instead, which
        // keeps the rim a hard edge — a wheel with a rounded-off rim looks
        // deflated.
        glm::vec3 n1(c1, 0, s1), n2(c2, 0, s2);
        glm::vec3 b1(r * c1, -h, r * s1), b2(r * c2, -h, r * s2);
        glm::vec3 u1(r * c1,  h, r * s1), u2(r * c2,  h, r * s2);
        float v1 = (float)j / slices, v2 = (float)(j + 1) / slices;

        // Side quad, wound CCW seen from outside.
        verts.push_back({b1, n1, w, {v1, 0.0f}});
        verts.push_back({u1, n1, w, {v1, 1.0f}});
        verts.push_back({u2, n2, w, {v2, 1.0f}});
        verts.push_back({b1, n1, w, {v1, 0.0f}});
        verts.push_back({u2, n2, w, {v2, 1.0f}});
        verts.push_back({b2, n2, w, {v2, 0.0f}});

        // Top cap — reversed winding relative to the bottom, since increasing
        // theta runs clockwise when viewed from +Y.
        verts.push_back({glm::vec3(0, h, 0), up, w, {0.5f, 0.5f}});
        verts.push_back({u2, up, w, {0.5f + 0.5f * c2, 0.5f + 0.5f * s2}});
        verts.push_back({u1, up, w, {0.5f + 0.5f * c1, 0.5f + 0.5f * s1}});

        // Bottom cap
        verts.push_back({glm::vec3(0, -h, 0), dn, w, {0.5f, 0.5f}});
        verts.push_back({b1, dn, w, {0.5f + 0.5f * c1, 0.5f + 0.5f * s1}});
        verts.push_back({b2, dn, w, {0.5f + 0.5f * c2, 0.5f + 0.5f * s2}});
    }
    return verts;
}

void initCylinder(int slices = 16) {
    std::vector<Vertex> verts = generateCylinder(slices);
    cylVertCount = (int)verts.size();
    glGenVertexArrays(1, &cylVAO); glGenBuffers(1, &cylVBO);
    glBindVertexArray(cylVAO); glBindBuffer(GL_ARRAY_BUFFER, cylVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

// Matches drawBoxModel/drawHexModel: the caller owns the full transform.
void drawCylModel(glm::mat4 model, glm::vec3 color) {
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(cylVAO);
    glDrawArrays(GL_TRIANGLES, 0, cylVertCount);
}

glm::vec3 bezierPoint(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    float u = 1.0f - t;
    return u*u*u*p0 + 3.0f*u*u*t*p1 + 3.0f*u*t*t*p2 + t*t*t*p3;
}

void initBezier() {
    glm::vec3 cp0(0, 0, 0), cp1(1, 3, 0), cp2(3, 3, 0), cp3(4, 0, 0);
    std::vector<Vertex> verts; glm::vec3 w(1.0f);
    int segments = 30; float tubeR = 0.08f; int tubeSlices = 8;
    for (int i = 0; i < segments; i++) {
        float t1 = (float)i / segments; float t2 = (float)(i + 1) / segments;
        glm::vec3 c1 = bezierPoint(cp0, cp1, cp2, cp3, t1);
        glm::vec3 c2 = bezierPoint(cp0, cp1, cp2, cp3, t2);
        glm::vec3 dir = glm::normalize(c2 - c1);
        glm::vec3 up(0, 1, 0);
        if (fabsf(glm::dot(dir, up)) > 0.99f) up = glm::vec3(1, 0, 0);
        glm::vec3 right = glm::normalize(glm::cross(dir, up));
        glm::vec3 nup = glm::normalize(glm::cross(right, dir));
        for (int j = 0; j < tubeSlices; j++) {
            float a1 = 2.0f * PI * j / tubeSlices; float a2 = 2.0f * PI * (j + 1) / tubeSlices;
            glm::vec3 n1 = cosf(a1) * right + sinf(a1) * nup;
            glm::vec3 n2 = cosf(a2) * right + sinf(a2) * nup;
            glm::vec3 p1a = c1 + tubeR * n1, p1b = c1 + tubeR * n2;
            glm::vec3 p2a = c2 + tubeR * n1, p2b = c2 + tubeR * n2;
            glm::vec2 uv0(0,0);
            verts.push_back({p1a, n1, w, uv0}); verts.push_back({p2a, n1, w, uv0}); verts.push_back({p1b, n2, w, uv0});
            verts.push_back({p1b, n2, w, uv0}); verts.push_back({p2a, n1, w, uv0}); verts.push_back({p2b, n2, w, uv0});
        }
    }
    bezierVertCount = (int)verts.size();
    glGenVertexArrays(1, &bezierVAO); glGenBuffers(1, &bezierVBO);
    glBindVertexArray(bezierVAO); glBindBuffer(GL_ARRAY_BUFFER, bezierVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

void drawBezier(glm::vec3 pos, float scale, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(scale));
    setMat4(shaderProgram, "model", model); setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(bezierVAO); glDrawArrays(GL_TRIANGLES, 0, bezierVertCount);
}

glm::vec3 catmullRom(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t) {
    float t2 = t * t, t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
                    + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

void initSpline() {
    glm::vec3 pts[] = {
        glm::vec3(0, 0, 0), glm::vec3(1, 0.5f, 1), glm::vec3(2, 0, 2.5f),
        glm::vec3(3.5f, 0.8f, 2), glm::vec3(5, 0, 0.5f), glm::vec3(6, 0.3f, -0.5f)
    };
    int nPts = 6; std::vector<Vertex> verts; glm::vec3 w(1.0f);
    float tubeR = 0.06f; int tubeSlices = 6;
    for (int seg = 0; seg < nPts - 3; seg++) {
        for (int i = 0; i < 20; i++) {
            float t1 = (float)i / 20.0f; float t2 = (float)(i + 1) / 20.0f;
            glm::vec3 c1 = catmullRom(pts[seg], pts[seg+1], pts[seg+2], pts[seg+3], t1);
            glm::vec3 c2 = catmullRom(pts[seg], pts[seg+1], pts[seg+2], pts[seg+3], t2);
            glm::vec3 dir = glm::normalize(c2 - c1);
            glm::vec3 up(0, 1, 0);
            if (fabsf(glm::dot(dir, up)) > 0.99f) up = glm::vec3(1, 0, 0);
            glm::vec3 right = glm::normalize(glm::cross(dir, up));
            glm::vec3 nup = glm::normalize(glm::cross(right, dir));
            for (int j = 0; j < tubeSlices; j++) {
                float a1 = 2.0f * PI * j / tubeSlices; float a2 = 2.0f * PI * (j + 1) / tubeSlices;
                glm::vec3 n1 = cosf(a1) * right + sinf(a1) * nup;
                glm::vec3 n2 = cosf(a2) * right + sinf(a2) * nup;
                glm::vec3 p1a = c1 + tubeR * n1, p1b = c1 + tubeR * n2;
                glm::vec3 p2a = c2 + tubeR * n1, p2b = c2 + tubeR * n2;
                glm::vec2 uv0(0,0);
                verts.push_back({p1a, n1, w, uv0}); verts.push_back({p2a, n1, w, uv0}); verts.push_back({p1b, n2, w, uv0});
                verts.push_back({p1b, n2, w, uv0}); verts.push_back({p2a, n1, w, uv0}); verts.push_back({p2b, n2, w, uv0});
            }
        }
    }
    splineVertCount = (int)verts.size();
    glGenVertexArrays(1, &splineVAO); glGenBuffers(1, &splineVBO);
    glBindVertexArray(splineVAO); glBindBuffer(GL_ARRAY_BUFFER, splineVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

void drawSpline(glm::vec3 pos, float scale, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(scale));
    setMat4(shaderProgram, "model", model); setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(splineVAO); glDrawArrays(GL_TRIANGLES, 0, splineVertCount);
}

void initRuledSurface() {
    std::vector<Vertex> verts; glm::vec3 w(1.0f); int steps = 30;
    for (int i = 0; i < steps; i++) {
        float t1 = (float)i / steps; float t2 = (float)(i + 1) / steps;
        glm::vec3 a1(t1 * 4.0f, 1.5f + 0.5f * cosf(t1 * PI * 4.0f), 0);
        glm::vec3 a2(t2 * 4.0f, 1.5f + 0.5f * cosf(t2 * PI * 4.0f), 0);
        glm::vec3 b1(t1 * 4.0f, 1.5f + 0.5f * cosf(t1 * PI * 4.0f), 2.0f);
        glm::vec3 b2(t2 * 4.0f, 1.5f + 0.5f * cosf(t2 * PI * 4.0f), 2.0f);
        glm::vec3 n = glm::normalize(glm::cross(a2 - a1, b1 - a1));
        glm::vec2 uv0(0,0);
        verts.push_back({a1, n, w, uv0}); verts.push_back({b1, n, w, uv0}); verts.push_back({a2, n, w, uv0});
        verts.push_back({a2, n, w, uv0}); verts.push_back({b1, n, w, uv0}); verts.push_back({b2, n, w, uv0});
    }
    ruledVertCount = (int)verts.size();
    glGenVertexArrays(1, &ruledVAO); glGenBuffers(1, &ruledVBO);
    glBindVertexArray(ruledVAO); glBindBuffer(GL_ARRAY_BUFFER, ruledVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

void drawRuledSurface(glm::vec3 pos, float scale, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(scale));
    setMat4(shaderProgram, "model", model); setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(ruledVAO); glDrawArrays(GL_TRIANGLES, 0, ruledVertCount);
}

// =====================================================
// Wine Glass — Surface of Revolution
// Profile curve revolved around Y-axis
// =====================================================
void initWineGlass(int slices = 24) {
    // 2D profile points (radius, height) defining wine glass cross-section
    // From bottom of base to top of rim
    struct ProfilePt { float r, y; };
    ProfilePt profile[] = {
        {0.6f, 0.0f},   // base outer edge
        {0.5f, 0.02f},  // base top
        {0.08f, 0.05f}, // stem start
        {0.06f, 0.4f},  // stem middle
        {0.06f, 0.7f},  // stem top
        {0.15f, 0.8f},  // bowl start
        {0.35f, 0.95f}, // bowl mid-low
        {0.45f, 1.15f}, // bowl widest
        {0.43f, 1.35f}, // bowl narrowing
        {0.38f, 1.5f},  // bowl upper
        {0.4f,  1.55f}, // rim flare
        {0.42f, 1.58f}, // rim top
    };
    int nProfile = sizeof(profile) / sizeof(profile[0]);

    std::vector<Vertex> verts;
    glm::vec3 w(1.0f);
    glm::vec3 glassColor(0.85f, 0.92f, 0.95f); // light glass blue

    for (int i = 0; i < nProfile - 1; i++) {
        for (int j = 0; j < slices; j++) {
            float th1 = 2.0f * PI * j / slices;
            float th2 = 2.0f * PI * (j + 1) / slices;

            float r0 = profile[i].r, y0 = profile[i].y;
            float r1 = profile[i+1].r, y1 = profile[i+1].y;

            glm::vec3 p00(r0 * cosf(th1), y0, r0 * sinf(th1));
            glm::vec3 p10(r0 * cosf(th2), y0, r0 * sinf(th2));
            glm::vec3 p01(r1 * cosf(th1), y1, r1 * sinf(th1));
            glm::vec3 p11(r1 * cosf(th2), y1, r1 * sinf(th2));

            // Compute normals from profile curve tangent
            float dr = r1 - r0;
            float dy = y1 - y0;
            // Normal in profile plane: (dy, -dr) normalized, then revolved
            float nLen = sqrtf(dy * dy + dr * dr);
            float nr = dy / nLen;  // outward radial component
            float ny = -dr / nLen; // vertical component

            glm::vec3 n00(nr * cosf(th1), ny, nr * sinf(th1));
            glm::vec3 n10(nr * cosf(th2), ny, nr * sinf(th2));
            glm::vec3 n01 = n00, n11 = n10;

            glm::vec2 uv00((float)j / slices, (float)i / (nProfile-1));
            glm::vec2 uv10((float)(j+1) / slices, (float)i / (nProfile-1));
            glm::vec2 uv01((float)j / slices, (float)(i+1) / (nProfile-1));
            glm::vec2 uv11((float)(j+1) / slices, (float)(i+1) / (nProfile-1));

            verts.push_back({p00, n00, glassColor, uv00});
            verts.push_back({p01, n01, glassColor, uv01});
            verts.push_back({p10, n10, glassColor, uv10});
            verts.push_back({p10, n10, glassColor, uv10});
            verts.push_back({p01, n01, glassColor, uv01});
            verts.push_back({p11, n11, glassColor, uv11});
        }
    }

    wineGlassVertCount = (int)verts.size();
    glGenVertexArrays(1, &wineGlassVAO); glGenBuffers(1, &wineGlassVBO);
    glBindVertexArray(wineGlassVAO); glBindBuffer(GL_ARRAY_BUFFER, wineGlassVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

void drawWineGlass(glm::vec3 pos, float scale, glm::vec3 color) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(scale));
    setMat4(shaderProgram, "model", model); setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(wineGlassVAO); glDrawArrays(GL_TRIANGLES, 0, wineGlassVertCount);
}

// =====================================================
// Slab Mesh — half-height hex prism
// Centered at origin: y in [-HEX_HEIGHT/4, +HEX_HEIGHT/4]
// drawSlab positions it at bottom or top half of a full block
// =====================================================
GLuint slabVAO, slabVBO;
int slabVertCount = 0;

void initSlabMesh() {
    std::vector<Vertex> verts;
    float r = HEX_RADIUS;
    float h = HEX_HEIGHT * 0.5f; // half height
    float halfH = h / 2.0f;

    glm::vec3 top[6], bot[6];
    for (int i = 0; i < 6; i++) {
        float angle = PI / 3.0f * i + PI / 6.0f;
        float x = r * cosf(angle);
        float z = r * sinf(angle);
        top[i] = glm::vec3(x,  halfH, z);
        bot[i] = glm::vec3(x, -halfH, z);
    }

    glm::vec3 white(1.0f);
    glm::vec3 topCenter(0,  halfH, 0);
    glm::vec3 botCenter(0, -halfH, 0);

    // Top cap
    for (int i = 0; i < 6; i++) {
        int next = (i + 1) % 6;
        verts.push_back({topCenter, glm::vec3(0,1,0), white, glm::vec2(0.5f, 0.5f)});
        verts.push_back({top[i],    glm::vec3(0,1,0), white, glm::vec2(0.5f + 0.5f*cosf(PI/3.0f*i),       0.5f + 0.5f*sinf(PI/3.0f*i))});
        verts.push_back({top[next], glm::vec3(0,1,0), white, glm::vec2(0.5f + 0.5f*cosf(PI/3.0f*(i+1)), 0.5f + 0.5f*sinf(PI/3.0f*(i+1)))});
    }
    // Bottom cap
    for (int i = 0; i < 6; i++) {
        int next = (i + 1) % 6;
        verts.push_back({botCenter, glm::vec3(0,-1,0), white, glm::vec2(0.5f, 0.5f)});
        verts.push_back({bot[next], glm::vec3(0,-1,0), white, glm::vec2(0,0)});
        verts.push_back({bot[i],    glm::vec3(0,-1,0), white, glm::vec2(0,0)});
    }
    // 6 side faces
    for (int i = 0; i < 6; i++) {
        int next = (i + 1) % 6;
        glm::vec3 edge = top[next] - top[i];
        glm::vec3 sideNorm = glm::normalize(glm::cross(edge, glm::vec3(0,1,0)));
        float u1 = (float)i / 6.0f, u2 = (float)(i+1) / 6.0f;
        verts.push_back({top[i],    sideNorm, white, glm::vec2(u1, 1.0f)});
        verts.push_back({bot[i],    sideNorm, white, glm::vec2(u1, 0.0f)});
        verts.push_back({bot[next], sideNorm, white, glm::vec2(u2, 0.0f)});
        verts.push_back({top[i],    sideNorm, white, glm::vec2(u1, 1.0f)});
        verts.push_back({bot[next], sideNorm, white, glm::vec2(u2, 0.0f)});
        verts.push_back({top[next], sideNorm, white, glm::vec2(u2, 1.0f)});
    }

    slabVertCount = (int)verts.size();
    glGenVertexArrays(1, &slabVAO); glGenBuffers(1, &slabVBO);
    glBindVertexArray(slabVAO); glBindBuffer(GL_ARRAY_BUFFER, slabVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

// topHalf=true  → slab sits in upper half of block (+HEX_HEIGHT/4 offset)
// topHalf=false → slab sits in lower half of block (-HEX_HEIGHT/4 offset)
void drawSlab(glm::vec3 blockPos, glm::vec3 color, bool topHalf = false) {
    float yOff = topHalf ? HEX_HEIGHT * 0.25f : -HEX_HEIGHT * 0.25f;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), blockPos + glm::vec3(0, yOff, 0));
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(slabVAO);
    glDrawArrays(GL_TRIANGLES, 0, slabVertCount);
}

// =====================================================
// Stair — drawn as two slab draws (no custom VAO needed)
// facing: 0=+Z, 1=+X, 2=-Z, 3=-X  (which direction the step goes up)
// =====================================================
void drawStair(glm::vec3 blockPos, glm::vec3 color, int facing = 0) {
    // Bottom slab (full width, bottom half)
    glm::mat4 mBot = glm::translate(glm::mat4(1.0f), blockPos + glm::vec3(0, -HEX_HEIGHT * 0.25f, 0));
    setMat4(shaderProgram, "model", mBot);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(slabVAO);
    glDrawArrays(GL_TRIANGLES, 0, slabVertCount);

    // Top slab (half depth, top half) — scale Z by 0.5, offset toward back
    static const glm::vec3 offsets[4] = {
        glm::vec3(0, 0, -HEX_RADIUS * 0.5f),  // facing +Z: step goes forward → back half raised
        glm::vec3(-HEX_RADIUS * 0.5f, 0, 0),
        glm::vec3(0, 0,  HEX_RADIUS * 0.5f),
        glm::vec3( HEX_RADIUS * 0.5f, 0, 0),
    };
    glm::vec3 topPos = blockPos + glm::vec3(0, HEX_HEIGHT * 0.25f, 0) + offsets[facing & 3];
    glm::mat4 mTop = glm::translate(glm::mat4(1.0f), topPos);
    // Scale to half depth
    if (facing == 0 || facing == 2)
        mTop = glm::scale(mTop, glm::vec3(1.0f, 1.0f, 0.5f));
    else
        mTop = glm::scale(mTop, glm::vec3(0.5f, 1.0f, 1.0f));
    setMat4(shaderProgram, "model", mTop);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(slabVAO);
    glDrawArrays(GL_TRIANGLES, 0, slabVertCount);
}

// =====================================================
// Panel Mesh — flat double-sided quad, 1×1 in XY, facing +Z
// Used for: doors, trapdoors, glass panes, iron bars, ladders, signs, banners
// =====================================================
GLuint panelVAO, panelVBO;
int panelVertCount = 0;

void initPanelMesh() {
    // Unit quad centered at origin in XY plane, facing ±Z
    // Corners: (-0.5,-0.5,0), (0.5,-0.5,0), (0.5,0.5,0), (-0.5,0.5,0)
    glm::vec3 white(1.0f);
    glm::vec3 fwd(0, 0, 1), bwd(0, 0, -1);

    glm::vec3 p[4] = {
        {-0.5f, -0.5f, 0},
        { 0.5f, -0.5f, 0},
        { 0.5f,  0.5f, 0},
        {-0.5f,  0.5f, 0},
    };
    glm::vec2 uv[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
    };

    std::vector<Vertex> verts;
    // Front face (+Z normal)
    verts.push_back({p[0], fwd, white, uv[0]});
    verts.push_back({p[1], fwd, white, uv[1]});
    verts.push_back({p[2], fwd, white, uv[2]});
    verts.push_back({p[0], fwd, white, uv[0]});
    verts.push_back({p[2], fwd, white, uv[2]});
    verts.push_back({p[3], fwd, white, uv[3]});
    // Back face (-Z normal)
    verts.push_back({p[0], bwd, white, uv[1]});
    verts.push_back({p[2], bwd, white, uv[3]});
    verts.push_back({p[1], bwd, white, uv[0]});
    verts.push_back({p[0], bwd, white, uv[1]});
    verts.push_back({p[3], bwd, white, uv[2]});
    verts.push_back({p[2], bwd, white, uv[3]});

    panelVertCount = (int)verts.size();
    glGenVertexArrays(1, &panelVAO); glGenBuffers(1, &panelVBO);
    glBindVertexArray(panelVAO); glBindBuffer(GL_ARRAY_BUFFER, panelVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
}

// Draw a panel given a parent transform matrix (caller handles position/rotation)
// width/height scale the 1×1 base mesh to the desired block face size
void drawPanel(glm::mat4 parentModel, glm::vec3 color, float width = 1.0f, float height = 1.0f) {
    glm::mat4 model = glm::scale(parentModel, glm::vec3(width, height, 1.0f));
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(panelVAO);
    glDrawArrays(GL_TRIANGLES, 0, panelVertCount);
}

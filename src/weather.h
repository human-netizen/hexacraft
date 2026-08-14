#pragma once
#include <cstdlib>   // rand, RAND_MAX
// =====================================================
// Weather — rain
// =====================================================
// Adapted from ../Tasfia-007-OpenGL-3.3--Medieval-European-Countryside-/Project1/main.cpp:2392-2460.
//
// The idea worth copying is that a drop's horizontal position is stored
// RELATIVE TO THE CAMERA. A fixed box of rain then follows the player forever,
// so a few hundred drops cover the whole visible near field and there is never a
// storm front to walk out of. Everything else falls out of that: one VAO, one
// dynamic VBO refilled per frame, one glDrawArrays, and four floats of state per
// drop.
//
// The one thing that could not be copied is the respawn test. Theirs is flat
// ground, so a drop restarts when it passes y = -0.4. hexacraft has real terrain,
// and a fixed floor would send rain straight through every hill and leave it
// pooling underground where it is visible through cave mouths. Here a drop
// restarts when it passes the ground height of the column it is currently over.

const int   MAX_RAINDROPS = 2000;
// Radius of the disc that follows the camera. 24 units is comfortably inside
// RENDER_DIST (50) — rain past the point where terrain has already fogged out
// would be lines hanging in front of empty sky.
const float RAIN_RADIUS   = 24.0f;
// Spawn height above the camera. Tall enough that the top of the column is off
// the top of the screen at a normal pitch, so drops are never seen appearing.
const float RAIN_TOP      = 20.0f;
// Drops are spread over the disc as r = RAIN_RADIUS * u^RAIN_CROWD (u uniform in
// 0..1), which packs them toward the camera instead of spreading them evenly.
//
// This is the fix for the first attempt, which used a uniformly filled square
// box. Looking at the sky that looked like rain, because the eye integrates
// along a ray that crosses the whole box and accumulates plenty of drops.
// Looking at the GROUND the ray hits terrain within a few units, so only the
// near field counts — and the near field of an evenly filled 52-unit box holds
// almost nothing. The capture had a visibly empty lower half.
//
// Raising density evenly would have meant shrinking the disc, which trades the
// empty foreground for a visible wall where the rain stops. Biasing the radius
// keeps drops out at 24 units AND fills the foreground.
const float RAIN_CROWD    = 1.5f;

struct RainDrop {
    float dx, dz;   // offset from the camera, world units
    float y;        // absolute world height — the ground test needs a real Y
    float spd;      // fall speed, units/sec
};

static RainDrop g_rain[MAX_RAINDROPS];
static GLuint   g_rainVAO = 0, g_rainVBO = 0;
// Two vertices (one line) per drop, three floats each.
static float    g_rainVerts[MAX_RAINDROPS * 6];
static int      g_rainLineCount = 0;

// Draws from the global rand() sequence. That is safe in a way srand() would not
// be — see the note in horizon.h about why clouds use a local hash instead. This
// only *consumes* values, so terrain generation, which seeds the sequence in
// initBlockGrid() and has finished long before initRain() runs, is unaffected.
static float rainRand() { return (float)rand() / (float)RAND_MAX; }

// Put a drop back at the top of the column, at a fresh random spot.
// Re-randomising XZ as well as Y matters: a drop that only resets its height
// would fall down the same line forever and the field would visibly become a set
// of fixed vertical tracks within a few seconds.
static void respawnDrop(RainDrop& d, float camY) {
    float ang = rainRand() * 6.2831853f;
    float r   = RAIN_RADIUS * powf(rainRand(), RAIN_CROWD);
    d.dx  = cosf(ang) * r;
    d.dz  = sinf(ang) * r;
    d.y   = camY + RAIN_TOP * (0.75f + rainRand() * 0.45f);
    d.spd = 26.0f + rainRand() * 16.0f;
}

void initRain() {
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        respawnDrop(g_rain[i], camPos.y);
        // Spread the initial fill over the whole column instead of the spawn
        // band, or the first second of rain arrives as one solid sheet.
        g_rain[i].y = camPos.y + rainRand() * RAIN_TOP * 1.2f;
    }

    glGenVertexArrays(1, &g_rainVAO);
    glGenBuffers(1, &g_rainVBO);
    glBindVertexArray(g_rainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_rainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_rainVerts), nullptr, GL_DYNAMIC_DRAW);
    // Position only. Attributes 1..3 (normal, colour, uv) are left disabled:
    // rain draws through the shader's isEmissive path, which returns before it
    // touches any of them.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    printf("[Rain] %d drops, 1 draw call\n", MAX_RAINDROPS);
}

// Advances every drop and rebuilds the vertex array. Must run once per frame,
// NOT once per viewport — in 4-viewport mode the four eyes share one rain field,
// and stepping it four times would quadruple the fall speed.
void updateRain(float dt, float time) {
    g_rainLineCount = 0;
    if (!rainOn) return;

    // Clamp: a long stall (world gen, a breakpoint) would otherwise teleport
    // every drop far below the ground in a single step and empty the sky.
    if (dt > 0.1f) dt = 0.1f;

    // One slow gust for the whole field rather than the per-position phase used
    // by the foliage sway. Rain is seen as a mass of parallel streaks, and
    // varying the slant between neighbouring drops destroys exactly that: it
    // reads as noise instead of as wind. So this is deliberately NOT the
    // vertexShader.glsl phase.
    float gustX = sinf(time * 0.35f) * 0.30f;
    float gustZ = cosf(time * 0.27f) * 0.22f;

    for (int i = 0; i < MAX_RAINDROPS; i++) {
        RainDrop& d = g_rain[i];
        d.y -= d.spd * dt;

        float wx = camPos.x + d.dx;
        float wz = camPos.z + d.dz;

        // Ground test. findGroundH() (world.h) starts from columnMaxH and uses
        // blocksSight(), so leaves and glass do not stop a drop but a roof does.
        int col, row, gh;
        worldToGrid(glm::vec3(wx, 0.0f, wz), col, row, gh);
        int solid = findGroundH(col, row);
        // solid < 0 means the column is off the grid or has nothing in it. The
        // rain box is 26 units and the world is finite, so this happens for real
        // near the map edge; a drop there has no ground to land on and would fall
        // forever, so it is recycled at once. The new position may be off-grid
        // too, in which case it is simply recycled again next frame.
        float groundY = (solid >= 0) ? (float)(solid + 1) * HEX_HEIGHT : d.y + 1.0f;

        // Respawn on landing, and also whenever a drop has ended up far above the
        // band it belongs in — that happens when the player descends quickly or
        // teleports, and without it the sky above them stays empty until the old
        // drops have fallen all the way down.
        if (d.y <= groundY || d.y > camPos.y + RAIN_TOP * 2.0f) {
            respawnDrop(d, camPos.y);
            continue;
        }

        // Drops almost touching the camera are skipped. A 1.5-unit streak one
        // unit from the eye stretches across the entire screen as a single
        // hairline, which reads as scratched glass rather than as weather. The
        // drop keeps falling, it is just not drawn until it is far enough out.
        float ny = d.y - camPos.y;
        if (d.dx * d.dx + d.dz * d.dz + ny * ny < 2.25f) continue;

        // Streak length scales with speed, so a fast drop draws a longer line.
        // This is what makes still-frame rain read as falling rather than as a
        // scatter of dashes.
        float len = d.spd * 0.045f;

        float* v = &g_rainVerts[g_rainLineCount * 6];
        v[0] = wx;                 v[1] = d.y;         v[2] = wz;
        v[3] = wx + gustX * len;   v[4] = d.y + len;   v[5] = wz + gustZ * len;
        g_rainLineCount++;
    }

    glBindBuffer(GL_ARRAY_BUFFER, g_rainVBO);
    // Orphan first: the previous frame's contents are never read again, and
    // handing the driver a fresh block means it does not have to stall waiting
    // for the in-flight draw to finish with the old one.
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_rainVerts), nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, g_rainLineCount * 6 * sizeof(float), g_rainVerts);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Call after the terrain and object passes — rain is transparent and must be
// composited over the solid world. Uses the main shader and the view/projection
// already uploaded for the current viewport.
void drawRain(float time) {
    if (!rainOn || g_rainLineCount == 0) return;

    setMat4(shaderProgram, "model", glm::mat4(1.0f));
    setSway(0.0f);
    setBool(shaderProgram, "isEmissive", true);

    // Rain is drawn emissive because a two-vertex line has no meaningful normal
    // to light it by. That path multiplies by `0.8 + 0.2 * sin(time * 3.0)` —
    // a throb intended for torch flames, which applied to a whole rain field
    // is a 3 Hz strobe over the entire screen. `time` here is the same value
    // main.cpp uploads to the shader, so dividing it back out cancels exactly.
    float pulse = 0.8f + 0.2f * sinf(time * 3.0f);
    setVec3(shaderProgram, "emissiveColor", glm::vec3(0.62f, 0.72f, 0.88f) / pulse);

    setFloat(shaderProgram, "alpha", 0.45f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Depth test stays ON so hills and roofs occlude the rain behind them, but
    // writes are OFF: 2000 alpha-blended lines writing depth would each punch a
    // hole that later drops fail against, and the field would visibly thin out
    // depending on draw order.
    glDepthMask(GL_FALSE);

    glBindVertexArray(g_rainVAO);
    glDrawArrays(GL_LINES, 0, g_rainLineCount * 2);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    setFloat(shaderProgram, "alpha", 1.0f);
    setBool(shaderProgram, "isEmissive", false);
}

void destroyRain() {
    glDeleteVertexArrays(1, &g_rainVAO);
    glDeleteBuffers(1, &g_rainVBO);
}

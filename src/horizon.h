#pragma once
// =====================================================
// Horizon — distant mountain ranges ringing the world
// =====================================================
// The problem this solves: hexacraft's terrain is drawn out to RENDER_DIST (50
// units) and fogged to the sky colour by the time it gets there. Past that there
// was nothing at all between the fade-out and the cubemap painted at infinity, so
// the eye read the fog boundary as the edge of a small world sitting under a
// wallpaper.
//
// Two concentric rings of procedural ridgeline sit well past the terrain cutoff
// and well inside the far plane. Their bases are painted in the frame's own sky
// colour — the exact colour terrain dissolves into — so the join is invisible and
// only the peaks read as separate objects. That single cue is most of what makes
// a world feel large.
//
// Approach borrowed from the sky dome in
// ../Tasfia-007-OpenGL-3.3--Medieval-European-Countryside-/Project1/main.cpp:269
// but the panorama texture is replaced with generated geometry: their version
// needs a painted horizon image, and a procedurally generated world should not
// have a hand-painted skyline stapled to it.
//
// Uses its own small shader, following the pattern skybox.h already establishes.
// The main shader would need lighting, fog and emissive all suppressed and a
// per-vertex haze blend added to do this, which is more disruption than a
// 20-line program is worth.

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

// =====================================================
// Shader
// =====================================================
static const char* horizonVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
// Reuses the engine's Vertex layout so setupVertexAttribs() can be shared.
// Location 2 is the colour slot, repurposed here as (rockShade, haze, unused).
layout(location = 2) in vec3 aAttr;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 camXZ;      // ring follows the camera on XZ, stays put on Y

out vec2 vAttr;

void main() {
    vAttr = aAttr.xy;
    gl_Position = projection * view * vec4(aPos + camXZ, 1.0);
}
)GLSL";

static const char* horizonFragSrc = R"GLSL(
#version 330 core
in vec2 vAttr;           // x = rock shade, y = how much haze to blend in
out vec4 FragColor;

uniform vec3  rockColor;  // lit rock tint for this time of day
uniform vec3  hazeColor;  // the frame's sky/fog colour
uniform float hazeBias;   // per-ring: farther ranges sit deeper in haze

void main() {
    vec3 rock = rockColor * vAttr.x;
    // Haze is strongest at the base of the ridge and weakest at the peaks. That
    // is the right way round for this job: the base has to melt into the band of
    // fog the terrain disappears into, while the peaks stay distinct enough to
    // read as mountains standing above it.
    float haze = clamp(vAttr.y + hazeBias, 0.0, 1.0);
    FragColor = vec4(mix(rock, hazeColor, haze), 1.0);
}
)GLSL";

// =====================================================
// State
// =====================================================
static GLuint horizonShader = 0;
struct HorizonRing {
    GLuint vao = 0, vbo = 0;
    int count = 0;
    float darken = 1.0f;   // nearer rings are darker, for aerial perspective
    float hazeBias = 0.0f; // farther rings sit deeper in haze
};
static HorizonRing g_horizonRings[2];

// =====================================================
// Build one ring of ridgeline
// =====================================================
// segments : columns around the circle. 192 gives roughly a 2-degree step, fine
//            enough that the ridgeline reads as a curve rather than a polygon.
// radius   : distance from the camera. Must be > RENDER_DIST so it never
//            intersects real terrain, and < the 200-unit far plane.
// noiseFreq/seed : different values per ring, so the two ranges are not the same
//            silhouette at two scales.
static void buildHorizonRing(HorizonRing& ring, int segments, float radius,
                             float baseY, float minH, float maxH,
                             float noiseFreq, float seed) {
    // Ridge height as a function of angle. Sampled on the *circle* — at
    // (cos t, sin t) — rather than on the raw angle t. Sampling a 1-D noise on t
    // directly would make theta = 0 and theta = 2*pi land on unrelated noise
    // values and leave a visible step in the skyline; on the circle the wrap is
    // seamless by construction, with no need for the mirrored-repeat trick their
    // panorama version depends on.
    auto ridgeAt = [&](float theta) {
        float cx = cosf(theta) * noiseFreq + seed;
        float cz = sinf(theta) * noiseFreq + seed;
        // Low frequency term first, so the range has a few distinct massifs
        // instead of uniform noise; higher octaves then roughen the peaks.
        float big  = fbmNoise(cx * 0.5f, cz * 0.5f, 2);
        float fine = fbmNoise(cx * 2.3f, cz * 2.3f, 3);
        float n = big * 0.68f + fine * 0.32f;
        // Push toward the extremes so there are real summits and real saddles
        // rather than everything hovering near the mean.
        n = n * n * (3.0f - 2.0f * n);
        return baseY + minH + n * (maxH - minH);
    };

    std::vector<Vertex> verts;
    verts.reserve(segments * 6);

    for (int i = 0; i < segments; i++) {
        float t0 = (float)i / segments * 2.0f * PI;
        float t1 = (float)(i + 1) / segments * 2.0f * PI;
        float h0 = ridgeAt(t0), h1 = ridgeAt(t1);

        glm::vec3 p0(cosf(t0) * radius, baseY, sinf(t0) * radius);
        glm::vec3 p1(cosf(t1) * radius, baseY, sinf(t1) * radius);
        glm::vec3 p2(cosf(t1) * radius, h1,    sinf(t1) * radius);
        glm::vec3 p3(cosf(t0) * radius, h0,    sinf(t0) * radius);

        // Normals are never read — the horizon shader does no lighting — but the
        // Vertex layout carries them, so fill in something sane (inward).
        glm::vec3 n0 = glm::normalize(glm::vec3(-cosf(t0), 0.0f, -sinf(t0)));
        glm::vec3 n1 = glm::normalize(glm::vec3(-cosf(t1), 0.0f, -sinf(t1)));

        // attr = (rock shade, haze amount). Both keyed off height up the column:
        // higher is darker rock and less haze.
        auto attrAt = [&](float y, float peak) {
            float f = (peak > baseY + 0.001f) ? (y - baseY) / (peak - baseY) : 0.0f;
            float shade = 0.50f + 0.50f * f;   // base in shadow, summits catching light
            // 0.92 at the base so it melts into the fog band the terrain fades
            // into, 0.30 at the summit so the peaks still read as solid rock.
            // Tuned by eye against a capture: the first pass ran 1.0 -> 0.45 and
            // the ranges washed out to almost nothing at player height.
            float haze  = 0.92f - 0.62f * f;
            return glm::vec3(shade, haze, 0.0f);
        };
        glm::vec3 aBot0 = attrAt(baseY, h0), aTop0 = attrAt(h0, h0);
        glm::vec3 aBot1 = attrAt(baseY, h1), aTop1 = attrAt(h1, h1);

        // Two triangles per column. No back-face culling is enabled anywhere in
        // this engine, so winding order does not matter here.
        verts.push_back({p0, n0, aBot0, {0.0f, 0.0f}});
        verts.push_back({p1, n1, aBot1, {1.0f, 0.0f}});
        verts.push_back({p2, n1, aTop1, {1.0f, 1.0f}});

        verts.push_back({p0, n0, aBot0, {0.0f, 0.0f}});
        verts.push_back({p2, n1, aTop1, {1.0f, 1.0f}});
        verts.push_back({p3, n0, aTop0, {0.0f, 1.0f}});
    }

    ring.count = (int)verts.size();
    glGenVertexArrays(1, &ring.vao);
    glGenBuffers(1, &ring.vbo);
    glBindVertexArray(ring.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ring.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    setupVertexAttribs();
    glBindVertexArray(0);
}

// =====================================================
// Public API
// =====================================================
void initHorizon() {
    horizonShader = compileSkyboxShader(horizonVertSrc, horizonFragSrc);

    // Two ranges at different distances. One ring alone still reads as a flat
    // backdrop; two, with the nearer one darker and less hazy, gives the
    // aerial-perspective cue that says "these are at different distances" and is
    // what actually sells depth.
    //
    // Radii sit past RENDER_DIST (50) and inside the far plane (200). The sun and
    // moon are drawn at 150 in renderSky(), so the far ring is kept off that
    // exact distance to avoid z-fighting with a sun disc sitting on the horizon
    // at dawn.
    //
    // Terrain surface sits around y = 20-26 (UNDERGROUND_DEPTH = 10 plus terrain
    // height), so the ridge bases start below that and the summits clear it.
    buildHorizonRing(g_horizonRings[0], 192, 168.0f, 0.0f, 26.0f, 52.0f, 3.1f,  0.0f);
    g_horizonRings[0].darken = 0.90f;
    g_horizonRings[0].hazeBias = 0.14f;

    buildHorizonRing(g_horizonRings[1], 192, 122.0f, 0.0f, 18.0f, 38.0f, 5.7f, 41.0f);
    g_horizonRings[1].darken = 1.0f;
    g_horizonRings[1].hazeBias = 0.0f;

    printf("[Horizon] Built 2 rings, %d triangles total\n",
           (g_horizonRings[0].count + g_horizonRings[1].count) / 3);
}

// Draw both ranges. Call AFTER the skybox and BEFORE the terrain.
//   eye     — this viewport's eye position. Passed in rather than read from the
//             global camPos because the 4-viewport mode (V) renders the scene
//             four times from four different eyes, and a ring centred on the
//             player's camera would sit off to one side in the other three.
//   skyTint — the frame's sky/fog colour, so ridge bases match what terrain
//             fades into.
void drawHorizon(const glm::mat4& viewMat, const glm::mat4& projMat,
                 glm::vec3 eye, float dayF, glm::vec3 skyTint) {
    if (!horizonShader) return;

    // Rock tint follows the time of day. Never fully black at night, or the
    // ranges vanish and take the depth cue with them; the moon rim on real
    // terrain would have no counterpart on the horizon.
    glm::vec3 rockDay(0.34f, 0.37f, 0.45f);
    glm::vec3 rockNight(0.06f, 0.07f, 0.12f);
    glm::vec3 rock = glm::mix(rockNight, rockDay, glm::clamp(dayF, 0.0f, 1.0f));

    // Depth writes off, exactly as renderSky() does for the sun and stars: the
    // rings must never occlude anything, and real terrain drawn afterwards has to
    // win the depth test against whatever is behind it, not against the horizon.
    glDepthMask(GL_FALSE);
    glUseProgram(horizonShader);
    setMat4(horizonShader, "view", viewMat);
    setMat4(horizonShader, "projection", projMat);
    setVec3(horizonShader, "hazeColor", skyTint);

    // Farthest ring first, so the nearer range paints over it.
    for (int i = 0; i < 2; i++) {
        const HorizonRing& r = g_horizonRings[i];
        if (!r.vao) continue;
        setVec3(horizonShader, "camXZ", glm::vec3(eye.x, 0.0f, eye.z));
        setVec3(horizonShader, "rockColor", rock * r.darken);
        setFloat(horizonShader, "hazeBias", r.hazeBias);
        glBindVertexArray(r.vao);
        glDrawArrays(GL_TRIANGLES, 0, r.count);
    }
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
}

// =====================================================
// Clouds
// =====================================================
// hexacraft deliberately removed its hex clouds in favour of the cubemap's
// painted ones (see the note at world.h:3148). That decision is not undone here:
// cubemap clouds are painted at *infinity* and never move, so no matter how far
// the player walks they sit at exactly the same place in the frame. Parallax is
// the one depth cue a cubemap structurally cannot provide, and it needs geometry
// at a finite altitude. These clouds exist for that and nothing else.
//
// Adapted from ../Tasfia-007.../Project1/main.cpp:1089-1128 — overlapping
// squashed spheres, shape frozen per cloud, drifting on one axis.

// Their version calls srand(seed) per cloud to freeze its shape. That is not safe
// here: initBlockGrid() seeds the global sequence for terrain generation, and
// clobbering it mid-frame would change the world. A local hash gives the same
// stability with no global state at all, and needs no storage — a cloud's shape is
// recomputed from its cell coordinates every frame and is identical every time.
static unsigned int cloudHash(int x, int z, int i) {
    unsigned int s = (unsigned int)x * 374761393u
                   + (unsigned int)z * 668265263u
                   + (unsigned int)i * 2246822519u;
    s = (s ^ (s >> 13)) * 1274126177u;
    return s ^ (s >> 16);
}
static float cloudRand(int x, int z, int i) {
    return (float)(cloudHash(x, z, i) & 0xFFFFFFu) / 16777215.0f;
}

// Squashed sphere. drawSphere() only takes a uniform radius, and a uniform sphere
// reads as a ball rather than as cloud; flattening it vertically is most of what
// makes a cluster of them look like weather.
static void drawPuff(glm::vec3 pos, glm::vec3 scale, glm::vec3 color) {
    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), pos), scale);
    setMat4(shaderProgram, "model", model);
    setVec3(shaderProgram, "objectColor", color);
    glBindVertexArray(sphereVAO);
    glDrawArrays(GL_TRIANGLES, 0, sphereVertCount);
}

// Call after renderSky and before the terrain pass. Uses the main shader.
void drawClouds(float time, glm::vec3 eye, float dayF) {
    const float CELL = 90.0f;   // one candidate cloud per cell of this size
    const int   SPAN = 2;       // cells either side of the camera -> 5x5 = 25 candidates

    // Drift. Shifting the *sampling grid* by the drift rather than moving each
    // cloud within a fixed cell means a cloud never has to be handed over from one
    // cell to the next, so none of them pop.
    float drift = time * 0.6f;

    int baseCX = (int)floorf((eye.x - drift) / CELL);
    int baseCZ = (int)floorf(eye.z / CELL);

    // Sky objects are drawn without fog for the same reason renderSky() does it:
    // these sit far past the fog cutoff and distance fog would erase them
    // outright. Depth writes off so they never occlude terrain.
    glDepthMask(GL_FALSE);
    setFloat(shaderProgram, "fogDensity", 0.0f);
    setInt(shaderProgram, "textureMode", 0);

    // Deliberately NOT drawn with isEmissive. That path multiplies by
    // `0.8 + 0.2 * sin(time * 3.0)` (fragmentShader.glsl) — a deliberate throb
    // that suits a torch flame and would make every cloud in the sky pulse in
    // unison. Ordinary lit geometry is also simply better here: the sun picks out
    // the tops of the puffs and leaves their undersides darker, which is most of
    // what gives a cloud its form.
    //
    // Specular strength forced to zero, though: a shiny cloud is a soap bubble.
    uploadSpecular(16.0f, 0.0f);

    // Clouds are lit by the sky, so they follow the time of day. Never fully
    // black — a moonlit cloud is still visible, and losing them at night would
    // lose the parallax cue exactly when the sky is emptiest.
    // Deliberately far brighter than white. The shader tone-maps with
    // result/(result+1), so an objectColor of 1.0 lands around 0.6 after ambient
    // and diffuse — mid-grey, which read as floating rocks in the first capture,
    // not as cloud. Driving the input well past 1 pushes the output up against
    // the asymptote so the lit tops come out near-white while the undersides
    // still fall away, which is what gives a cloud its shape.
    glm::vec3 cloudColor = glm::mix(glm::vec3(0.55f, 0.60f, 0.78f),
                                    glm::vec3(3.4f, 3.4f, 3.5f),
                                    glm::clamp(dayF, 0.0f, 1.0f));

    for (int dz = -SPAN; dz <= SPAN; dz++) {
        for (int dx = -SPAN; dx <= SPAN; dx++) {
            int cx = baseCX + dx, cz = baseCZ + dz;

            // Only some cells carry a cloud, or the sky is a solid overcast lid.
            if (cloudRand(cx, cz, 0) < 0.38f) continue;

            // High enough to stay above the player even in free-fly, and above
            // anything buildable in a 32-layer grid.
            glm::vec3 c(((float)cx + cloudRand(cx, cz, 1)) * CELL + drift,
                        68.0f + cloudRand(cx, cz, 2) * 22.0f,
                        ((float)cz + cloudRand(cx, cz, 3)) * CELL);

            float radius = 13.0f + cloudRand(cx, cz, 4) * 12.0f;

            // One frustum test for the whole cluster rather than per puff.
            if (!isInFrustum(c, radius * 2.2f)) continue;

            int puffs = 7 + (int)(cloudRand(cx, cz, 5) * 4.0f);   // 7..10
            for (int i = 0; i < puffs; i++) {
                glm::vec3 off((cloudRand(cx, cz, 10 + i * 3) - 0.5f) * radius * 2.0f,
                              (cloudRand(cx, cz, 11 + i * 3) - 0.5f) * radius * 0.35f,
                              (cloudRand(cx, cz, 12 + i * 3) - 0.5f) * radius * 1.4f);
                float s = radius * (0.50f + cloudRand(cx, cz, 40 + i) * 0.45f);
                // Heavily flattened. Puffs must also overlap generously — a
                // cluster of separated spheres reads as a handful of balls, and
                // only once they intersect does the silhouette turn into cloud.
                drawPuff(c + off, glm::vec3(s, s * 0.30f, s * 0.75f), cloudColor);
            }
        }
    }

    glBindVertexArray(0);
    resetBlockSpecular();
    setFloat(shaderProgram, "fogDensity", currentFogDensity);
    glDepthMask(GL_TRUE);
}

void destroyHorizon() {
    for (int i = 0; i < 2; i++) {
        glDeleteVertexArrays(1, &g_horizonRings[i].vao);
        glDeleteBuffers(1, &g_horizonRings[i].vbo);
    }
    glDeleteProgram(horizonShader);
}

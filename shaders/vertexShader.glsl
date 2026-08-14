#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
layout(location = 3) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Gouraud shading uniforms (duplicated from fragment for per-vertex lighting)
uniform bool useGouraud;
uniform vec3 objectColor;
uniform bool lightOn;
uniform bool dirLightOn, pointLightOn, spotLightOn;
uniform bool ambientOn, diffuseOn, specularOn;
uniform vec3 dirLightDir, dirLightColor;
uniform vec3 moonLightDir, moonLightColor;
uniform float specPower;
uniform float specStrength;
uniform vec3 pointLightPos[8];
uniform vec3 pointLightColor[8];
uniform int numPointLights;
uniform vec3 spotLightPos, spotLightDir;
uniform vec3 spotLightColor;
uniform float spotCutoff;
uniform vec3 viewPos;
uniform float dayFactor;

// Baked-tree base colour — see the matching block in fragmentShader.glsl
uniform int colorMode;
uniform vec3 woodColor;
uniform vec3 leafColor;

// Sub-rectangle of the bound texture to sample, as (scaleU, scaleV, offU, offV).
// Identity is (1,1,0,0). Several pack textures are vertical ANIMATION STRIPS —
// water_still.png is 16x512, i.e. 32 stacked 16x16 frames — and sampling them
// whole squashes all 32 frames onto every face. Setting uvRect to one frame's
// slice picks a single frame, and advancing the offset over time plays the
// animation. See flushWaterPass() in src/world.h.
uniform vec4 uvRect;

// =====================================================
// Wind sway
// =====================================================
// How far this draw's vertices may be pushed sideways by wind, in world units.
// Zero for everything by default; the caller raises it around foliage.
//
// This has to happen on the GPU rather than in the geometry: tree meshes are
// baked to static VBOs and drawn with one glDrawArrays each (see initTreeMeshes
// in world.h), so animating them CPU-side would mean re-uploading every tree
// every frame and would throw away the whole point of baking them.
uniform float swayAmount;
uniform float time;

out vec3 FragPos;
out vec3 Normal;
out vec3 vertexColor;
out vec2 TexCoord;
out vec3 gouraudColor;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    vertexColor = aColor;
    TexCoord = aTexCoord * uvRect.xy + uvRect.zw;

    if (swayAmount > 0.0) {
        // Which vertices are allowed to move.
        //
        // For a baked tree (colorMode == 1) aColor.r is the wood->leaf blend
        // factor, so it is already exactly "how leafy is this vertex" — free, and
        // it means trunks stay put while the canopy moves. Multiplying by height
        // above the model origin keeps the base planted, so the tree bends rather
        // than sliding sideways as a rigid object.
        float mask = 1.0;
        if (colorMode == 1) mask = aColor.r * clamp(aPos.y / 4.0, 0.0, 1.0);

        // The position terms are the important part. A phase of time alone would
        // move every tree in view identically at the same instant, which reads as
        // the camera shaking rather than as wind. Offsetting the phase by world
        // position makes the gust travel across the landscape.
        float phase = time * 1.25 + FragPos.x * 0.45 + FragPos.z * 0.35;
        FragPos.xz += vec2(sin(phase), cos(phase * 0.8)) * swayAmount * mask;
    }

    gl_Position = projection * view * vec4(FragPos, 1.0);

    // Gouraud shading: compute lighting per-vertex
    if (useGouraud && lightOn) {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        // Ambient and direct kept apart so the AO hint applies only to direct
        // light, matching the Phong path in fragmentShader.glsl. The H key is
        // meant to demonstrate per-vertex versus per-fragment evaluation, so any
        // *other* difference between the two paths is a bug in the comparison.
        vec3 ambient = vec3(0.0);
        vec3 direct  = vec3(0.0);
        vec3 objColor = (colorMode == 1)
                      ? mix(woodColor, leafColor, aColor.r) * aColor.g
                      : objectColor;

        // Ambient
        vec3 ambientColor = mix(vec3(0.05, 0.05, 0.1), vec3(0.15, 0.15, 0.2), dayFactor);
        if (ambientOn) ambient += ambientColor * objColor;

        // Directional light (sun)
        if (dirLightOn) {
            vec3 ld = normalize(-dirLightDir);
            float diff = max(dot(norm, ld), 0.0);
            vec3 ref = reflect(-ld, norm);
            float spec = pow(max(dot(viewDir, ref), 0.0), specPower);
            if (diffuseOn) direct += diff * dirLightColor * objColor;
            if (specularOn) direct += spec * dirLightColor * 0.5 * specStrength;
        }

        // Directional light (moon) — diffuse only, fade already baked into the colour
        if (dirLightOn && diffuseOn) {
            float mdiff = max(dot(norm, normalize(-moonLightDir)), 0.0);
            direct += moonLightColor * mdiff * objColor;
        }

        // Point lights
        if (pointLightOn) {
            for (int i = 0; i < numPointLights; i++) {
                vec3 ldir = pointLightPos[i] - FragPos;
                float dist = length(ldir);
                ldir = normalize(ldir);
                float atten = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
                float diff = max(dot(norm, ldir), 0.0);
                vec3 ref = reflect(-ldir, norm);
                float spec = pow(max(dot(viewDir, ref), 0.0), specPower);
                if (diffuseOn) direct += diff * pointLightColor[i] * objColor * atten;
                if (specularOn) direct += spec * pointLightColor[i] * 0.3 * atten * specStrength;
            }
        }

        // Spot light
        if (spotLightOn) {
            vec3 slDir = normalize(spotLightPos - FragPos);
            float theta = dot(slDir, normalize(-spotLightDir));
            if (theta > spotCutoff) {
                float diff = max(dot(norm, slDir), 0.0);
                float dist = length(spotLightPos - FragPos);
                float atten = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
                if (diffuseOn) direct += diff * spotLightColor * objColor * atten;
            }
        }

        // AO hint on direct light, then the moon rim — same order and constants
        // as the Phong path.
        float aoHint = 0.72 + 0.28 * clamp(norm.y, 0.0, 1.0);
        vec3 result = ambient + direct * aoHint;
        float rim = pow(1.0 - max(dot(norm, viewDir), 0.0), 3.0);
        result += moonLightColor * rim * 0.6;

        gouraudColor = result;
    } else {
        gouraudColor = vec3(0.0);
    }
}

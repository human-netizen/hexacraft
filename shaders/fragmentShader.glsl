#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 vertexColor;
in vec2 TexCoord;
in vec3 gouraudColor;

uniform vec3 viewPos;
uniform float time;
uniform vec3 objectColor;

// Master toggle
uniform bool lightOn;

// Light type toggles
uniform bool dirLightOn;
uniform bool pointLightOn;
uniform bool spotLightOn;

// Component toggles
uniform bool ambientOn;
uniform bool diffuseOn;
uniform bool specularOn;

// Emissive
uniform bool isEmissive;
uniform vec3 emissiveColor;

// HUD Rendering
uniform bool isHUD;

// Directional light (the sun)
uniform vec3 dirLightDir;
uniform vec3 dirLightColor;

// Second directional light (the moon).
// The sun used to do double duty: at night dirLightColor was simply re-tinted to
// a dim blue. That forces one compromise colour to serve both, so the moon could
// never be cooler than the sun was warm. Two lights let each keep its own colour
// and direction; main.cpp bakes the day/night fade into moonLightColor itself, so
// nothing here needs to scale by dayFactor a second time.
uniform vec3 moonLightDir;
uniform vec3 moonLightColor;

// Specular response of the surface being drawn.
// Every surface used to share pow(..., 32.0) with a fixed strength, so dirt and
// grass carried the same tight highlight as polished metal and a bright spot
// slid over the terrain as the camera moved. Set per block family in
// bindBlockTexture(); the defaults reproduce the old behaviour for any draw that
// does not bother.
uniform float specPower;
uniform float specStrength;

// Point lights
#define MAX_POINT_LIGHTS 8
uniform vec3 pointLightPos[MAX_POINT_LIGHTS];
uniform vec3 pointLightColor[MAX_POINT_LIGHTS];
uniform int numPointLights;

// Spot light
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform vec3 spotLightColor;
uniform float spotCutoff;

// Day factor (0=night, 1=day)
uniform float dayFactor;

// Alpha for transparency (water)
uniform float alpha;

// Fog
uniform vec3 fogColor;
uniform float fogDensity;
// Distance at which fog starts. Plain exp(-density*dist^2) fog reaching full
// opacity by the render cutoff also hazes over the near field; holding it off
// until fogStart keeps the near field crisp and confines the fade to the band
// just before terrain stops being drawn. Driven from RENDER_DIST in main.cpp.
uniform float fogStart;

float fogAmount(float dist) {
    float d = max(dist - fogStart, 0.0);
    return clamp(exp(-fogDensity * d * d), 0.0, 1.0);
}

// =====================================================
// Procedural noise toolkit
// =====================================================
// Standard sin-fract value hash. Not a good PRNG in any statistical sense, but
// it is cheap, deterministic and continuous enough to interpolate, which is all
// a value-noise lattice needs.
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Value noise: hash the four lattice corners and interpolate with a smoothstep
// curve. The f*f*(3-2f) is what stops the lattice showing up as visible squares.
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i),             hash(i + vec2(1, 0)), f.x),
               mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
}

// Fractal Brownian motion — four octaves of value noise at rising frequency and
// falling amplitude. The 2.13 frequency step is deliberately not exactly 2.0:
// an integer ratio lines the octaves' lattices up and the sum shows a grid.
float fBm(vec2 p) {
    float v = 0.0, amp = 0.5, freq = 1.0;
    for (int i = 0; i < 4; i++) {
        v    += amp * vnoise(p * freq);
        freq *= 2.13;
        amp  *= 0.48;
    }
    return v;
}

// Break up the flat repetition of a tiled 16x16 texture across large areas of
// one block type. Sampled in world space, so the variation belongs to the
// terrain rather than to each block, and neighbouring blocks agree at their
// shared edge instead of each restarting the pattern.
uniform bool proceduralTint;

// Texture
uniform sampler2D texture1;
// 0=no texture, 1=simple (texture only), 2=blended (texture * color),
// 3=triplanar (project on all three axes, blend by normal — for geometry whose
//   UVs stretch, i.e. anything that is not an axis-aligned box)
uniform int textureMode;

// Project the texture down all three axes in world space and blend the three
// samples by how much the surface faces each one. Costs three texture fetches
// instead of one, and needs no UVs at all, which is the point: it is for meshes
// whose UV layout stretches (the ambulance body, spheres, cones).
vec3 triplanarSample(vec3 p, vec3 n, float scale) {
    vec3 blend = abs(n);
    // Sharpen the blend so the three projections do not smear into each other
    // across a whole 90-degree turn; without this a cube face picks up a visible
    // ghost of the other two projections near its edges.
    blend = pow(blend, vec3(4.0));
    blend /= max(blend.x + blend.y + blend.z, 0.00001);
    return texture(texture1, p.yz * scale).rgb * blend.x
         + texture(texture1, p.xz * scale).rgb * blend.y
         + texture(texture1, p.xy * scale).rgb * blend.z;
}

// Base-colour source.
// 0 = objectColor uniform (everything).
// 1 = baked tree mesh: aColor packs (blend, shade) instead of an RGB colour, so
//     one baked mesh can be drawn with any leaf colour. Every colour a tree used
//     to produce was mix(wood, leaf, blend) * shade, so this is exact.
uniform int colorMode;
uniform vec3 woodColor;
uniform vec3 leafColor;

// Gouraud/Phong toggle
uniform bool useGouraud;

// Damage tint (red flash on mob hit)
uniform vec3 colorTint;
uniform float colorTintStrength;

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(Normal);

    // Determine base color: apply texture if needed
    vec3 baseColor = (colorMode == 1)
                   ? mix(woodColor, leafColor, vertexColor.r) * vertexColor.g
                   : objectColor;
    vec3 objColor = baseColor;
    if (textureMode == 1) {
        // Simple texture: texture color only, no surface color mixing
        objColor = texture(texture1, TexCoord).rgb;
    } else if (textureMode == 2) {
        // Blended texture: texture color mixed with surface color
        vec3 texColor = texture(texture1, TexCoord).rgb;
        objColor = texColor * baseColor;
    } else if (textureMode == 3) {
        // Triplanar: no UVs used at all
        objColor = triplanarSample(FragPos, norm, 0.5) * baseColor;
    }

    // Large expanses of one block type tile visibly — a hillside of grass reads
    // as wallpaper because every hex samples the same 16x16 image at the same
    // scale. A low-frequency world-space brightness variation on top breaks that
    // up without needing more texture memory or a second material. +/-10%, which
    // is enough to see as terrain variation and not enough to read as dirt.
    if (proceduralTint) {
        objColor *= 0.90 + 0.20 * fBm(FragPos.xz * 0.28);
    }

    // HUD objects: no lighting, no fog.
    // textureMode 0 : solid — use per-vertex color (face shading baked in)
    // textureMode 1 : sprite — texture only (door item icons, etc.)
    // textureMode 2 : textured iso face — texture * per-vertex tint (light/dark faces)
    if (isHUD) {
        vec3 hudColor;
        if (textureMode == 0)      hudColor = vertexColor;
        else if (textureMode == 1) hudColor = texture(texture1, TexCoord).rgb;
        else                       hudColor = texture(texture1, TexCoord).rgb * vertexColor;
        FragColor = vec4(hudColor, alpha);
        return;
    }

    // Emissive objects always glow
    if (isEmissive) {
        float pulse = 0.8 + 0.2 * sin(time * 3.0);
        vec3 emResult = emissiveColor * pulse;
        float dist = length(viewPos - FragPos);
        float fogFactor = fogAmount(dist);
        emResult = mix(fogColor, emResult, fogFactor);
        FragColor = vec4(emResult, alpha);
        return;
    }

    // If master light is off, show very dim
    if (!lightOn) {
        vec3 dimResult = objColor * 0.05;
        float dist = length(viewPos - FragPos);
        float fogFactor = fogAmount(dist);
        dimResult = mix(fogColor, dimResult, fogFactor);
        FragColor = vec4(dimResult, 1.0);
        return;
    }

    // Gouraud shading: use pre-computed per-vertex color from vertex shader
    if (useGouraud) {
        vec3 result = gouraudColor;
        // Apply texture to Gouraud result
        if (textureMode == 1) {
            result = gouraudColor * texture(texture1, TexCoord).rgb / max(objectColor, vec3(0.01));
        } else if (textureMode == 2) {
            result = gouraudColor * texture(texture1, TexCoord).rgb;
        } else if (textureMode == 3) {
            result = gouraudColor * triplanarSample(FragPos, normalize(Normal), 0.5);
        }
        result = result / (result + vec3(1.0)); // tone mapping
        // Same 10% desaturation as the Phong path, so H does not also change how
        // saturated the world looks.
        float gLum = dot(result, vec3(0.299, 0.587, 0.114));
        result = mix(result, vec3(gLum), 0.10);
        float dist = length(viewPos - FragPos);
        float fogFactor = fogAmount(dist);
        result = mix(fogColor, result, fogFactor);
        result = mix(result, colorTint, colorTintStrength);
        FragColor = vec4(result, alpha);
        return;
    }

    // --- Phong shading (per-fragment, default) ---
    // Ambient and direct light are accumulated separately so the ambient-occlusion
    // hint below can be applied to direct light only. Folding it into the ambient
    // as well would double up with the hemispheric term and undo the 19E-7 fix.
    vec3 ambient = vec3(0.0);
    vec3 direct  = vec3(0.0);
    vec3 viewDir = normalize(viewPos - FragPos);

    // --- Ambient (hemispheric) ---
    // 19E-7: the "black holes in the terrain" were never holes. Ambient used to
    // be one flat constant, so every face the sun cannot reach landed on exactly
    // the same value — and for a hex prism that is always three of the six side
    // faces, because their normals are horizontal and the noon sun is nearly
    // straight down. A one-block step in flat ground therefore showed a couple of
    // uniformly near-black quads, which read as a gap in the surface rather than
    // as a shaded wall. Measured: 41/255 on those faces against 114/255 on the
    // lit tops, a 0.37 ratio; Minecraft never lets a face fall below ~0.6.
    //
    // Split the ambient into a sky term from above and a weaker bounce term from
    // below, so brightness varies with how much sky a face can actually see.
    if (ambientOn) {
        vec3 ambColor = mix(vec3(0.05, 0.05, 0.1), vec3(0.25, 0.27, 0.3), dayFactor);
        float upness  = norm.y * 0.5 + 0.5;   // 1 = up, 0.5 = vertical, 0 = down
        vec3 skyAmb   = ambColor * 2.0;
        vec3 gndAmb   = ambColor * 1.0;
        ambient += mix(gndAmb, skyAmb, upness) * objColor;
    }

    // --- Directional light (sun) ---
    if (dirLightOn) {
        vec3 ldir = normalize(-dirLightDir);
        if (diffuseOn) {
            float diff = max(dot(norm, ldir), 0.0);
            // Bounce fill from the anti-sun direction. A cheap stand-in for one
            // light bounce: it keeps a fully back-facing surface from collapsing
            // to bare ambient, which is the other half of the 19E-7 fix.
            float fill = max(dot(norm, -ldir), 0.0) * 0.25;
            direct += dirLightColor * (diff + fill) * objColor;
        }
        if (specularOn) {
            vec3 reflDir = reflect(-ldir, norm);
            float spec = pow(max(dot(viewDir, reflDir), 0.0), specPower);
            direct += dirLightColor * spec * 0.4 * specStrength;
        }
    }

    // --- Directional light (moon) ---
    // Diffuse only: a specular highlight from a light this dim lands below the
    // tone-mapping knee and is invisible, so computing one is wasted work.
    // moonLightColor already carries the day/night fade from main.cpp, and is
    // black at noon, so this costs a multiply-add on a day-lit frame.
    if (dirLightOn && diffuseOn) {
        vec3 mdir = normalize(-moonLightDir);
        float mdiff = max(dot(norm, mdir), 0.0);
        direct += moonLightColor * mdiff * objColor;
    }

    // --- Point lights ---
    if (pointLightOn) {
        for (int i = 0; i < numPointLights; i++) {
            vec3 ldir = pointLightPos[i] - FragPos;
            float dist = length(ldir);
            ldir = normalize(ldir);
            float atten = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

            // Flicker for torches
            float flicker = 0.85 + 0.15 * sin(time * 8.0 + float(i) * 2.7);
            atten *= flicker;

            if (diffuseOn) {
                float diff = max(dot(norm, ldir), 0.0);
                direct += pointLightColor[i] * diff * objColor * atten;
            }
            if (specularOn) {
                vec3 reflDir = reflect(-ldir, norm);
                float spec = pow(max(dot(viewDir, reflDir), 0.0), specPower);
                direct += pointLightColor[i] * spec * 0.3 * atten * specStrength;
            }
        }
    }

    // --- Spot light ---
    if (spotLightOn) {
        vec3 ldir = normalize(spotLightPos - FragPos);
        float theta = dot(ldir, normalize(-spotLightDir));
        if (theta > spotCutoff) {
            float dist = length(spotLightPos - FragPos);
            float atten = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
            float intensity = clamp((theta - spotCutoff) / (1.0 - spotCutoff), 0.0, 1.0);

            if (diffuseOn) {
                float diff = max(dot(norm, ldir), 0.0);
                direct += spotLightColor * diff * objColor * atten * intensity;
            }
            if (specularOn) {
                vec3 reflDir = reflect(-ldir, norm);
                float spec = pow(max(dot(viewDir, reflDir), 0.0), specPower);
                direct += spotLightColor * spec * 0.5 * atten * intensity * specStrength;
            }
        }
    }

    // --- Ambient-occlusion hint ---
    // Upward-facing surfaces see more of the world and so pick up more bounced
    // light; downward-facing ones sit in their own shadow. Keying that off the
    // normal alone is a crude approximation of a real AO pass, but it costs one
    // multiply and captures most of what AO is actually for: making creases and
    // undersides read as recessed. Direct light only — see the note above.
    float aoHint = 0.72 + 0.28 * clamp(norm.y, 0.0, 1.0);
    vec3 result = ambient + direct * aoHint;

    // --- Moon rim light ---
    // Silhouettes objects against a night sky by brightening fragments whose
    // normal is near-perpendicular to the view — i.e. the outline. Deliberately
    // not AO-modulated: a rim is an edge effect, and dimming it by the surface's
    // upness would break the outline wherever it wrapped under an object.
    float rim = pow(1.0 - max(dot(norm, viewDir), 0.0), 3.0);
    result += moonLightColor * rim * 0.6;

    // Tone mapping
    result = result / (result + vec3(1.0));

    // Pull slightly toward grey. Fully saturated lighting output reads as
    // cartoonish; 10% off the saturation is enough to seat the colours without
    // draining them. Must come after tone mapping (so it acts on displayed
    // colour, not on unbounded radiance) and before fog (so the fog colour
    // itself is not desaturated a second time).
    float lum = dot(result, vec3(0.299, 0.587, 0.114));
    result = mix(result, vec3(lum), 0.10);

    // Distance fog
    float dist = length(viewPos - FragPos);
    float fogFactor = fogAmount(dist);
    result = mix(fogColor, result, fogFactor);

    // Damage tint (red flash on hit)
    result = mix(result, colorTint, colorTintStrength);

    FragColor = vec4(result, alpha);
}

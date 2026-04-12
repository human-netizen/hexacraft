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

// Directional light
uniform vec3 dirLightDir;
uniform vec3 dirLightColor;

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

// Texture
uniform sampler2D texture1;
uniform int textureMode; // 0=no texture, 1=simple (texture only), 2=blended (texture * color)

// Gouraud/Phong toggle
uniform bool useGouraud;

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(Normal);

    // Determine base color: apply texture if needed
    vec3 objColor = objectColor;
    if (textureMode == 1) {
        // Simple texture: texture color only, no surface color mixing
        objColor = texture(texture1, TexCoord).rgb;
    } else if (textureMode == 2) {
        // Blended texture: texture color mixed with surface color
        vec3 texColor = texture(texture1, TexCoord).rgb;
        objColor = texColor * objectColor;
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
        float fogFactor = clamp(exp(-fogDensity * dist * dist), 0.0, 1.0);
        emResult = mix(fogColor, emResult, fogFactor);
        FragColor = vec4(emResult, alpha);
        return;
    }

    // If master light is off, show very dim
    if (!lightOn) {
        vec3 dimResult = objColor * 0.05;
        float dist = length(viewPos - FragPos);
        float fogFactor = clamp(exp(-fogDensity * dist * dist), 0.0, 1.0);
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
        }
        result = result / (result + vec3(1.0)); // tone mapping
        float dist = length(viewPos - FragPos);
        float fogFactor = clamp(exp(-fogDensity * dist * dist), 0.0, 1.0);
        result = mix(fogColor, result, fogFactor);
        FragColor = vec4(result, alpha);
        return;
    }

    // --- Phong shading (per-fragment, default) ---
    vec3 result = vec3(0.0);

    // --- Ambient ---
    if (ambientOn) {
        vec3 ambColor = mix(vec3(0.05, 0.05, 0.1), vec3(0.25, 0.27, 0.3), dayFactor);
        result += ambColor * objColor;
    }

    // --- Directional light (sun/moon) ---
    if (dirLightOn) {
        vec3 ldir = normalize(-dirLightDir);
        if (diffuseOn) {
            float diff = max(dot(norm, ldir), 0.0);
            result += dirLightColor * diff * objColor;
        }
        if (specularOn) {
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflDir = reflect(-ldir, norm);
            float spec = pow(max(dot(viewDir, reflDir), 0.0), 32.0);
            result += dirLightColor * spec * 0.4;
        }
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
                result += pointLightColor[i] * diff * objColor * atten;
            }
            if (specularOn) {
                vec3 viewDir = normalize(viewPos - FragPos);
                vec3 reflDir = reflect(-ldir, norm);
                float spec = pow(max(dot(viewDir, reflDir), 0.0), 32.0);
                result += pointLightColor[i] * spec * 0.3 * atten;
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
                result += spotLightColor * diff * objColor * atten * intensity;
            }
            if (specularOn) {
                vec3 viewDir = normalize(viewPos - FragPos);
                vec3 reflDir = reflect(-ldir, norm);
                float spec = pow(max(dot(viewDir, reflDir), 0.0), 32.0);
                result += spotLightColor * spec * 0.5 * atten * intensity;
            }
        }
    }

    // Tone mapping
    result = result / (result + vec3(1.0));

    // Distance fog
    float dist = length(viewPos - FragPos);
    float fogFactor = exp(-fogDensity * dist * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    result = mix(fogColor, result, fogFactor);

    FragColor = vec4(result, alpha);
}

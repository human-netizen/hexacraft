#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 vertexColor;

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

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 objColor = objectColor;

    // Emissive objects always glow
    if (isEmissive) {
        float pulse = 0.8 + 0.2 * sin(time * 3.0);
        FragColor = vec4(emissiveColor * pulse, 1.0);
        return;
    }

    // If master light is off, show very dim
    if (!lightOn) {
        FragColor = vec4(objColor * 0.05, 1.0);
        return;
    }

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

    FragColor = vec4(result, 1.0);
}

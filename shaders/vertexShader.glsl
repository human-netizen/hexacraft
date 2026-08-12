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
uniform vec3 pointLightPos[8];
uniform vec3 pointLightColor[8];
uniform int numPointLights;
uniform vec3 spotLightPos, spotLightDir;
uniform vec3 spotLightColor;
uniform float spotCutoff;
uniform vec3 viewPos;
uniform float dayFactor;

out vec3 FragPos;
out vec3 Normal;
out vec3 vertexColor;
out vec2 TexCoord;
out vec3 gouraudColor;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    vertexColor = aColor;
    TexCoord = aTexCoord;

    gl_Position = projection * view * vec4(FragPos, 1.0);

    // Gouraud shading: compute lighting per-vertex
    if (useGouraud && lightOn) {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 result = vec3(0.0);

        // Ambient
        vec3 ambientColor = mix(vec3(0.05, 0.05, 0.1), vec3(0.15, 0.15, 0.2), dayFactor);
        if (ambientOn) result += ambientColor * objectColor;

        // Directional light
        if (dirLightOn) {
            vec3 ld = normalize(-dirLightDir);
            float diff = max(dot(norm, ld), 0.0);
            vec3 ref = reflect(-ld, norm);
            float spec = pow(max(dot(viewDir, ref), 0.0), 32.0);
            if (diffuseOn) result += diff * dirLightColor * objectColor;
            if (specularOn) result += spec * dirLightColor * 0.5;
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
                float spec = pow(max(dot(viewDir, ref), 0.0), 32.0);
                if (diffuseOn) result += diff * pointLightColor[i] * objectColor * atten;
                if (specularOn) result += spec * pointLightColor[i] * 0.3 * atten;
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
                if (diffuseOn) result += diff * spotLightColor * objectColor * atten;
            }
        }

        gouraudColor = result;
    } else {
        gouraudColor = vec3(0.0);
    }
}

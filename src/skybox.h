#pragma once
// =====================================================
// Skybox — cubemap rendered behind everything else
// Uses its own shader (separate from main shaderProgram)
// =====================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

// Loaded from stb_image (implementation in main.cpp)
#ifndef STBI_INCLUDE_STB_IMAGE_H
#include "stb_image.h"
#endif

// =====================================================
// Skybox shader sources (embedded — no extra .glsl files)
// =====================================================
static const char* skyboxVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 TexCoords;
uniform mat4 projection;
uniform mat4 view;           // translation-stripped view
void main() {
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    // Force depth to 1.0 (far plane) so skybox is always behind everything
    gl_Position = pos.xyww;
}
)GLSL";

static const char* skyboxFragSrc = R"GLSL(
#version 330 core
in vec3 TexCoords;
out vec4 FragColor;
uniform samplerCube skybox;
void main() {
    FragColor = texture(skybox, TexCoords);
}
)GLSL";

// =====================================================
// Globals
// =====================================================
static GLuint skyboxShader  = 0;
static GLuint skyboxVAO     = 0;
static GLuint skyboxVBO     = 0;
static GLuint skyboxCubemap = 0;

// =====================================================
// Internal helpers
// =====================================================
static GLuint compileSkyboxShader(const char* vertSrc, const char* fragSrc) {
    auto compileStage = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
            printf("[Skybox] Shader compile error: %s\n", log);
        }
        return s;
    };
    GLuint vs = compileStage(GL_VERTEX_SHADER,   vertSrc);
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(prog, 512, nullptr, log);
        printf("[Skybox] Link error: %s\n", log);
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

// =====================================================
// Load cubemap from 6 face image files
// Order expected by OpenGL:
//   +X right, -X left, +Y top, -Y bottom, +Z front, -Z back
// =====================================================
static GLuint loadCubemap(const char* faces[6]) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    // Cubemap faces must NOT be flipped (unlike regular 2-D textures)
    stbi_set_flip_vertically_on_load(false);

    for (int i = 0; i < 6; i++) {
        int w, h, ch;
        unsigned char* data = stbi_load(faces[i], &w, &h, &ch, 0);
        if (data) {
            GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            printf("[Skybox] Loaded face %d: %s (%dx%d)\n", i, faces[i], w, h);
        } else {
            printf("[Skybox] Failed to load face %d: %s\n", i, faces[i]);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Restore default flip state used by the rest of the engine
    stbi_set_flip_vertically_on_load(true);

    return texID;
}

// =====================================================
// Public API
// =====================================================

// Call once after gladLoadGL
void initSkybox() {
    // --- Shader ---
    skyboxShader = compileSkyboxShader(skyboxVertSrc, skyboxFragSrc);

    // --- Unit cube (36 vertices, CCW winding) ---
    static const float skyboxVerts[] = {
        // +X face (right)
        1,-1,-1,  1,-1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1,  1,-1,-1,
        // -X face (left)
       -1,-1, 1, -1,-1,-1, -1, 1,-1, -1, 1,-1, -1, 1, 1, -1,-1, 1,
        // +Y face (top)
       -1, 1,-1,  1, 1,-1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1, 1,-1,
        // -Y face (bottom)
       -1,-1, 1,  1,-1, 1,  1,-1,-1,  1,-1,-1, -1,-1,-1, -1,-1, 1,
        // +Z face (front)
       -1,-1, 1, -1, 1, 1,  1, 1, 1,  1, 1, 1,  1,-1, 1, -1,-1, 1,
        // -Z face (back)
        1,-1,-1,  1, 1,-1, -1, 1,-1, -1, 1,-1, -1,-1,-1,  1,-1,-1,
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVerts), skyboxVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // --- Cubemap textures ---
    const char* faces[6] = {
        "assets/srcs/right.png",   // +X
        "assets/srcs/left.png",    // -X
        "assets/srcs/top.png",     // +Y
        "assets/srcs/bottom.png",  // -Y
        "assets/srcs/front.png",   // +Z
        "assets/srcs/back.png",    // -Z
    };
    skyboxCubemap = loadCubemap(faces);

    printf("[Skybox] Initialised\n");
}

// Draw the skybox.  Call BEFORE the main scene each frame.
// viewMat  — full camera view matrix (translation will be stripped here)
// projMat  — same projection used for the main scene
void drawSkybox(const glm::mat4& viewMat, const glm::mat4& projMat) {
    if (!skyboxShader || !skyboxVAO || !skyboxCubemap) return;

    // Strip translation so the box stays centred on the camera
    glm::mat4 skyView = glm::mat4(glm::mat3(viewMat));

    glDepthFunc(GL_LEQUAL);   // skybox depth == 1.0 — pass even when equal
    glDepthMask(GL_FALSE);    // don't write to depth buffer

    glUseProgram(skyboxShader);
    setMat4(skyboxShader, "view", skyView);
    setMat4(skyboxShader, "projection", projMat);
    setInt(skyboxShader, "skybox", 0);

    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);     // restore depth writes
    glDepthFunc(GL_LESS);     // restore default depth test
}

// Cleanup — call before program exits
void destroySkybox() {
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteTextures(1, &skyboxCubemap);
    glDeleteProgram(skyboxShader);
}

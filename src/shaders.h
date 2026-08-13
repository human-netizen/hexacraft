#pragma once

// =====================================================
// Shader utilities
// =====================================================
std::string loadFile(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) { printf("Cannot open: %s\n", path); return ""; }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        printf("Shader error: %s\n", log);
    }
    return s;
}

GLuint loadShaders(const char* vp, const char* fp) {
    std::string vs = loadFile(vp), fs = loadFile(fp);
    GLuint v = compileShader(GL_VERTEX_SHADER, vs.c_str());
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs.c_str());
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    int ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, 512, NULL, log);
        printf("Link error: %s\n", log);
    }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

// =====================================================
// Uniform location cache
// glGetUniformLocation() is a driver-side string lookup. The setters below run
// ~215k times per frame, so calling it every time cost more than the draw calls
// themselves. Cache the location the first time each name is seen.
//
// Keyed on the name POINTER (almost every call site passes a string literal, and
// literals have stable addresses), but the stored name is re-checked with an
// equality test, so a reused char buffer can never hand back a stale location —
// it just misses and re-resolves.
// =====================================================
struct UniformCacheEntry {
    GLuint prog;
    std::string name;
    GLint loc;
};
static std::unordered_map<uint64_t, UniformCacheEntry> g_uniformCache;
static long g_uniformCacheHits = 0, g_uniformCacheMisses = 0;

static GLint uniformLoc(GLuint prog, const char* name) {
    // Fold the program into the key: the compiler merges identical string literals
    // across the whole translation unit, so "view" in main.cpp and "view" in
    // skybox.h are the same address despite belonging to different programs.
    uint64_t key = ((uint64_t)prog << 48) ^ (uint64_t)(uintptr_t)name;
    auto it = g_uniformCache.find(key);
    if (it != g_uniformCache.end() && it->second.prog == prog && it->second.name == name) {
        g_uniformCacheHits++;
        return it->second.loc;
    }
    GLint loc = glGetUniformLocation(prog, name);
    g_uniformCache[key] = UniformCacheEntry{prog, std::string(name), loc};
    g_uniformCacheMisses++;
    return loc;
}

void setMat4(GLuint prog, const char* name, const glm::mat4& m) {
    glUniformMatrix4fv(uniformLoc(prog, name), 1, GL_FALSE, glm::value_ptr(m));
}
void setVec3(GLuint prog, const char* name, const glm::vec3& v) {
    glUniform3fv(uniformLoc(prog, name), 1, glm::value_ptr(v));
}
void setBool(GLuint prog, const char* name, bool val) {
    glUniform1i(uniformLoc(prog, name), val ? 1 : 0);
}
void setFloat(GLuint prog, const char* name, float val) {
    glUniform1f(uniformLoc(prog, name), val);
}
void setInt(GLuint prog, const char* name, int val) {
    glUniform1i(uniformLoc(prog, name), val);
}

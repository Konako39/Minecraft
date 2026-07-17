#pragma once

#include <glad/glad.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 读取整个文本文件；失败返回空字符串
inline std::string readTextFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << '\n';
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline void printShaderLog(GLuint shader, const char* label) {
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok) return;
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::cerr << label << " compile error:\n" << log << '\n';
}

inline void printProgramLog(GLuint program) {
    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok) return;
    char log[1024];
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    std::cerr << "Shader link error:\n" << log << '\n';
}

// 读 vert/frag → 编译 → 链接，返回 program；失败返回 0
inline unsigned int loadShaderProgram(const char* vertPath, const char* fragPath) {
    const std::string vertSrc = readTextFile(vertPath);
    const std::string fragSrc = readTextFile(fragPath);
    if (vertSrc.empty() || fragSrc.empty()) return 0;

    const char* vSrc = vertSrc.c_str();
    const char* fSrc = fragSrc.c_str();

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vSrc, nullptr);
    glCompileShader(vert);
    printShaderLog(vert, "VERTEX");

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fSrc, nullptr);
    glCompileShader(frag);
    printShaderLog(frag, "FRAGMENT");

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    printProgramLog(program);

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

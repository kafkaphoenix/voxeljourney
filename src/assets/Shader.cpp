#include "Shader.h"

#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace se::assets {

static std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static void checkShaderCompilation(unsigned int shader, const std::string& type) {
    int success;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        throw std::runtime_error(type + " shader compilation failed: " + infoLog);
    }
}

static void checkProgramLinking(unsigned int program) {
    int success;
    char infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        throw std::runtime_error("Shader program linking failed: " + std::string(infoLog));
    }
}

Shader::Shader(const std::string& shaderPath)
    : Asset(shaderPath), m_Path(shaderPath) {
    std::string vSrc = loadFile(shaderPath + ".vert");
    std::string fSrc = loadFile(shaderPath + ".frag");

    const char* v = vSrc.c_str();
    const char* f = fSrc.c_str();

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &v, nullptr);
    glCompileShader(vs);
    checkShaderCompilation(vs, "VERTEX");

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &f, nullptr);
    glCompileShader(fs);
    checkShaderCompilation(fs, "FRAGMENT");

    m_ID = glCreateProgram();
    glAttachShader(m_ID, vs);
    glAttachShader(m_ID, fs);
    glLinkProgram(m_ID);
    checkProgramLinking(m_ID);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() { glDeleteProgram(m_ID); }

void Shader::bind() const { glUseProgram(m_ID); }
void Shader::unbind() const { glUseProgram(0); }

int Shader::getUniformLocation(const std::string& name) const {
    auto it = m_UniformLocations.find(name);
    if (it != m_UniformLocations.end()) {
        return it->second;
    }

    int loc = glGetUniformLocation(m_ID, name.c_str());
    m_UniformLocations.emplace(name, loc);
    return loc;
}

void Shader::setMat4(const std::string& name, const float* value) const {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, value);
    }
}

void Shader::setVec4(const std::string& name, const float* value) const {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glUniform4fv(loc, 1, value);
    }
}

void Shader::setVec3(const std::string& name, const float* value) const {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glUniform3fv(loc, 1, value);
    }
}

void Shader::setInt(const std::string& name, int value) const {
    int loc = getUniformLocation(name);
    if (loc != -1) glUniform1i(loc, value);
}

void Shader::setFloat(const std::string& name, float value) const {
    int loc = getUniformLocation(name);
    if (loc != -1) glUniform1f(loc, value);
}

void Shader::setBool(const std::string& name, bool value) const {
    int loc = getUniformLocation(name);
    if (loc != -1) glUniform1i(loc, value ? 1 : 0);
}

void Shader::bindUniformBlock(const std::string& name, unsigned int binding) const {
    auto it = m_BlockIndices.find(name);
    unsigned int index = 0;
    if (it != m_BlockIndices.end()) {
        index = it->second;
    } else {
        index = glGetUniformBlockIndex(m_ID, name.c_str());
        m_BlockIndices.emplace(name, index);
    }

    if (index == GL_INVALID_INDEX) {
        throw std::runtime_error("Uniform block not found: " + name);
    }

    glUniformBlockBinding(m_ID, index, binding);
}

}  // namespace se::assets

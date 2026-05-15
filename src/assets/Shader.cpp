#include "Shader.h"

#include <glad/glad.h>

#include <array>
#include <format>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "render/UboDefinitions.h"

namespace se::assets {

std::string Shader::loadFile(std::string_view path) {
    // open file in binary mode and move the file pointer to the end of the file to get the file size
    std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open shader file: {}", path));
    }

    // get file size
    const std::streamsize size = file.tellg();
    // reset file pointer to the beginning of the file
    file.seekg(0);

    // read file contents into a string with exactly the size of the file
    std::string buffer(size, '\0');
    file.read(buffer.data(), size);
    return buffer;
}

void Shader::checkShaderCompilation(unsigned int shader, std::string_view type) const {
    int success = 0;
    std::array<char, 1024> infoLog{};
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog.data());
        std::string name = type == "VERTEX" ? m_VertPath : m_FragPath;
        throw std::runtime_error(std::format("{} shader {} compilation failed: {}", type, name, infoLog.data()));
    }
}

void Shader::checkProgramLinking() const {
    int success = 0;
    std::array<char, 1024> infoLog{};
    glGetProgramiv(m_Id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_Id, 1024, nullptr, infoLog.data());
        throw std::runtime_error(std::format("Shader {} program linking failed: {}", m_Name, infoLog.data()));
    }
}

Shader::Shader(std::string_view directory) : Shader(directory, directory) {}

Shader::Shader(std::string_view vertPath, std::string_view fragPath)
    : Asset(std::format("{}|{}", vertPath, fragPath)),
      m_VertPath(vertPath),
      m_FragPath(fragPath),
      m_Id(glCreateProgram()) {
    std::string vSrc = loadFile(std::format("{}.vert", m_VertPath));
    std::string fSrc = loadFile(std::format("{}.frag", m_FragPath));

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

    glAttachShader(m_Id, vs);
    glAttachShader(m_Id, fs);
    glLinkProgram(m_Id);
    checkProgramLinking();

    std::string progLabel = std::format("Shader Program [{}]", m_Name);
    glObjectLabel(GL_PROGRAM, m_Id, static_cast<GLsizei>(progLabel.size()), progLabel.c_str());

    glDeleteShader(vs);
    glDeleteShader(fs);

    for (const auto& [name, binding] : se::render::UBO_BINDINGS) {
        unsigned int index = glGetUniformBlockIndex(m_Id, name.data());
        if (index != GL_INVALID_INDEX) {  // if the shader doesn't have this block, we just skip it instead of treating
                                          // it as an error
            glUniformBlockBinding(m_Id, index, std::to_underlying(binding));
        }
    }
}

Shader::~Shader() { glDeleteProgram(m_Id); }

void Shader::validateLayout(const se::render::BufferLayout& layout) const {
#ifndef NDEBUG
    GLint activeAttribs = 0;
    glGetProgramInterfaceiv(m_Id, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &activeAttribs);

    // Collect only the vertex attribs (skip builtins and instance slots)
    std::unordered_map<std::string, GLint> shaderVertexAttribs;
    for (GLint i = 0; i < activeAttribs; ++i) {
        std::array<char, 256> nameBuf{};
        GLsizei nameLen = 0;
        glGetProgramResourceName(m_Id, GL_PROGRAM_INPUT, i, static_cast<GLsizei>(nameBuf.size()), &nameLen,
                                 nameBuf.data());

        const GLenum prop = GL_LOCATION;
        GLint location = -1;
        glGetProgramResourceiv(m_Id, GL_PROGRAM_INPUT, i, 1, &prop, 1, nullptr, &location);

        if (location < 0) {
            continue;  // skip built-in attributes which don't have a location
        }

        // Skip built-in attributes (e.g. gl_VertexID) which don't have a location and aren't part of the layout.
        std::string name(nameBuf.data(), nameLen);
        if (name.starts_with("gl_")) {
            continue;
        }

        // Skip instance attribs, they're owned by Mesh::setupInstanceAttributes
        auto instanceAttribBase = static_cast<GLuint>(layout.getElements().size());
        if (instanceAttribBase > 0 && location >= static_cast<GLint>(instanceAttribBase)) {
            continue;
        }

        shaderVertexAttribs[name] = location;
    }

    const auto& elements = layout.getElements();

    if (shaderVertexAttribs.size() != elements.size()) {
        throw std::runtime_error(std::format("Shader '{}': layout has {} vertex attribs but shader expects {}", m_Name,
                                             elements.size(), shaderVertexAttribs.size()));
    }

    GLint expectedLocation = 0;
    for (const auto& element : elements) {
        auto it = shaderVertexAttribs.find(element.name);
        if (it == shaderVertexAttribs.end()) {
            throw std::runtime_error(
                std::format("Shader '{}': layout element '{}' not found in shader", m_Name, element.name));
        }
        if (it->second != expectedLocation) {
            throw std::runtime_error(
                std::format("Shader '{}': layout element '{}' expected at location {} but shader has it at {}", m_Name,
                            element.name, expectedLocation, it->second));
        }
        ++expectedLocation;
    }
#endif
}

int Shader::getUniformLocation(std::string_view name) {
    auto it = m_UniformLocations.find(name);
    if (it != m_UniformLocations.end()) {
        return it->second;
    }

    // we save the uniform location in the map even if it's -1 (not found) to avoid redundant
    // glGetUniformLocation calls for the same name in the future.
    std::string nameStr(name);
    int loc = glGetUniformLocation(m_Id, nameStr.c_str());
    m_UniformLocations.emplace(std::move(nameStr), loc);
    return loc;
}

void Shader::setInt(std::string_view name, int value) {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glProgramUniform1i(m_Id, loc, value);
    }
}

void Shader::setFloat(std::string_view name, float value) {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glProgramUniform1f(m_Id, loc, value);
    }
}

void Shader::setBool(std::string_view name, bool value) {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glProgramUniform1i(m_Id, loc, value ? 1 : 0);
    }
}

void Shader::setVec2(std::string_view name, glm::vec2 value) {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glProgramUniform2fv(m_Id, loc, 1, glm::value_ptr(value));
    }
}

void Shader::setVec3(std::string_view name, glm::vec3 value) {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glProgramUniform3fv(m_Id, loc, 1, glm::value_ptr(value));
    }
}

void Shader::setVec4(std::string_view name, glm::vec4 value) {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glProgramUniform4fv(m_Id, loc, 1, glm::value_ptr(value));
    }
}

void Shader::setMat3(std::string_view name, glm::mat3 value) {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glProgramUniformMatrix3fv(m_Id, loc, 1, GL_FALSE, glm::value_ptr(value));
    }
}

void Shader::setMat4(std::string_view name, glm::mat4 value) {
    int loc = getUniformLocation(name);
    if (loc != -1) {
        glProgramUniformMatrix4fv(m_Id, loc, 1, GL_FALSE, glm::value_ptr(value));
    }
}

}  // namespace se::assets
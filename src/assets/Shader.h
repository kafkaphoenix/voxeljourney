#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

#include "Asset.h"
#include "StringHash.h"
#include "render/BufferLayout.h"

namespace se::assets {

class Shader : public Asset {
public:
    explicit Shader(std::string_view directory);
    explicit Shader(std::string_view vertPath, std::string_view fragPath);
    ~Shader() override;

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    void bind() const { glUseProgram(m_Id); }
    static void unbind() { glUseProgram(0); }

    [[nodiscard]] unsigned int id() const { return m_Id; }

    void validateLayout(const se::render::BufferLayout& layout) const;

    void setInt(std::string_view name, int value);
    void setFloat(std::string_view name, float value);
    void setBool(std::string_view name, bool value);
    void setVec2(std::string_view name, glm::vec2 value);
    void setVec3(std::string_view name, glm::vec3 value);
    void setVec4(std::string_view name, glm::vec4 value);
    void setMat3(std::string_view name, glm::mat3 value);
    void setMat4(std::string_view name, glm::mat4 value);

private:
    // Returns cached location, or queries and caches it on first call.
    int getUniformLocation(std::string_view name);
    static std::string loadFile(std::string_view path);
    void checkShaderCompilation(unsigned int shader, std::string_view type) const;
    void checkProgramLinking() const;

    unsigned int m_Id = 0;
    std::string m_VertPath;
    std::string m_FragPath;
    std::unordered_map<std::string, int, StringHash, std::equal_to<>> m_UniformLocations;
};

}  // namespace se::assets
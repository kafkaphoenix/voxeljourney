#pragma once
#include <glad/glad.h>

#include <string>
#include <string_view>
#include <unordered_map>

#include "Asset.h"
#include "StringHash.h"
#include "render/BufferLayout.h"

namespace se::assets {

class Shader : public Asset {
public:
    explicit Shader(std::string path);
    ~Shader() override;

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    void bind() const { glUseProgram(m_Id); }
    static void unbind() { glUseProgram(0); }

    [[nodiscard]] unsigned int id() const { return m_Id; }

    void validateLayout(const se::render::BufferLayout& layout, GLuint instanceAttribBase) const;

    void setMat4(std::string_view name, const float* value);
    void setVec4(std::string_view name, const float* value);
    void setVec3(std::string_view name, const float* value);
    void setInt(std::string_view name, int value);
    void setFloat(std::string_view name, float value);
    void setBool(std::string_view name, bool value);

private:
    // Returns cached location, or queries and caches it on first call.
    int getUniformLocation(std::string_view name);

    unsigned int m_Id = 0;
    std::unordered_map<std::string, int, StringHash, std::equal_to<>> m_UniformLocations;
};

}  // namespace se::assets
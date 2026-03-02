#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "Asset.h"
#include "StringHash.h"

namespace se::assets {

class Shader : public Asset {
   public:
    explicit Shader(std::string shaderPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    void bind() const;
    void unbind() const;

    void setMat4(std::string_view name, const float* value) const;
    void setVec4(std::string_view name, const float* value) const;
    void setVec3(std::string_view name, const float* value) const;
    void setInt(std::string_view name, int value) const;
    void setFloat(std::string_view name, float value) const;
    void setBool(std::string_view name, bool value) const;
    void bindUniformBlock(std::string_view name, unsigned int binding) const;

    std::string_view getPath() const override { return m_Path; }

   private:
    int getUniformLocation(std::string_view name) const;

    unsigned int m_ID;
    std::string m_Path;
    mutable std::unordered_map<std::string, int, TransparentStringHash, std::equal_to<>> m_UniformLocations;
    mutable std::unordered_map<std::string, unsigned int, TransparentStringHash, std::equal_to<>> m_BlockIndices;
};

}  // namespace se::assets

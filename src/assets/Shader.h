#pragma once
#include <string>
#include <unordered_map>

#include "Asset.h"

namespace se::assets {

class Shader : public Asset {
   public:
    Shader(const std::string& shaderPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    void bind() const;
    void unbind() const;

    void setMat4(const std::string& name, const float* value) const;
    void setVec4(const std::string& name, const float* value) const;
    void setVec3(const std::string& name, const float* value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setBool(const std::string& name, bool value) const;
    void bindUniformBlock(const std::string& name, unsigned int binding) const;

    const std::string& getPath() const override { return m_Path; }

   private:
    int getUniformLocation(const std::string& name) const;

    unsigned int m_ID;
    std::string m_Path;
    mutable std::unordered_map<std::string, int> m_UniformLocations;
    mutable std::unordered_map<std::string, unsigned int> m_BlockIndices;
};

}  // namespace se::assets

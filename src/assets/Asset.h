#pragma once
#include <string>

namespace se::assets {

class Asset {
   public:
    virtual ~Asset() = default;

    virtual const std::string& getPath() const = 0;

   protected:
    Asset(const std::string& path) : m_Path(path) {}

    std::string m_Path;
};

}  // namespace se::assets
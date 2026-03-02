#pragma once
#include <string>
#include <string_view>

namespace se::assets {

class Asset {
   public:
    virtual ~Asset() = default;

    virtual std::string_view getPath() const = 0;

   protected:
    // explicit constructor to prevent accidental implicit conversions
    explicit Asset(std::string path) : m_Path(std::move(path)) {}

    std::string m_Path;
};

}  // namespace se::assets
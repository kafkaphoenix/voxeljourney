#pragma once
#include <string>
#include <string_view>

namespace se::assets {

class Asset {
public:
    virtual ~Asset() = default;

    [[nodiscard]] std::string_view getName() const { return m_Name; }

protected:
    explicit Asset(std::string name) : m_Name(std::move(name)) {}

    std::string m_Name;
};

}  // namespace se::assets
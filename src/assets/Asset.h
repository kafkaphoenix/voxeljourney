#pragma once
#include <string>
#include <string_view>

namespace se::assets {

class Asset {
public:
    virtual ~Asset() = default;

    [[nodiscard]] std::string_view getPath() const { return m_Path; }

protected:
    explicit Asset(std::string path) : m_Path(std::move(path)) {}

    std::string m_Path;
};

}  // namespace se::assets
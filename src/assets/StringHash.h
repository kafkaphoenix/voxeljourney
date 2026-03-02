#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace se::assets {

// TransparentStringHash allows us to use std::string_view as keys in unordered_maps without needing
// to construct std::string objects, which can save memory and improve performance when looking up
// assets by path https://www.cppstories.com/2021/heterogeneous-access-cpp20/
struct TransparentStringHash {
    using is_transparent = void;

    size_t operator()(std::string_view value) const noexcept {
        return std::hash<std::string_view>{}(value);
    }
};

}  // namespace se::assets

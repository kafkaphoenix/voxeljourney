#pragma once

#include <glm/mat4x4.hpp>

#include "assets/Pose.h"

namespace se::render {

class Mesh;
}

namespace se::assets {
class Material;
}

namespace se::render {

struct ModelSubmission {
    Mesh* mesh = nullptr;
    const se::assets::Material* material = nullptr;
    glm::mat4 modelMatrix{1.0f};
    const se::assets::BonePalette* bones = nullptr;

    [[nodiscard]] bool isAnimated() const noexcept { return bones != nullptr; }
};

}  // namespace se::render
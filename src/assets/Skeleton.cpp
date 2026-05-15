#include "Skeleton.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace se::assets {

void Skeleton::buildPalette(const Pose& pose, BonePalette& outPalette) const {
    std::array<glm::mat4, MAX_BONES> localMatrices{};
    std::array<glm::mat4, MAX_BONES> globalMatrices{};

    const int count = (std::min)(static_cast<int>(bones.size()), MAX_BONES);
    for (int i = 0; i < count; ++i) {
        const auto& p = pose.at(i);
        const glm::mat4 t = glm::translate(glm::mat4{1.0f}, p.translation);
        const glm::mat4 r = glm::toMat4(p.rotation);
        const glm::mat4 s = glm::scale(glm::mat4{1.0f}, p.scale);
        localMatrices.at(i) = t * r * s;
    }

    for (int i = 0; i < count; ++i) {
        if (bones.at(i).parent >= 0) {
            globalMatrices.at(i) = globalMatrices.at(bones.at(i).parent) * localMatrices.at(i);
        } else {
            globalMatrices.at(i) = localMatrices.at(i);
        }
    }

    for (int i = 0; i < count; ++i) { outPalette.at(i) = globalMatrices.at(i) * bones.at(i).inverseBindMatrix; }

    for (int i = count; i < MAX_BONES; ++i) { outPalette.at(i) = glm::mat4{1.0f}; }
}

}  // namespace se::assets

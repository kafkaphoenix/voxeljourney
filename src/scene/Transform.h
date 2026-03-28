#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace se::scene {

struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1, 0, 0, 0};
    glm::vec3 scale{1.0f};

    [[nodiscard]] glm::mat4 getMatrix() const {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 r = glm::mat4_cast(rotation);
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        return t * r * s;
    }
};

}  // namespace se::scene

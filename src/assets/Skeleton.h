#pragma once
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace se::assets {

// A single bone in the skeleton hierarchy.
struct Bone {
    std::string name;
    int parent = -1;  // -1 = root
    glm::mat4 inverseBindMatrix{1.0f};

    // Rest pose TRS (decomposed from glTF node transform).
    // Used as defaults when only some channels are animated.
    glm::vec3 restPosition{0.0f};
    glm::quat restRotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 restScale{1.0f};
};

// Skeleton extracted from a glTF skin.
// Bones are stored in flat array; parent indices point into the same array.
struct Skeleton {
    std::vector<Bone> bones;

    // Maps glTF node index to bone index in this skeleton, or -1 if the node isn't a joint.
    // Used to match animation channels to bones.
    std::vector<int> nodeToJoint;
};

inline constexpr int MAX_BONES = 128;

}  // namespace se::assets

#pragma once

#include <string>
#include <utility>

#include "Animator.h"
#include "Transform.h"
#include "assets/AssetHandle.h"

namespace se::scene {

struct AnimatedInstance {
    se::assets::ModelHandle model;
    Transform transform;
    Animator animator;
    std::string tag;

    AnimatedInstance(se::assets::ModelHandle handle, Transform instanceTransform, std::string instanceTag)
        : model(handle), transform(instanceTransform), animator(model), tag(std::move(instanceTag)) {}
};

}  // namespace se::scene

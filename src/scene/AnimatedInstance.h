#pragma once

#include <string>

#include "Animator.h"
#include "Transform.h"
#include "assets/AssetHandle.h"

namespace se::scene {

struct AnimatedInstance {
    se::assets::ModelHandle model;
    Transform transform;
    Animator animator;
    std::string tag;

    AnimatedInstance(se::assets::ModelHandle modelHandle, Transform instanceTransform, std::string instanceTag)
        : model(std::move(modelHandle)),
          transform(std::move(instanceTransform)),
          animator(model),
          tag(std::move(instanceTag)) {}
};

}  // namespace se::scene

#pragma once

#include <string>
#include <utility>

#include "AnimationController.h"
#include "Animator.h"
#include "Transform.h"
#include "assets/AssetHandle.h"

namespace se::scene {

struct AnimatedInstance {
    se::assets::ModelHandle model;
    Transform transform;
    Animator animator;
    AnimationController controller;
    std::string tag;

    AnimatedInstance(se::assets::ModelHandle handle, Transform instanceTransform, std::string instanceTag,
                     AnimationController::LocomotionClips clips = {})
        : model(handle), transform(instanceTransform), animator(model), tag(std::move(instanceTag)) {
        controller.setLocomotionClips(std::move(clips));
    }
};

}  // namespace se::scene

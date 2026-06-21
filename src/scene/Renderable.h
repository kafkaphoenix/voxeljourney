#pragma once

#include <cassert>
#include <variant>

#include "Transform.h"
#include "assets/Material.h"
#include "render/VisibilityMask.h"

namespace se::render {
class Mesh;
}

namespace se::scene {
class Animator;

struct StaticPose {
    Transform transform;
};

struct DynamicPose {
    const Transform* transform = nullptr;
    const Animator* animator = nullptr;
};

struct Renderable {
    se::render::Mesh* mesh = nullptr;
    se::assets::MaterialHandle material;
    std::variant<StaticPose, DynamicPose> poseSource = StaticPose{};
    se::render::visibility::Layer visibilityMask = se::render::visibility::Layer::World;

    [[nodiscard]] static Renderable makeStatic(se::render::Mesh* mesh, se::assets::MaterialHandle handle,
                                               Transform transform) {
        return Renderable{
            .mesh = mesh,
            .material = handle,
            .poseSource = StaticPose{transform},
            .visibilityMask = se::render::visibility::Layer::World,
        };
    }

    [[nodiscard]] static Renderable makeAnimated(
        se::render::Mesh* mesh, se::assets::MaterialHandle handle, const Transform& transform, const Animator& animator,
        se::render::visibility::Layer visibilityMask = se::render::visibility::Layer::World) {
        return Renderable{
            .mesh = mesh,
            .material = handle,
            .poseSource = DynamicPose{&transform, &animator},
            .visibilityMask = visibilityMask,
        };
    }

    [[nodiscard]] const Transform& resolvedTransform() const {
        if (const auto* const staticPose = std::get_if<StaticPose>(&poseSource)) {
            return staticPose->transform;
        }

        const auto* dynamicPose = std::get_if<DynamicPose>(&poseSource);
        assert(dynamicPose && dynamicPose->transform && "Dynamic renderable must reference a valid transform");
        return *dynamicPose->transform;
    }

    [[nodiscard]] const Animator* resolvedAnimator() const {
        if (const auto* const dynamicPose = std::get_if<DynamicPose>(&poseSource)) {
            return dynamicPose->animator;
        }
        return nullptr;
    }
};

}  // namespace se::scene

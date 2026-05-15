#pragma once

#include <cassert>
#include <variant>

#include "Transform.h"
#include "assets/Material.h"

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

    [[nodiscard]] static Renderable makeStatic(se::render::Mesh* mesh, se::assets::MaterialHandle material,
                                               Transform transform) {
        return Renderable{
            .mesh = mesh,
            .material = std::move(material),
            .poseSource = StaticPose{std::move(transform)},
        };
    }

    [[nodiscard]] static Renderable makeAnimated(se::render::Mesh* mesh, se::assets::MaterialHandle material,
                                                 const Transform& transform, const Animator& animator) {
        return Renderable{
            .mesh = mesh,
            .material = std::move(material),
            .poseSource = DynamicPose{&transform, &animator},
        };
    }

    [[nodiscard]] const Transform& resolvedTransform() const {
        if (const auto staticPose = std::get_if<StaticPose>(&poseSource)) {
            return staticPose->transform;
        }

        const auto* dynamicPose = std::get_if<DynamicPose>(&poseSource);
        assert(dynamicPose && dynamicPose->transform && "Dynamic renderable must reference a valid transform");
        return *dynamicPose->transform;
    }

    [[nodiscard]] const Animator* resolvedAnimator() const {
        if (const auto dynamicPose = std::get_if<DynamicPose>(&poseSource)) {
            return dynamicPose->animator;
        }
        return nullptr;
    }
};

}  // namespace se::scene

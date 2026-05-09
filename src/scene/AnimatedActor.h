#pragma once

#include "AnimatedRenderable.h"
#include "Animator.h"
#include "Transform.h"
#include "assets/AssetHandle.h"
#include "assets/Model.h"

namespace se::scene {

class AnimatedActor {
public:
    AnimatedActor(se::assets::ModelHandle model, Transform transform, std::string tag);

    void update(float deltaTime);

    [[nodiscard]] std::string_view getTag() const { return m_Tag; }
    [[nodiscard]] Transform& getTransform() { return m_Transform; }
    [[nodiscard]] const Transform& getTransform() const { return m_Transform; }
    [[nodiscard]] std::vector<AnimatedRenderable> collectRenderables() const;

    void playClip(std::string_view clipName, float blendDuration = 0.15f);

private:
    se::assets::ModelHandle m_Model;
    Transform m_Transform;
    std::optional<Animator> m_Animator;
    std::string m_Tag;
};

}  // namespace se::scene

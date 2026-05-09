#include "AnimatedActor.h"

namespace se::scene {

AnimatedActor::AnimatedActor(se::assets::ModelHandle model, Transform transform, std::string tag)
    : m_Model(model), m_Transform(transform), m_Tag(std::move(tag)) {
    auto ptr = m_Model.get();
    if (ptr && ptr->isAnimated() && !ptr->getAnimations().empty()) {
        m_Animator.emplace(m_Model, 0);
    }
}

std::vector<AnimatedRenderable> AnimatedActor::collectRenderables() const {
    auto ptr = m_Model.get();
    if (!ptr || !m_Animator)
        return {};
    std::vector<AnimatedRenderable> out;
    for (const auto& sub : ptr->getSubMeshes()) {
        if (!sub.mesh)
            continue;
        out.push_back({.renderable = {.mesh = sub.mesh.get(), .material = sub.material, .transform = m_Transform},
                       .boneMatrices = m_Animator->boneMatrices()});
    }
    return out;
}

void AnimatedActor::playClip(std::string_view clipName, float blendDuration) {
    auto ptr = m_Model.get();
    if (!ptr || !m_Animator)
        return;
    const auto& anims = ptr->getAnimations();
    for (int i = 0; i < static_cast<int>(anims.size()); ++i) {
        if (anims[i].name == clipName) {
            m_Animator->setClipFade(i, blendDuration);
            return;
        }
    }
}

void AnimatedActor::update(float deltaTime) {
    if (m_Animator)
        m_Animator->update(deltaTime);
}

}  // namespace se::scene
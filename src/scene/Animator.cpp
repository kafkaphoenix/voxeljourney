#include "Animator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace se::scene {

Animator::Animator(se::assets::ModelHandle handle, int clipIndex) : m_Model(handle), m_ClipIndex(clipIndex) {
    m_Bones.fill(glm::mat4{1.0f});

    auto* model = m_Model.get();
    if (model) {
        const auto& animations = model->getAnimations();
        if (animations.empty()) {
            m_ClipIndex = -1;
        } else {
            m_ClipIndex = std::clamp(m_ClipIndex, 0, static_cast<int>(animations.size()) - 1);
        }
    }

    if (clip()) {
        evaluateCurrentPose();
    }
}

float Animator::duration() const {
    const auto* c = clip();
    return c ? c->duration : 0.0f;
}

void Animator::play(int clipIndex) {
    if (m_ClipIndex == clipIndex) {
        return;
    }

    m_ClipIndex = clipIndex;
    m_CurrentTime = 0.0f;
    m_Playing = true;
    m_BlendTimer = 0.0f;
    m_BlendDuration = 0.0f;

    evaluateCurrentPose();
}

void Animator::play(std::string_view clipName) {
    auto* model = m_Model.get();
    if (!model) {
        throw std::runtime_error("Model handle is invalid");
    }

    const auto clipIndex = model->findAnimationClipIndex(clipName);
    if (!clipIndex.has_value()) {
        throw std::runtime_error("Animator: clip '" + std::string(clipName) + "' not found in model '" +
                                 std::string(model->getName()) + "'");
    }

    play(*clipIndex);
}

void Animator::blendTo(int clipIndex, float blendDuration) {
    if (blendDuration <= 0.0f) {
        play(clipIndex);
        return;
    }

    if (m_ClipIndex == clipIndex) {
        return;
    }

    if (const auto* currentClip = clip(); currentClip != nullptr) {
        if (const auto* s = skeleton(); s != nullptr) {
            currentClip->sample(m_CurrentTime, *s, m_BlendFromPose);
        }
    }

    m_BlendTimer = blendDuration;
    m_BlendDuration = blendDuration;

    m_ClipIndex = clipIndex;
    m_CurrentTime = 0.0f;
    m_Playing = true;
}

void Animator::blendTo(std::string_view clipName, float blendDuration) {
    auto* model = m_Model.get();
    if (!model) {
        throw std::runtime_error("Model handle is invalid");
    }

    const auto clipIndex = model->findAnimationClipIndex(clipName);
    if (!clipIndex.has_value()) {
        throw std::runtime_error("Animator: clip '" + std::string(clipName) + "' not found in model '" +
                                 std::string(model->getName()) + "'");
    }

    blendTo(*clipIndex, blendDuration);
}

void Animator::update(float deltaTime) {
    const auto* c = clip();

    if (!c || !m_Playing) {
        return;
    }

    if (c->duration <= 0.0f) {
        m_CurrentTime = 0.0f;
        evaluateCurrentPose();
        m_Playing = false;
        return;
    }

    m_CurrentTime += deltaTime;

    if (m_CurrentTime >= c->duration) {
        if (m_Loop) {
            m_CurrentTime = std::fmod(m_CurrentTime, c->duration);
        } else {
            m_CurrentTime = c->duration;
            m_Playing = false;
        }
    }

    const auto* s = skeleton();
    if (s == nullptr) {
        return;
    }

    c->sample(m_CurrentTime, *s, m_LocalPose);

    if (m_BlendTimer > 0.0f) {
        m_BlendTimer = std::max(m_BlendTimer - deltaTime, 0.0f);

        const float t = 1.0f - std::clamp(m_BlendTimer / m_BlendDuration, 0.0f, 1.0f);

        // Blend between the previous pose and the current pose based on t, and build matrices from the blended pose
        se::assets::Pose blendedPose{};

        const int n = boneCount();

        for (int i = 0; i < n && i < se::assets::MAX_BONES; ++i) {
            const se::assets::BonePose& from = m_BlendFromPose.at(i);
            const se::assets::BonePose& to = m_LocalPose.at(i);

            blendedPose.at(i).translation = glm::mix(from.translation, to.translation, t);

            blendedPose.at(i).rotation = glm::normalize(glm::slerp(from.rotation, to.rotation, t));

            blendedPose.at(i).scale = glm::mix(from.scale, to.scale, t);
        }

        s->buildPalette(blendedPose, m_Bones);
    } else {  // No blending, just use the current pose
        s->buildPalette(m_LocalPose, m_Bones);
    }
}

void Animator::play() { m_Playing = true; }

void Animator::pause() { m_Playing = false; }

void Animator::stop() {
    m_Playing = false;
    m_CurrentTime = 0.0f;

    evaluateCurrentPose();
}

const se::assets::Skeleton* Animator::skeleton() const {
    auto* model = m_Model.get();
    if (!model) {
        throw std::runtime_error("Model handle is invalid");
    }
    return &model->getSkeleton();
}

const se::assets::AnimationClip* Animator::clip() const {
    auto* model = m_Model.get();
    if (!model) {
        throw std::runtime_error("Model handle is invalid");
    }
    const auto& anims = model->getAnimations();

    if (m_ClipIndex < 0 || m_ClipIndex >= static_cast<int>(anims.size())) {
        return nullptr;
    }

    return &anims[m_ClipIndex];
}

int Animator::boneCount() const {
    const auto* s = skeleton();
    return s ? static_cast<int>(s->bones.size()) : 0;
}

void Animator::evaluateCurrentPose() {
    const auto* c = clip();
    const auto* s = skeleton();
    if (!c || !s) {
        return;
    }

    c->sample(m_CurrentTime, *s, m_LocalPose);
    s->buildPalette(m_LocalPose, m_Bones);
}

}  // namespace se::scene
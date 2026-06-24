#pragma once

#include <glm/glm.hpp>

#include "Camera.h"
#include "CameraController.h"
#include "CharacterController.h"
#include "Transform.h"

namespace se::core {
class Input;
class Config;
}

namespace se::scene {
struct AnimatedInstance;

class Player {
public:
    explicit Player(const se::core::Config& config);
    ~Player() = default;

    void update(float deltaTime, const se::core::Input& input);
    void finalizeFrame(float deltaTime);
    void setBodyInstance(AnimatedInstance* bodyInstance);

    [[nodiscard]] Camera& getCamera() { return m_Camera; }
    [[nodiscard]] const Camera& getCamera() const { return m_Camera; }
    [[nodiscard]] glm::vec3 getPosition() const { return m_Transform.position; }

private:
    struct RootMotionSettings {
        bool enabled = false;
        float playbackSpeed = 1.0f;
        glm::vec3 translationMask{1.0f, 0.0f, 1.0f};
    };

    void updateFirstPerson(float deltaTime, const se::core::Input& input);
    void updateThirdPerson(float deltaTime, const se::core::Input& input);
    void configureBodyAnimator() const;
    void applyRootMotion(float deltaTime);
    void syncBodyToPlayer() const;

    Transform m_Transform;
    Camera m_Camera;
    AnimatedInstance* m_BodyInstance = nullptr;
    CharacterController m_CharacterController;
    CameraController m_CameraController;
    RootMotionSettings m_RootMotion;
};

}  // namespace se::scene
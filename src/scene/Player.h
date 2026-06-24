#pragma once

#include "AnimationController.h"
#include "Camera.h"
#include "CameraController.h"
#include "CharacterController.h"
#include "PlayerIntent.h"
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

    [[nodiscard]] PlayerIntent sampleIntent(const se::core::Input& input) const;
    void update(float deltaTime, const PlayerIntent& intent);
    void finalizeFrame(float deltaTime);
    void setBodyInstance(AnimatedInstance* bodyInstance);

    [[nodiscard]] Camera& getCamera() { return m_Camera; }
    [[nodiscard]] const Camera& getCamera() const { return m_Camera; }
    [[nodiscard]] glm::vec3 getPosition() const { return m_Transform.position; }

private:
    struct RootMotionSettings {
        bool enabled = false;
    };

    void updateFirstPerson(float deltaTime, const PlayerIntent& intent);
    void updateThirdPerson(float deltaTime, const PlayerIntent& intent);
    void configureBodyAnimator() const;
    void syncBodyToPlayer() const;

    Transform m_Transform;
    Camera m_Camera;
    AnimatedInstance* m_BodyInstance = nullptr;
    AnimationController m_AnimationController;
    CharacterController m_CharacterController;
    CameraController m_CameraController;
    RootMotionSettings m_RootMotion;
};

}  // namespace se::scene
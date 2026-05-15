#pragma once

#include <glm/glm.hpp>

#include "Camera.h"
#include "Transform.h"

namespace se::core {
class Input;
class Config;
}

namespace se::scene {
struct AnimatedInstance;

class Player {
public:
    struct BodyClips {
        std::string idle = "Survey";
        std::string walk = "Walk";
        std::string run = "Run";
    };

    Player(const se::core::Config& config);
    ~Player() = default;

    void update(float deltaTime, const se::core::Input& input);
    void setBodyInstance(AnimatedInstance* bodyInstance);

    [[nodiscard]] Camera& getCamera() { return m_Camera; }
    [[nodiscard]] const Camera& getCamera() const { return m_Camera; }
    [[nodiscard]] glm::vec3 getPosition() const { return m_Transform.position; }

private:
    enum class MoveState : uint8_t { Idle, Walking, Running };

    void updateMouseLook(const se::core::Input& input);
    void updateMovement(float deltaTime, const se::core::Input& input);
    void applyMovementStep(float stepSeconds, const se::core::Input& input);
    void updateMoveState(const se::core::Input& input);
    void syncCameraAndBody();
    void transitionToClip(std::string_view clipName, float blendDuration);

    Transform m_Transform;
    Camera m_Camera;
    AnimatedInstance* m_BodyInstance = nullptr;
    MoveState m_MoveState = MoveState::Idle;
    BodyClips m_Clips;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_WalkSpeed = 20.0f;
    float m_RunSpeed = 40.0f;
    float m_CameraHeight = 1.0f;
    float m_CameraDistance = 1.0f;

    float m_MouseSensitivity = 0.1f;
    float m_MouseSmoothAlpha = 0.15f;
    float m_SmoothedDx = 0.0f;
    float m_SmoothedDy = 0.0f;

    bool m_UseFixedStep = true;
    float m_FixedStep = 1.0f / 60.0f;
    float m_MoveAccumulator = 0.0f;
};

}  // namespace se::scene
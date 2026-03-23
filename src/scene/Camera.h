#pragma once
#include <glm/glm.hpp>

namespace se::scene {

class Camera {
   public:
    struct Config {
        float aspectRatio = 16.0f / 9.0f;
        glm::vec3 position = {-5.0f, 5.0f, 5.0f};
        float moveSpeed = 10.0f;
        float mouseSensitivity = 0.1f;
        float fov = 60.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
    };

    Camera();

    void processMouse(float xoffset, float yoffset);
    void processKeyboard(bool forward, bool backward,
                         bool left, bool right,
                         bool up, bool down,
                         float deltaTime);

    glm::mat4 getViewProjection() const;
    glm::vec3 getPosition() const { return m_Position; }

    void setAspect(float aspect) { m_Aspect = aspect; }
    void setPosition(const glm::vec3& position) { m_Position = position; }
    void setMoveSpeed(float speed);
    void setMouseSensitivity(float sensitivity);
    void setFov(float fovDegrees);
    void setClipPlanes(float nearPlane, float farPlane);
    void applyConfig(const Config& config);

   private:
    void updateVectors();

    glm::vec3 m_Position{-5.0f, 5.0f, 5.0f};
    glm::vec3 m_Front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_Up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_Right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_WorldUp{0.0f, 1.0f, 0.0f};
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_Aspect = 16.0f / 9.0f;
    float m_Speed = 10.0f;
    float m_Sensitivity = 0.1f;
    float m_Fov = 60.0f;
    float m_Near = 0.1f;
    float m_Far = 1000.0f;
};

}  // namespace se::scene
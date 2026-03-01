#pragma once
#include <glm/glm.hpp>

namespace se::scene {

class Camera {
   public:
    struct Settings {
        glm::vec3 position{-5.0f, 5.0f, 5.0f};
        float moveSpeed = 10.0f;
        float mouseSensitivity = 0.1f;
        float fov = 60.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
    };

    Camera(float aspectRatio);

    void processMouse(float xoffset, float yoffset);
    void processKeyboard(bool forward, bool backward, bool left, bool right, bool up, bool down, float deltaTime);

    glm::mat4 getViewProjection() const;
    void setAspect(float aspect) { m_Aspect = aspect; }
    void setPosition(const glm::vec3& position) { m_Position = position; }
    void setMoveSpeed(float speed);
    void setMouseSensitivity(float sensitivity);
    void setFov(float fovDegrees);
    void setClipPlanes(float nearPlane, float farPlane);
    void applySettings(const Settings& settings);

   private:
    void updateVectors();

    glm::vec3 m_Position;
    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    float m_Yaw;
    float m_Pitch;
    float m_Aspect;
    float m_Speed;
    float m_Sensitivity;
    float m_Fov;
    float m_Near;
    float m_Far;
};

}  // namespace se::scene
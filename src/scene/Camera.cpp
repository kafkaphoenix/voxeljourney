#include "Camera.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace se::scene {

Camera::Camera() {
    updateVectors();
}

void Camera::updateVectors() {
    glm::vec3 front;
    front.x = std::cos(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));
    front.y = std::sin(glm::radians(m_Pitch));
    front.z = std::sin(glm::radians(m_Yaw)) * std::cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void Camera::processMouse(float xoffset, float yoffset) {
    m_Yaw += xoffset * m_Sensitivity;
    m_Pitch += yoffset * m_Sensitivity;
    m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
    updateVectors();
}

void Camera::processKeyboard(bool forward, bool backward,
                             bool left, bool right,
                             bool up, bool down,
                             float deltaTime) {
    const float velocity = m_Speed * deltaTime;

    glm::vec3 flatFront = glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z));
    glm::vec3 flatRight = glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z));

    if (forward) m_Position += flatFront * velocity;
    if (backward) m_Position -= flatFront * velocity;
    if (left) m_Position -= flatRight * velocity;
    if (right) m_Position += flatRight * velocity;
    if (up) m_Position.y += velocity;
    if (down) m_Position.y -= velocity;
}

void Camera::setMoveSpeed(float speed) {
    if (speed > 0.0f) m_Speed = speed;
}

void Camera::setMouseSensitivity(float sensitivity) {
    if (sensitivity > 0.0f) m_Sensitivity = sensitivity;
}

void Camera::setFov(float fovDegrees) {
    if (fovDegrees > 1.0f && fovDegrees < 179.0f) m_Fov = fovDegrees;
}

void Camera::setClipPlanes(float nearPlane, float farPlane) {
    if (nearPlane > 0.0f && farPlane > nearPlane) {
        m_Near = nearPlane;
        m_Far = farPlane;
    }
}

void Camera::applyConfig(const Config& config) {
    setAspect(config.aspectRatio);
    setPosition(config.position);
    setMoveSpeed(config.moveSpeed);
    setMouseSensitivity(config.mouseSensitivity);
    setFov(config.fov);
    setClipPlanes(config.nearPlane, config.farPlane);
    updateVectors();
}

glm::mat4 Camera::getViewProjection() const {
    const glm::mat4 view = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    const glm::mat4 proj = glm::perspective(
        glm::radians(m_Fov), m_Aspect, m_Near, m_Far);
    return proj * view;
}

}  // namespace se::scene
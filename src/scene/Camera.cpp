#include "Camera.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace se::scene {

Camera::Camera(float aspectRatio)
    : m_Position(-5.0f, 5.0f, 5.0f),  // Eye level
      m_Front(0.0f, 0.0f, -1.0f),
      m_Up(0.0f, 1.0f, 0.0f),
      m_WorldUp(m_Up),
      m_Yaw(0.0f),
      m_Pitch(0.0f),
      m_Aspect(aspectRatio),
      m_Speed(10.0f),
      m_Sensitivity(0.1f),
      m_Fov(60.0f),
      m_Near(0.1f),
      m_Far(1000.0f) {
    updateVectors();
}

void Camera::updateVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void Camera::processMouse(float xoffset, float yoffset) {
    xoffset *= m_Sensitivity;
    yoffset *= m_Sensitivity;

    m_Yaw += xoffset;
    m_Pitch += yoffset;

    if (m_Pitch > 89.0f) m_Pitch = 89.0f;
    if (m_Pitch < -89.0f) m_Pitch = -89.0f;

    updateVectors();
}

void Camera::processKeyboard(bool forward, bool backward, bool left, bool right, bool up, bool down, float deltaTime) {
    float velocity = m_Speed * deltaTime;

    glm::vec3 frontMovement = glm::vec3(m_Front.x, 0.0f, m_Front.z);
    glm::vec3 rightMovement = glm::vec3(m_Right.x, 0.0f, m_Right.z);

    float frontLen = glm::length(frontMovement);
    float rightLen = glm::length(rightMovement);

    if (frontLen > 0.001f) frontMovement = frontMovement / frontLen;
    if (rightLen > 0.001f) rightMovement = rightMovement / rightLen;

    if (forward) m_Position += frontMovement * velocity;
    if (backward) m_Position -= frontMovement * velocity;
    if (left) m_Position -= rightMovement * velocity;
    if (right) m_Position += rightMovement * velocity;

    if (up) m_Position.y += velocity;
    if (down) m_Position.y -= velocity;
}

void Camera::setMoveSpeed(float speed) {
    if (speed > 0.0f) {
        m_Speed = speed;
    }
}

void Camera::setMouseSensitivity(float sensitivity) {
    if (sensitivity > 0.0f) {
        m_Sensitivity = sensitivity;
    }
}

void Camera::setFov(float fovDegrees) {
    if (fovDegrees > 1.0f && fovDegrees < 179.0f) {
        m_Fov = fovDegrees;
    }
}

void Camera::setClipPlanes(float nearPlane, float farPlane) {
    if (nearPlane > 0.0f && farPlane > nearPlane) {
        m_Near = nearPlane;
        m_Far = farPlane;
    }
}

void Camera::applySettings(const Settings& settings) {
    setPosition(settings.position);
    setMoveSpeed(settings.moveSpeed);
    setMouseSensitivity(settings.mouseSensitivity);
    setFov(settings.fov);
    setClipPlanes(settings.nearPlane, settings.farPlane);
}

glm::mat4 Camera::getViewProjection() const {
    glm::mat4 view = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
    glm::mat4 proj = glm::perspective(glm::radians(m_Fov), m_Aspect, m_Near, m_Far);
    return proj * view;
}

}  // namespace se::scene
#include "Core3D/Camera.hpp"
#include "config.hpp"

namespace Core3D {

Camera::Camera(const glm::vec3 &position, const glm::vec3 &up,
               float yaw, float pitch)
    : m_Position(position),
      m_Front(glm::vec3(0.0f, 0.0f, -1.0f)),
      m_Up(up),
      m_Right(glm::vec3(1.0f, 0.0f, 0.0f)),
      m_WorldUp(up),
      m_Yaw(yaw),
      m_Pitch(pitch),
      m_FOV(PTSD_Config::DEFAULT_FOV),
      m_AspectRatio(static_cast<float>(PTSD_Config::WINDOW_WIDTH) /
                    static_cast<float>(PTSD_Config::WINDOW_HEIGHT)),
      m_NearClip(PTSD_Config::NEAR_CLIP_3D),
      m_FarClip(PTSD_Config::FAR_CLIP_3D) {
    UpdateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

glm::mat4 Camera::GetProjectionMatrix() const {
    if (m_ProjectionType == ProjectionType::PERSPECTIVE) {
        return glm::perspective(glm::radians(m_FOV), m_AspectRatio,
                                m_NearClip, m_FarClip);
    } else {
        float halfW = m_OrthoSize * m_AspectRatio;
        float halfH = m_OrthoSize;
        return glm::ortho(-halfW, halfW, -halfH, halfH,
                          m_NearClip, m_FarClip);
    }
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime) {
    float velocity = m_MovementSpeed * deltaTime;

    // 水平前方向量（忽略 pitch，保持等速移動）
    glm::vec3 flatFront = glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z));

    switch (direction) {
    case CameraMovement::FORWARD:
        m_Position += flatFront * velocity;
        break;
    case CameraMovement::BACKWARD:
        m_Position -= flatFront * velocity;
        break;
    case CameraMovement::LEFT:
        m_Position -= m_Right * velocity;
        break;
    case CameraMovement::RIGHT:
        m_Position += m_Right * velocity;
        break;
    case CameraMovement::UP:
        m_Position += m_WorldUp * velocity;
        break;
    case CameraMovement::DOWN:
        m_Position -= m_WorldUp * velocity;
        break;
    }
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset,
                                  bool constrainPitch) {
    xOffset *= m_MouseSensitivity;
    yOffset *= m_MouseSensitivity;

    m_Yaw += xOffset;
    m_Pitch += yOffset;

    if (constrainPitch) {
        if (m_Pitch > 89.0f) m_Pitch = 89.0f;
        if (m_Pitch < -89.0f) m_Pitch = -89.0f;
    }

    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset) {
    m_FOV -= yOffset;
    if (m_FOV < 1.0f) m_FOV = 1.0f;
    if (m_FOV > 120.0f) m_FOV = 120.0f;
}

void Camera::LookAt(const glm::vec3 &target) {
    glm::vec3 direction = glm::normalize(target - m_Position);
    m_Pitch = glm::degrees(asin(direction.y));
    m_Yaw = glm::degrees(atan2(direction.z, direction.x));
    UpdateCameraVectors();
}

void Camera::UpdateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    front.y = sin(glm::radians(m_Pitch));
    front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
    m_Front = glm::normalize(front);

    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

} // namespace Core3D

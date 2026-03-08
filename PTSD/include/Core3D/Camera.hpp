#ifndef CORE3D_CAMERA_HPP
#define CORE3D_CAMERA_HPP

#include "pch.hpp" // IWYU pragma: export

namespace Core3D {

/**
 * @brief Camera movement directions for keyboard input.
 */
enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

/**
 * @brief Projection type for the camera.
 */
enum class ProjectionType {
    PERSPECTIVE,
    ORTHOGRAPHIC
};

/**
 * @brief 3D camera with FPS-style and orbit-style controls.
 *
 * Supports both perspective and orthographic projection.
 * Integrates with the existing Util::Input system.
 *
 * @code
 * // Basic usage:
 * Core3D::Camera camera(glm::vec3(0, 5, 10));
 * camera.SetFOV(60.0f);
 *
 * // In update loop:
 * if (Util::Input::IsKeyPressed(Util::Keycode::W))
 *     camera.ProcessKeyboard(Core3D::CameraMovement::FORWARD, deltaTime);
 * camera.ProcessMouseMovement(xOffset, yOffset);
 *
 * // Get matrices for rendering:
 * glm::mat4 view = camera.GetViewMatrix();
 * glm::mat4 proj = camera.GetProjectionMatrix();
 * @endcode
 */
class Camera {
public:
    /**
     * @brief Construct a camera at the given position.
     * @param position World-space position (default: origin).
     * @param up World up vector (default: Y-up).
     * @param yaw Initial yaw in degrees (default: -90 = looking along -Z).
     * @param pitch Initial pitch in degrees (default: 0).
     */
    Camera(const glm::vec3 &position = glm::vec3(0.0f, 0.0f, 3.0f),
           const glm::vec3 &up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f);

    // ====== Matrix Getters ======

    /** @brief Get the view matrix (lookAt). */
    glm::mat4 GetViewMatrix() const;

    /** @brief Get the projection matrix (perspective or orthographic). */
    glm::mat4 GetProjectionMatrix() const;

    // ====== Input Processing ======

    /**
     * @brief Move the camera based on keyboard input.
     * @param direction Movement direction.
     * @param deltaTime Frame delta time for speed normalization.
     */
    void ProcessKeyboard(CameraMovement direction, float deltaTime);

    /**
     * @brief Rotate the camera based on mouse movement.
     * @param xOffset Horizontal mouse delta.
     * @param yOffset Vertical mouse delta.
     * @param constrainPitch Clamp pitch to (-89, 89) degrees (default: true).
     */
    void ProcessMouseMovement(float xOffset, float yOffset,
                              bool constrainPitch = true);

    /**
     * @brief Zoom by adjusting FOV based on scroll input.
     * @param yOffset Scroll wheel delta.
     */
    void ProcessMouseScroll(float yOffset);

    // ====== Setters ======

    void SetPosition(const glm::vec3 &pos) { m_Position = pos; }
    void SetFOV(float fov) { m_FOV = fov; }
    void SetAspectRatio(float ratio) { m_AspectRatio = ratio; }
    void SetNearClip(float near) { m_NearClip = near; }
    void SetFarClip(float far) { m_FarClip = far; }
    void SetMovementSpeed(float speed) { m_MovementSpeed = speed; }
    void SetMouseSensitivity(float sens) { m_MouseSensitivity = sens; }
    void SetProjectionType(ProjectionType type) { m_ProjectionType = type; }
    void SetOrthoSize(float size) { m_OrthoSize = size; }
    void SetYaw(float yaw) { m_Yaw = yaw; }
    void SetPitch(float pitch) { m_Pitch = pitch; }

    /** @brief Recalculate direction vectors after changing yaw/pitch manually. */
    void UpdateVectors() { UpdateCameraVectors(); }

    /**
     * @brief Point the camera at a target position (orbit mode).
     * @param target The world-space point to look at.
     */
    void LookAt(const glm::vec3 &target);

    // ====== Getters ======

    const glm::vec3 &GetPosition() const { return m_Position; }
    const glm::vec3 &GetFront() const { return m_Front; }
    const glm::vec3 &GetUp() const { return m_Up; }
    const glm::vec3 &GetRight() const { return m_Right; }
    float GetFOV() const { return m_FOV; }
    float GetYaw() const { return m_Yaw; }
    float GetPitch() const { return m_Pitch; }
    float GetAspectRatio() const { return m_AspectRatio; }
    ProjectionType GetProjectionType() const { return m_ProjectionType; }

private:
    /** @brief Recalculate front/right/up vectors from yaw and pitch. */
    void UpdateCameraVectors();

    // Position and orientation
    glm::vec3 m_Position;
    glm::vec3 m_Front;
    glm::vec3 m_Up;
    glm::vec3 m_Right;
    glm::vec3 m_WorldUp;

    // Euler angles (degrees)
    float m_Yaw;
    float m_Pitch;

    // Projection
    ProjectionType m_ProjectionType = ProjectionType::PERSPECTIVE;
    float m_FOV;
    float m_AspectRatio;
    float m_NearClip;
    float m_FarClip;
    float m_OrthoSize = 10.0f; ///< Half-size for orthographic projection

    // Control settings
    float m_MovementSpeed = 5.0f;
    float m_MouseSensitivity = 0.1f;
};

} // namespace Core3D

#endif
